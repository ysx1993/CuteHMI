#include "../cutehmi.metadata.hpp"
#include "../cutehmi.dirs.hpp"
#include "cutehmi/daemon/logging.hpp"
#include "cutehmi/daemon/Daemon.hpp"
#include "cutehmi/daemon/EngineThread.hpp"
#include "cutehmi/daemon/CoreData.hpp"

#include <cutehmi/CuteHMI.hpp>
#include <cutehmi/Singleton.hpp>

#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QDir>
#include <QCommandLineParser>
#include <QUrl>
#include <QTranslator>
#include <QLibraryInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

using namespace cutehmi::daemon;

/**
 * Main function.
 * @param argc number of arguments passed to the program.
 * @param argv list of arguments passed to the program.
 * @return return code.
 */
int main(int argc, char * argv[])
{
	//<cutehmi_daemon-silent_initialization.principle>
	// Output shall remain silent until daemon logging is set up.

	//<Qt-Qt_5_7_0_Reference_Documentation-Threads_and_QObjects-QObject_Reentrancy-creating_QObjects_before_QApplication.assumption>
	// "In general, creating QObjects before the QApplication is not supported and can lead to weird crashes on exit, depending on the
	//	platform. This means static instances of QObject are also not supported. A properly structured single or multi-threaded application
	//	should make the QApplication be the first created, and last destroyed QObject."

	// Set up application.

	QCoreApplication::setOrganizationDomain(QString(CUTEHMI_DAEMON_VENDOR).toLower());
	QCoreApplication::setApplicationName(CUTEHMI_DAEMON_VENDOR " " CUTEHMI_DAEMON_FRIENDLY_NAME);
	QCoreApplication::setApplicationVersion(QString("%1.%2.%3").arg(CUTEHMI_DAEMON_MAJOR).arg(CUTEHMI_DAEMON_MINOR).arg(CUTEHMI_DAEMON_MICRO));

	QCoreApplication app(argc, argv);


	// Configure command line parser and process arguments.

	QJsonDocument metadataJson;
	{
		QFile metadataFile(":/" CUTEHMI_DAEMON_NAME "/cutehmi.metadata.json");
		if (metadataFile.open(QFile::ReadOnly)) {
			metadataJson = QJsonDocument::fromJson(metadataFile.readAll());
		} else
			CUTEHMI_DIE("Could not open ':/" CUTEHMI_DAEMON_NAME "/cutehmi.metadata.json' file.");
	}

	QCommandLineParser cmd;
	cmd.setApplicationDescription(QCoreApplication::applicationName() + "\n" + metadataJson.object().value("description").toString());
	cmd.addHelpOption();
	cmd.addVersionOption();
	CoreData::Options opt {
		QCommandLineOption("app", QCoreApplication::translate("main", "Run project in application mode.")),
		QCommandLineOption("basedir", QCoreApplication::translate("main", "Set base directory to <dir>."), QCoreApplication::translate("main", "dir")),
		QCommandLineOption("lang", QCoreApplication::translate("main", "Choose application <language>."), QCoreApplication::translate("main", "language")),
		QCommandLineOption("pidfile", QCoreApplication::translate("main", "PID file <path> (Unix-specific)."), QCoreApplication::translate("main", "path")),
		QCommandLineOption({"p", "project"}, QCoreApplication::translate("main", "Load QML project <URL>."), QCoreApplication::translate("main", "URL"))
	};
	opt.pidfile.setDefaultValue(QString("/var/run/") + CUTEHMI_DAEMON_NAME ".pid");
	opt.pidfile.setDescription(opt.pidfile.description() + "\nDefault value: '" + opt.pidfile.defaultValues().at(0) + "'.");
	opt.basedir.setDefaultValue(QDir(QCoreApplication::applicationDirPath() + "/..").canonicalPath());
	opt.basedir.setDescription(opt.basedir.description() + "\nDefault value: '" + opt.basedir.defaultValues().at(0) + "'.");
	cmd.addOption(opt.app);
	cmd.addOption(opt.basedir);
	cmd.addOption(opt.lang);
	cmd.addOption(opt.pidfile);
	cmd.addOption(opt.project);
	cmd.process(app);


	// Prepare program core.

	CoreData data;
	data.app = & app;
	data.cmd = & cmd;
	data.opt = & opt;

	std::function<int(CoreData &)> core = [](CoreData & data) {
		try {

			CUTEHMI_DEBUG("Default locale: " << QLocale());

			QTranslator qtTranslator;
			if (data.cmd->isSet(data.opt->lang))
				qtTranslator.load("qt_" + data.cmd->value(data.opt->lang), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
			else
				qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
			data.app->installTranslator(& qtTranslator);

			QDir baseDir = data.cmd->value(data.opt->basedir);
			QString baseDirPath = baseDir.absolutePath() + "/";
			CUTEHMI_DEBUG("Base directory: " << baseDirPath);

			CUTEHMI_DEBUG("Library paths: " << QCoreApplication::libraryPaths());

			EngineThread engineThread;
			std::unique_ptr<QQmlApplicationEngine> engine(new QQmlApplicationEngine);
			engine->addImportPath(baseDirPath + CUTEHMI_DIRS_QML_EXTENSION_INSTALL_DIRNAME);
			CUTEHMI_DEBUG("QML import paths: " << engine->importPathList());

			if (!data.cmd->value(data.opt->project).isNull()) {
				CUTEHMI_DEBUG("Project: " << data.cmd->value(data.opt->project));
				QUrl projectUrl(data.cmd->value(data.opt->project));
				if (projectUrl.isValid()) {
					// Assure that URL is not mixing relative path with explicitly specified scheme, which is forbidden. QUrl::isValid() doesn't check this out.
					if (!projectUrl.scheme().isEmpty() && QDir::isRelativePath(projectUrl.path()))
						cutehmi::CuteHMI::Instance().popupBridge()->critical(QObject::tr("URL '%1' contains relative path along with URL scheme, which is forbidden.").arg(projectUrl.url()));
					else {
						// If source URL is relative (does not contain scheme), then make absolute URL: file:///baseDirPath/sourceUrl.
						if (projectUrl.isRelative())
							projectUrl = QUrl::fromLocalFile(baseDirPath).resolved(projectUrl);
						// Check if file exists and eventually set context property.
						if (projectUrl.isLocalFile() && !QFile::exists(projectUrl.toLocalFile()))
							cutehmi::CuteHMI::Instance().popupBridge()->critical(QObject::tr("Project file '%1' does not exist.").arg(projectUrl.url()));
						else {
							engine->moveToThread(& engineThread);

							QObject::connect(& engineThread, & EngineThread::loadRequested, engine.get(), QOverload<const QString &>::of(& QQmlApplicationEngine::load));
							QObject::connect(& engineThread, SIGNAL(loadRequested(const QString &)), & engineThread, SLOT(start()));
							QObject::connect(data.app, & QCoreApplication::aboutToQuit, & engineThread, & QThread::quit);
							// Delegate management of engine to EngineThread, so that it gets deleted after thread finishes execution.
							QObject::connect(& engineThread, & QThread::finished, engine.release(), & QObject::deleteLater);

							emit engineThread.loadRequested(projectUrl.url());
							int result = data.app->exec();
							engineThread.wait();

							return result;
						}
					}
				} else
					cutehmi::CuteHMI::Instance().popupBridge()->critical(QObject::tr("Invalid format of project URL '%1'.").arg(data.cmd->value(data.opt->project)));
			} else
				cutehmi::CuteHMI::Instance().popupBridge()->note(QObject::tr("No project file has been specified."));

			return EXIT_SUCCESS;

		} catch (const cutehmi::PopupBridge::NoAdvertiserException & e) {
			CUTEHMI_CRITICAL("Prompt message: " << e.prompt()->text());
			if (!e.prompt()->informativeText().isEmpty())
				CUTEHMI_CRITICAL("Informative text: " << e.prompt()->informativeText());
			if (!e.prompt()->detailedText().isEmpty())
				CUTEHMI_CRITICAL("Detailed text: " << e.prompt()->detailedText());
			CUTEHMI_CRITICAL("Available buttons: " << e.prompt()->buttons());
		} catch (const std::exception & e) {
			CUTEHMI_CRITICAL(e.what());
		} catch (...) {
			CUTEHMI_CRITICAL("Caught unrecognized exception.");
			throw;
		}

		return  EXIT_FAILURE;
	};


	// Run program core in daemon or application mode.

	int exitCode;

	if (!cmd.isSet(opt.app)) {
		Daemon daemon(& data, core);

	// At this point logging should be configured and printing facilities silenced. Not much to say anyways...
	//</cutehmi_daemon-silent_initialization.principle>

		exitCode = daemon.exitCode();
	} else
		exitCode = core(data);

	// Destroy singleton instances before QCoreApplication. Ignoring the recommendation to connect clean-up code to the
	// aboutToQuit() signal, because for daemon it is always violent termination if QCoreApplication::exec() does not exit.
	cutehmi::destroySingletonInstances();

	return exitCode;

	//</Qt-Qt_5_7_0_Reference_Documentation-Threads_and_QObjects-QObject_Reentrancy-creating_QObjects_before_QApplication.assumption>
}

//(c)MP: Copyright © 2019, Michal Policht. All rights reserved.
//(c)MP: This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
