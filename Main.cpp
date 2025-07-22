/*
 File: Main.cpp
 Created on: 12/11/2016
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
#include <ConfigurationDialog.h>
#include <TrayWeather.h>
#include <Utils.h>

// Qt
#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include <QDebug>

// C++
#include <iostream>

QString REQUESTS_BUFFER;

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

  // To fix networking problems as Qt looks for updated networks every 10 seconds...
  // https://bugreports.qt.io/browse/QTBUG-46015
  // WARNING: This could break wifi detection
  const QByteArray ROAMING_POLL_VALUE = getRoamingRegistryValue() ? QByteArray::number(30000) : QByteArray::number(-1);
  qputenv("QT_BEARER_POLL_TIMEOUT", ROAMING_POLL_VALUE);

  // For debugging purposes.
  if(argc > 1 && strcmp(argv[1], "--log") == 0)
    NetworkAccessManager::LOG_REQUESTS = true;

  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);

  Configuration configuration;
  load(configuration);

  changeLanguage(configuration.language);

  if(!QSystemTrayIcon::isSystemTrayAvailable())
  {
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    msgBox.setWindowTitle(QObject::tr("Tray Weather"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(QObject::tr("TrayWeather cannot execute in this computer because there isn't a tray available!.\nThe application will exit now."));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    std::exit(0);
  }

  // allow only one instance
  QSharedMemory guard;
  const QString guardKey = isPortable() ? QCoreApplication::applicationDirPath() : "TrayWeather";
  guard.setKey(guardKey);

  if (!guard.create(1))
  {
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    msgBox.setWindowTitle(QObject::tr("Tray Weather"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(QObject::tr("TrayWeather is already running!"));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    std::exit(0);
  }

  if(!configuration.isValid())
  {
    ConfigurationDialog dialog(configuration);

    QObject::connect(&dialog, &ConfigurationDialog::languageChanged, []( const QString &lang ) { changeLanguage(lang); });

    const auto value = dialog.exec();

    dialog.getConfiguration(configuration);

    if(configuration.isValid() && value == QDialog::Accepted)
    {
      save(configuration);
    }
    else
    {
      QMessageBox msgBox;
      msgBox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
      msgBox.setWindowTitle(QObject::tr("Tray Weather"));
      msgBox.setIcon(QMessageBox::Warning);
      msgBox.setText(QObject::tr("TrayWeather cannot execute without a valid location and a valid weather data provider.\nThe application will exit now."));
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();
      std::exit(0);
    }
  }

  QString sheet;

  if(!configuration.lightTheme)
  {
    QFile file(":qdarkstyle/style.qss");
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts(&file);
    sheet = ts.readAll();
  }

  qApp->setStyleSheet(sheet);
  QObject::connect(qApp, &QCoreApplication::aboutToQuit, [&configuration](){ save(configuration); });

  TrayWeather application{configuration};
  application.show();

  auto resultValue = app.exec();

  qDebug() << "terminated with value" << resultValue;

  return resultValue;
}

