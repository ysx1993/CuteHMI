#ifndef H_MODULES_CUTEHMI_u_STUPID_u_1_SRC_CUTEHMI_STUPID_PLUGIN_PROJECTPLUGIN_HPP
#define H_MODULES_CUTEHMI_u_STUPID_u_1_SRC_CUTEHMI_STUPID_PLUGIN_PROJECTPLUGIN_HPP

#include "../../../../cutehmi.metadata.hpp"
#include "../../../../include/cutehmi/stupid/Client.hpp"
#include "../../../../include/cutehmi/stupid/internal/DatabaseConnectionData.hpp"

#include <cutehmi/xml/IBackendPlugin.hpp>
#include <cutehmi/xml/ParseHelper.hpp>

#include <cutehmi/IProjectPlugin.hpp>

#include <QObject>
#include <QSqlDatabase>

#include <memory>

namespace cutehmi {
namespace stupid {
namespace plugin {

class ProjectPlugin:
	public QObject,
	public IProjectPlugin,
	public xml::IBackendPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID CUTEHMI_IPROJECTPLUGIN_IID FILE "../../../../cutehmi.metadata.json")
	Q_INTERFACES(cutehmi::IProjectPlugin)

	public:
		// IProjectPlugin
		void init(ProjectNode & node) override;

		// xml::IBackendPlugin
		void readXML(QXmlStreamReader & xmlReader, ProjectNode & node) override;

		// xml::IBackendPlugin
		void writeXML(QXmlStreamWriter & xmlWriter, ProjectNode & node) const noexcept(false) override;

	private:
		void parseStupid(const xml::ParseHelper & parentHelper, ProjectNode & node, const QString & id, const QString & name);

		void parsePostgreSQL(const xml::ParseHelper & parentHelper, stupid::DatabaseConnectionData & dbData);
};

}
}
}

#endif

//(c)MP: Copyright © 2018, Michal Policht. All rights reserved.
//(c)MP: This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
