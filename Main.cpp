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
#include <QFile>
#include <QTextStream>
#include <QColor>
#include <QDebug>

// C++
#include <iostream>

static const QString LONGITUDE               = QObject::tr("Longitude");
static const QString LATITUDE                = QObject::tr("Latitude");
static const QString COUNTRY                 = QObject::tr("Country");
static const QString REGION                  = QObject::tr("Region");
static const QString CITY                    = QObject::tr("City");
static const QString ISP                     = QObject::tr("Service provider");
static const QString IP                      = QObject::tr("IP Address");
static const QString TIMEZONE                = QObject::tr("Timezone");
static const QString ZIPCODE                 = QObject::tr("Zipcode");
static const QString OPENWEATHERMAP_APIKEY   = QObject::tr("OpenWeatherMap API Key");
static const QString TEMP_UNITS              = QObject::tr("Units");
static const QString UPDATE_INTERVAL         = QObject::tr("Update interval");
static const QString MAPS_TAB_ENABLED        = QObject::tr("Maps tab enabled");
static const QString USE_DNS_GEOLOCATION     = QObject::tr("Use DNS Geolocation");
static const QString USE_GEOLOCATION_SERVICE = QObject::tr("Use ip-api.com services");
static const QString ROAMING_ENABLED         = QObject::tr("Roaming enabled");
static const QString THEME                   = QObject::tr("Light theme used");
static const QString TRAY_ICON_TYPE          = QObject::tr("Tray icon type");
static const QString TRAY_TEXT_COLOR         = QObject::tr("Tray text color");
static const QString TRAY_TEXT_COLOR_MODE    = QObject::tr("Tray text color mode");
static const QString TRAY_DYNAMIC_MIN_COLOR  = QObject::tr("Tray text color dynamic minimum");
static const QString TRAY_DYNAMIC_MAX_COLOR  = QObject::tr("Tray text color dynamic maximum");
static const QString TRAY_DYNAMIC_MIN_VALUE  = QObject::tr("Tray text color dynamic minimum value");
static const QString TRAY_DYNAMIC_MAX_VALUE  = QObject::tr("Tray text color dynamic maximum value");

//--------------------------------------------------------------------
void saveConfiguration(const Configuration &configuration)
{
  QSettings settings("Felix de las Pozas Alvarez", "TrayWeather");

  settings.setValue(LONGITUDE,               configuration.longitude);
  settings.setValue(LATITUDE,                configuration.latitude);
  settings.setValue(COUNTRY,                 configuration.country);
  settings.setValue(REGION,                  configuration.region);
  settings.setValue(CITY,                    configuration.city);
  settings.setValue(ISP,                     configuration.isp);
  settings.setValue(IP,                      configuration.ip);
  settings.setValue(TIMEZONE,                configuration.timezone);
  settings.setValue(ZIPCODE,                 configuration.zipcode);
  settings.setValue(OPENWEATHERMAP_APIKEY,   configuration.owm_apikey);
  settings.setValue(TEMP_UNITS,              static_cast<int>(configuration.units));
  settings.setValue(UPDATE_INTERVAL,         configuration.updateTime);
  settings.setValue(MAPS_TAB_ENABLED,        configuration.mapsEnabled);
  settings.setValue(USE_DNS_GEOLOCATION,     configuration.useDNS);
  settings.setValue(USE_GEOLOCATION_SERVICE, configuration.useGeolocation);
  settings.setValue(ROAMING_ENABLED,         configuration.roamingEnabled);
  settings.setValue(THEME,                   configuration.lightTheme);
  settings.setValue(TRAY_ICON_TYPE,          configuration.iconType);
  settings.setValue(TRAY_TEXT_COLOR,         configuration.trayTextColor.name(QColor::HexArgb));
  settings.setValue(TRAY_TEXT_COLOR_MODE,    configuration.trayTextMode);
  settings.setValue(TRAY_DYNAMIC_MIN_COLOR,  configuration.minimumColor.name(QColor::HexArgb));
  settings.setValue(TRAY_DYNAMIC_MAX_COLOR,  configuration.maximumColor.name(QColor::HexArgb));
  settings.setValue(TRAY_DYNAMIC_MIN_VALUE,  configuration.minimumValue);
  settings.setValue(TRAY_DYNAMIC_MAX_VALUE,  configuration.maximumValue);

  settings.sync();
}

//--------------------------------------------------------------------
void loadConfiguration(Configuration &configuration)
{
  QSettings settings("Felix de las Pozas Alvarez", "TrayWeather");

  configuration.longitude      = settings.value(LONGITUDE, -181.0).toDouble();
  configuration.latitude       = settings.value(LATITUDE, -91.0).toDouble();
  configuration.country        = settings.value(COUNTRY, QString()).toString();
  configuration.region         = settings.value(REGION, QString()).toString();
  configuration.city           = settings.value(CITY, QString()).toString();
  configuration.isp            = settings.value(ISP, QString()).toString();
  configuration.ip             = settings.value(IP, QString()).toString();
  configuration.timezone       = settings.value(TIMEZONE, QString()).toString();
  configuration.zipcode        = settings.value(ZIPCODE, QString()).toString();
  configuration.owm_apikey     = settings.value(OPENWEATHERMAP_APIKEY, QString()).toString();
  configuration.units          = static_cast<Temperature>(settings.value(TEMP_UNITS, 0).toInt());
  configuration.updateTime     = settings.value(UPDATE_INTERVAL, 15).toUInt();
  configuration.mapsEnabled    = settings.value(MAPS_TAB_ENABLED, true).toBool();
  configuration.useDNS         = settings.value(USE_DNS_GEOLOCATION, false).toBool();
  configuration.useGeolocation = settings.value(USE_GEOLOCATION_SERVICE, true).toBool();
  configuration.roamingEnabled = settings.value(ROAMING_ENABLED, false).toBool();
  configuration.lightTheme     = settings.value(THEME, true).toBool();
  configuration.iconType       = settings.value(TRAY_ICON_TYPE, 0).toUInt();
  configuration.trayTextColor  = QColor(settings.value(TRAY_TEXT_COLOR, "#FFFFFFFF").toString());
  configuration.trayTextMode   = settings.value(TRAY_TEXT_COLOR_MODE, true).toBool();
  configuration.minimumColor   = QColor(settings.value(TRAY_DYNAMIC_MIN_COLOR, "#FF0000FF").toString());
  configuration.maximumColor   = QColor(settings.value(TRAY_DYNAMIC_MAX_COLOR, "#FFFF0000").toString());
  configuration.minimumValue   = settings.value(TRAY_DYNAMIC_MIN_VALUE, -15).toInt();
  configuration.maximumValue   = settings.value(TRAY_DYNAMIC_MAX_VALUE, 45).toInt();
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

  QString sheet;

  if(!configuration.lightTheme)
  {
    QFile file(":qdarkstyle/style.qss");
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts(&file);
    sheet = ts.readAll();
  }

  qApp->setStyleSheet(sheet);

  TrayWeather application{configuration};
  application.show();

  auto resultValue = app.exec();

  qDebug() << "terminated with value" << resultValue;

  saveConfiguration(configuration);

  return resultValue;
}

