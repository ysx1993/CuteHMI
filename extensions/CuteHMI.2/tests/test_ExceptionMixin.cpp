#include <cutehmi/ExceptionMixin.hpp>

#include <cutehmi/test/random.hpp>

#include <QtTest/QtTest>
#include <QtConcurrent>

namespace cutehmi {

class TestException:
	public ExceptionMixin<TestException>
{
	public:
		explicit TestException(const QString & message):
			ExceptionMixin<TestException>(message)
		{
		}

		TestException(const TestException & other) = default;

		~TestException() override;
};

TestException::~TestException()
{
}

class test_ExceptionMixin:
	public QObject
{
	Q_OBJECT

	private slots:
		void what();

		void concurrent();
};

void test_ExceptionMixin::what()
{
	try {
		throw TestException("Exception test.");
	} catch (TestException & e) {
		QCOMPARE(e.what(), "Exception test.");
	}

	QString randomString = test::rand<QString>();
	try {
		throw TestException(randomString);
	} catch (TestException & e) {
		QCOMPARE(e.what(), randomString.toLocal8Bit().constData());
	}
}

void test_ExceptionMixin::concurrent()
{
	try {
		int arr[1];
		QtConcurrent::blockingMap(arr, arr + 1, [](int){throw TestException("Exception thrown from QtConcurrent::blockingMap.");});
	} catch (TestException & e) {
		QCOMPARE(e.what(), "Exception thrown from QtConcurrent::blockingMap.");
	}
}

}

QTEST_MAIN(cutehmi::test_ExceptionMixin)
#include "test_ExceptionMixin.moc"

//(c)MP: Copyright © 2019, Michal Policht. All rights reserved.
//(c)MP: This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
