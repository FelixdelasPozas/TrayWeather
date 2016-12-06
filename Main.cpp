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
#include <QSettings>
#include <QDebug>

// C++
#include <iostream>

static const QString LONGITUDE             = QObject::tr("Longitude");
static const QString LATITUDE              = QObject::tr("Latitude");
static const QString COUNTRY               = QObject::tr("Country");
static const QString REGION                = QObject::tr("Region");
static const QString CITY                  = QObject::tr("City");
static const QString ISP                   = QObject::tr("Service provider");
static const QString IP                    = QObject::tr("IP Address");
static const QString TIMEZONE              = QObject::tr("Timezone");
static const QString ZIPCODE               = QObject::tr("Zipcode");
static const QString OPENWEATHERMAP_APIKEY = QObject::tr("OpenWeatherMap API Key");
static const QString TEMP_UNITS            = QObject::tr("Units");
static const QString UPDATE_INTERVAL       = QObject::tr("Update interval");
static const QString MAPS_TAB_ENABLED      = QObject::tr("Maps tab enabled");

//--------------------------------------------------------------------
void saveConfiguration(const Configuration &configuration)
{
  QSettings settings("TrayWeather.ini", QSettings::IniFormat);

  settings.setValue(LONGITUDE,             configuration.longitude);
  settings.setValue(LATITUDE,              configuration.latitude);
  settings.setValue(COUNTRY,               configuration.country);
  settings.setValue(REGION,                configuration.region);
  settings.setValue(CITY,                  configuration.city);
  settings.setValue(ISP,                   configuration.isp);
  settings.setValue(IP,                    configuration.ip);
  settings.setValue(TIMEZONE,              configuration.timezone);
  settings.setValue(ZIPCODE,               configuration.zipcode);
  settings.setValue(OPENWEATHERMAP_APIKEY, configuration.owm_apikey);
  settings.setValue(TEMP_UNITS,            static_cast<int>(configuration.units));
  settings.setValue(UPDATE_INTERVAL,       configuration.updateTime);
  settings.setValue(MAPS_TAB_ENABLED,      configuration.mapsEnabled);

  settings.sync();
}

//--------------------------------------------------------------------
void loadConfiguration(Configuration &configuration)
{
  QSettings settings("TrayWeather.ini", QSettings::IniFormat);

  configuration.longitude   = settings.value(LONGITUDE, -181.0).toDouble();
  configuration.latitude    = settings.value(LATITUDE, -181.0).toDouble();
  configuration.country     = settings.value(COUNTRY, QString()).toString();
  configuration.region      = settings.value(REGION, QString()).toString();
  configuration.city        = settings.value(CITY, QString()).toString();
  configuration.isp         = settings.value(ISP, QString()).toString();
  configuration.ip          = settings.value(IP, QString()).toString();
  configuration.timezone    = settings.value(TIMEZONE, QString()).toString();
  configuration.zipcode     = settings.value(ZIPCODE, QString()).toString();
  configuration.owm_apikey  = settings.value(OPENWEATHERMAP_APIKEY, QString()).toString();
  configuration.units       = static_cast<Temperature>(settings.value(TEMP_UNITS, 0).toInt());
  configuration.updateTime  = settings.value(UPDATE_INTERVAL, 15).toUInt();
  configuration.mapsEnabled = settings.value(MAPS_TAB_ENABLED, true).toBool();
}

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

  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);

  if(!QSystemTrayIcon::isSystemTrayAvailable())
  {
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/TrayWeather/application.ico"));
    msgBox.setWindowTitle(QObject::tr("Tray Weather"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("TrayWeather cannot execute in this computer because there isn't a tray available!.\nThe application will exit now.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    std::exit(0);
  }

  // allow only one instance
  QSharedMemory guard;
  guard.setKey("TrayWeather");

  if (!guard.create(1))
  {
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/TrayWeather/application.ico"));
    msgBox.setWindowTitle(QObject::tr("Tray Weather"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("TrayWeather is already running!");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    std::exit(0);
  }

  Configuration configuration;
  loadConfiguration(configuration);

  if(!configuration.isValid())
  {
    ConfigurationDialog dialog(configuration);
    dialog.exec();

    dialog.getConfiguration(configuration);

    if(configuration.isValid())
    {
      saveConfiguration(configuration);
    }
    else
    {
      QMessageBox msgBox;
      msgBox.setWindowIcon(QIcon(":/TrayWeather/application.ico"));
      msgBox.setWindowTitle(QObject::tr("Tray Weather"));
      msgBox.setIcon(QMessageBox::Warning);
      msgBox.setText("TrayWeather cannot execute without a valid location and a valid OpenWeatherMap API Key.\nThe application will exit now.");
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();
      std::exit(0);
    }
  }

  TrayWeather application{configuration};
  application.show();

  auto resultValue = app.exec();

  qDebug() << "terminated with value" << resultValue;

  saveConfiguration(configuration);

  return resultValue;
}

