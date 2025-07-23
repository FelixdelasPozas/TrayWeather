/*
 File: Main.cpp
 Created on: 24/06/2025
 Author: Felix de las Pozas Alvarez

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project
#include <Updater.h>
#include <Utils.h>

// Qt
#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <QDebug>

// C++
#include <iostream>
#include <windows.h>
#include <winuser.h>

QString REQUESTS_BUFFER; // Unused

//-----------------------------------------------------------------
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  const char symbols[] =
  { 'I', 'E', '!', 'X' };
//  QString output = QString("[%1] %2 (%3:%4 -> %5)").arg( symbols[type] ).arg( msg ).arg(context.file).arg(context.line).arg(context.function);
  QString output = QString("[%1] %2").arg(symbols[type]).arg(msg);
  std::cerr << output.toStdString() << std::endl;
  if (type == QtFatalMsg) abort();
}

//-----------------------------------------------------------------
int main(int argc, char *argv[])
{
  qInstallMessageHandler(myMessageOutput);

  if (argc > 1 && strcmp(argv[1], "--log") == 0) {
  }

  QApplication app(argc, argv);

  // allow only one instance
  QSharedMemory guard;
  const QString guardKey = isPortable() ? "TrayWeatherUpdater " + QCoreApplication::applicationDirPath() : "TrayWeatherUpdater";
  guard.setKey(guardKey);

  if (!guard.create(1)) {
      QMessageBox msgBox;
      msgBox.setWindowIcon(QIcon(":/TrayWeather/updater.svg"));
      msgBox.setWindowTitle(QObject::tr("TrayWeather Updater"));
      msgBox.setIcon(QMessageBox::Warning);
      msgBox.setText(QObject::tr("TrayWeather Updater is already running!"));
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();
      std::exit(0);
  }

  if (argc != 2 ) {
      QMessageBox msgBox;
      msgBox.setWindowIcon(QIcon(":/TrayWeather/updater.svg"));
      msgBox.setWindowTitle(QObject::tr("TrayWeather Updater"));
      msgBox.setIcon(QMessageBox::Warning);
      msgBox.setText(QObject::tr("TrayWeather Updater should be called from the application."));
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();
      std::exit(0);
  }

  const QUrl url = QString::fromLocal8Bit(argv[1]);

  TrayWeatherUpdater application{url};
  application.show();

  auto resultValue = app.exec();

  qDebug() << "terminated with value" << resultValue;  

  return resultValue;
}

