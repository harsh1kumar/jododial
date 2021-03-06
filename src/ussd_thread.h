/*
 * Copyright (C) 2014 Harsh Kumar <harsh1kumar@gmail.com>
 *
 * This file is part of Jododial.
 *
 * Jododial is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Jododial is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Jododial.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef USSD_THREAD_H
#define USSD_THREAD_H

#include <QThread>
#include <QString>

class UssdThread : public QThread
{
	Q_OBJECT

public:
	UssdThread();

	QString command;
	QString reply;
	void run();

private:
	/* Private data */
	static const QString service;
	static const QString modemInterface;
	static const QString ussdInterface;
	QString path;

	/* Private functions */
	void setModemPath();
	void ussdCall();
};

#endif // USSD_THREAD_H
