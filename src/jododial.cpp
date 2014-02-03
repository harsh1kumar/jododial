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

#include "jododial.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QSettings>
#include <QCheckBox>

Jododial::Jododial(QWidget * parent):
		QWidget(parent)
{
	QLabel * titleLabel = new QLabel(tr("Jododial"));
	QString fontFamily = (this->font()).family();
	titleLabel->setFont(QFont(fontFamily, 16, QFont::Bold));

	outputText = new QPlainTextEdit("");
	outputText->setReadOnly(true);

	networkCombo = new QComboBox;
	networkCombo->setEditable(true);

	connectButton = new QPushButton(tr("&Connect"));

	wvdialProc = new QProcess(this);

	/* Set layout */
	QVBoxLayout * mainLayout = new QVBoxLayout;
	mainLayout->addWidget(titleLabel);
	mainLayout->addWidget(outputText);
	mainLayout->addWidget(networkCombo);
	mainLayout->addWidget(connectButton);
	setLayout(mainLayout);

	createSysTrayIcon();
	findNetworks();

	setWindowTitle(tr("Jododial"));

	readSettings();

	connect(connectButton, SIGNAL(clicked()), this, SLOT(connectDisconnect()));
	connect(wvdialProc, SIGNAL(readyReadStandardOutput()), this, SLOT(printOutput()));
}

/*
 * Creates System Tray Icon with associated actions & menu
 */
void Jododial::createSysTrayIcon()
{
	/* Create Actions for System Tray Icon Menu */
	restoreAct = new QAction(tr("&Restore"), this);
	connect(restoreAct, SIGNAL(triggered()), this, SLOT(show()));

	quitAct = new QAction(tr("&Quit"), this);
	connect(quitAct, SIGNAL(triggered()), this, SLOT(saveAndQuit()));

	/* Create System Tray Icon Menu */
	sysTrayMenu = new QMenu(this);
	sysTrayMenu->addAction(restoreAct);
	sysTrayMenu->addAction(quitAct);

	/* Create System Tray Icon*/
	sysTrayIcon = new QSystemTrayIcon(this);
	sysTrayIcon->setIcon(this->windowIcon()); /* Use the icon of parent */
	sysTrayIcon->setContextMenu(sysTrayMenu);
	sysTrayIcon->show();

	connect(sysTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), 
		this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
}

/*
 * Find the networks which have been configured in wvdial.conf &
 * put them in the combo box
 */
void Jododial::findNetworks()
{
	QFile configFile("/etc/wvdial.conf");
	if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::warning(this, "Warning",
				"<center>Unable to open \"/etc/wvdial.conf\" for reading.<br>"
				"Combo Box will be empty</center>");
		return;
	}

	QTextStream in(&configFile);
	QString line = in.readLine();
	while (!line.isNull())
	{
		line = line.trimmed(); // Remove any leading or trailing white spaces
		if (!line.startsWith(";"))
		{
			// Line is not commented
			if (line.contains("Dialer") && line.startsWith("[") && line.endsWith("]"))
			{
				// A line with network name found
				line.remove("Dialer");
				line.remove(0,1); // Remove leading '['
				line.chop(1); // Remove trailing ']'
				line = line.trimmed(); // Remove any leading or trailing white spaces

				// 'line' now has a network name
				// Insert the network name in the Combo Box
				networkCombo->insertItem(0, line);
			}
		}
		// Read next line
		line = in.readLine();
	}
	configFile.close();
}

/*
 * Reimplemented Event Handler for Jododial's close event
 *
 * It hides the Jododial widget & ignores the close event, so that
 * it seems that Jododial has minimized to system tray
 */
void Jododial::closeEvent(QCloseEvent *event)
{
	if (showMsgOnHide)
	{
		QMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Information);
		msgBox.setText(tr("Jododial is still running."));
		msgBox.setInformativeText(tr("To quit, right-click on system tray icon "
					"& choose <b>Quit</b>"));
		msgBox.setWindowTitle("Jododial");

		QCheckBox * chkBox = new QCheckBox("Do not show this again");
		msgBox.setCheckBox(chkBox);
		msgBox.exec();

		if (chkBox->checkState() == Qt::Checked)
			showMsgOnHide = false;
	}

	hide(); /* Hide Jododial Widget*/
	event->ignore(); /* Ignore the close event */
}

/*
 * Write settings & quit Jododial
 */
void Jododial::saveAndQuit()
{
	writeSettings();
	qApp->quit();
}

/*
 * Connect to network by starting wvdial process if process not running already
 * Disconnect the process if it is already running
 */
void Jododial::connectDisconnect()
{
	if (wvdialProc->state() == QProcess::NotRunning)
	{
		/* Connect */
		QString prog = "wvdial";
		QStringList args;
		args << networkCombo->currentText();

		/* Check to make sure combo box entry is not empty */
		if (!args.at(0).isEmpty())
		{
			/* Connect to the network */
			/* Merging stdout & stderr into a single channel */
			wvdialProc->setProcessChannelMode(QProcess::MergedChannels);
			wvdialProc->start(prog, args);
			qDebug() << "Connection attempted";
			connectButton->setText(tr("&Disconnect"));
		}
		else
		{
			QMessageBox::information(this, tr("No Network"), tr("Please enter a network name in the combo box."));
		}
		
	}
	else
	{
		/* Disconnect */
		wvdialProc->terminate();
		qDebug() << "Disconnection attempted";
		connectButton->setText(tr("&Connect"));
	}
}

/*
 * Print output in text box
 */
void Jododial::printOutput()
{
	QString output = wvdialProc->readAll();
	outputText->appendPlainText(output);
}

/*
 * Behaviour of tray icon when it is double clicked
 * 	If Jododial is visible, hide it
 * 	If Jododial is hidden, show it.
 */
void Jododial::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::DoubleClick)
	{
		if (this->isVisible())
			hide();
		else
			show();
	}
}

/*
 * Read settings from config file or load default value.
 * Settings read are:
 * 	Size
 * 	Point/Position
 * 	showMsgOnHide - Show messageBox when minimizing to system tray or not
 */
void Jododial::readSettings()
{
	QSettings settings("jododial", "jododial");

	QSize size = settings.value("size", QSize(400,200)).toSize();
	QPoint pos = settings.value("pos", QPoint(400,200)).toPoint();
	showMsgOnHide = settings.value("showMsgOnHide", true).toBool(); //By default, message box will be displayed
	resize(size);
	move(pos);
}

/*
 * Write settings to config file
 */
void Jododial::writeSettings()
{
	QSettings settings("jododial", "jododial");
	settings.setValue("size", size());
	settings.setValue("pos", pos());
	settings.setValue("showMsgOnHide", showMsgOnHide);
}

