/*
		File: Utils.cpp
    Created on: 20/11/2016
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
#include <Utils.h>
#include <ISO 3166-1-alpha-2.h>

// Qt
#include <QJsonArray>
#include <QIcon>
#include <QScreen>
#include <QApplication>
#include <QDialog>
#include <QSettings>
#include <QColor>
#include <QMap>

// C++
#include <functional>
#include <exception>
#include <memory>
#include <time.h>
#include <cmath>

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
static const QString TRAY_TEXT_SIZE          = QObject::tr("Tray text size");
static const QString TRAY_DYNAMIC_MIN_COLOR  = QObject::tr("Tray text color dynamic minimum");
static const QString TRAY_DYNAMIC_MAX_COLOR  = QObject::tr("Tray text color dynamic maximum");
static const QString TRAY_DYNAMIC_MIN_VALUE  = QObject::tr("Tray text color dynamic minimum value");
static const QString TRAY_DYNAMIC_MAX_VALUE  = QObject::tr("Tray text color dynamic maximum value");
static const QString UPDATE_CHECKS_FREQUENCY = QObject::tr("Update checks frequency");
static const QString UPDATE_LAST_CHECK       = QObject::tr("Update last check");
static const QString AUTOSTART               = QObject::tr("Autostart");
static const QString LAST_TAB                = QObject::tr("Last tab");
static const QString LAST_MAP_LAYER          = QObject::tr("Last map layer");
static const QString LAST_STREET_LAYER       = QObject::tr("Last street layer");

static const QMap<QString, QString> Icons = { { "01d", ":/TrayWeather/01d.svg" },
                                              { "01n-0", ":/TrayWeather/01n-0.svg" },
                                              { "01n-1", ":/TrayWeather/01n-1.svg" },
                                              { "01n-2", ":/TrayWeather/01n-2.svg" },
                                              { "01n-3", ":/TrayWeather/01n-3.svg" },
                                              { "01n-4", ":/TrayWeather/01n-4.svg" },
                                              { "01n-5", ":/TrayWeather/01n-5.svg" },
                                              { "01n-6", ":/TrayWeather/01n-6.svg" },
                                              { "01n-7", ":/TrayWeather/01n-7.svg" },
                                              { "02d", ":/TrayWeather/02d.svg" },
                                              { "02n", ":/TrayWeather/02n.svg" },
                                              { "03d", ":/TrayWeather/03.svg" },
                                              { "03n", ":/TrayWeather/03.svg" },
                                              { "04d", ":/TrayWeather/04.svg" },
                                              { "04n", ":/TrayWeather/04.svg" },
                                              { "09d", ":/TrayWeather/09.svg" },
                                              { "09n", ":/TrayWeather/09.svg" },
                                              { "10d", ":/TrayWeather/10d.svg" },
                                              { "10n", ":/TrayWeather/10n.svg" },
                                              { "11d", ":/TrayWeather/11.svg" },
                                              { "11n", ":/TrayWeather/11.svg" },
                                              { "13d", ":/TrayWeather/13.svg" },
                                              { "13n", ":/TrayWeather/13.svg" },
                                              { "50d", ":/TrayWeather/50.svg" },
                                              { "50n", ":/TrayWeather/50.svg" } };

constexpr int DEFAULT_LOGICAL_DPI = 96;

//--------------------------------------------------------------------
const double convertKelvinTo(const double temp, const Temperature units)
{
  switch(units)
  {
    case Temperature::FAHRENHEIT:
      return (temp * 9./5.) - 459.67;
      break;
    default:
    case Temperature::CELSIUS:
      return temp - 273.15;
      break;
  }

  throw std::bad_function_call();
}

//--------------------------------------------------------------------
QDebug operator <<(QDebug d, const ForecastData& data)
{
  QChar fillChar('0');
  struct tm t;
  localtime_s(&t, &data.dt);

  d << "-- Forecast " << QString("%1/%2/%3 - %4:%5:%6 --").arg(t.tm_mday, 2, 10, fillChar)
                                                          .arg(t.tm_mon + 1, 2, 10, fillChar)
                                                          .arg(t.tm_year + 1900, 4, 10, fillChar)
                                                          .arg(t.tm_hour, 2, 10, fillChar)
                                                          .arg(t.tm_min, 2, 10, fillChar)
                                                          .arg(t.tm_sec, 2, 10, fillChar) << "\n";
  d << "Name             : " << data.name << "\n";
  d << "Country          : " << data.country << "\n";
  d << "Temperature      : " << data.temp << "\n";
  d << "Temperature (min): " << data.temp_min << "\n";
  d << "Temperature (max): " << data.temp_max << "\n";
  d << "Cloudiness       : " << data.cloudiness << "\n";
  d << "Humidity         : " << data.humidity << "\n";
  d << "Ground Pressure  : " << data.pressure << "\n";
  d << "Wind Speed       : " << data.wind_speed << "\n";
  d << "Wind Direction   : " << data.wind_dir << "\n";
  d << "Snow (3 hours)   : " << data.snow << "\n";
  d << "Rain (3 hours)   : " << data.rain << "\n";
  d << "Description      : " << data.description << "\n";
  d << "Icon Id          : " << data.icon_id << "\n";
  d << "Parameters       : " << data.parameters << "\n";
  d << "Weather Id       : " << data.weather_id << "\n";

  localtime_s(&t, &data.sunrise);
  d << "Sunrise          : " << QString("%1/%2/%3 - %4:%5:%6 --").arg(t.tm_mday, 2, 10, fillChar)
                                                                 .arg(t.tm_mon + 1, 2, 10, fillChar)
                                                                 .arg(t.tm_year + 1900, 4, 10, fillChar)
                                                                 .arg(t.tm_hour, 2, 10, fillChar)
                                                                 .arg(t.tm_min, 2, 10, fillChar)
                                                                 .arg(t.tm_sec, 2, 10, fillChar) << "\n";

  localtime_s(&t, &data.sunset);
  d << "Sunset           : " << QString("%1/%2/%3 - %4:%5:%6 --").arg(t.tm_mday, 2, 10, fillChar)
                                                                 .arg(t.tm_mon + 1, 2, 10, fillChar)
                                                                 .arg(t.tm_year + 1900, 4, 10, fillChar)
                                                                 .arg(t.tm_hour, 2, 10, fillChar)
                                                                 .arg(t.tm_min, 2, 10, fillChar)
                                                                 .arg(t.tm_sec, 2, 10, fillChar) << "\n";

  return d;
}

//--------------------------------------------------------------------
void parseForecastEntry(const QJsonObject& entry, ForecastData& data, const Temperature unit)
{
  const auto main    = entry.value("main").toObject();
  const auto weather = entry.value("weather").toArray().first().toObject();
  const auto wind    = entry.value("wind").toObject();
  const auto rain    = entry.value("rain").toObject();
  const auto snow    = entry.value("snow").toObject();
  const auto sys     = entry.value("sys").toObject();

  data.dt          = entry.value("dt").toInt(0);
  data.cloudiness  = entry.value("clouds").toObject().value("all").toDouble(0);
  data.temp        = main.value("temp").toDouble(0);
  data.temp_min    = main.value("temp_min").toDouble(0);
  data.temp_max    = main.value("temp_max").toDouble(0);
  data.humidity    = main.value("humidity").toDouble(0);
  data.pressure    = main.keys().contains("grnd_level") ? main.value("grnd_level").toDouble(0) : main.value("pressure").toDouble(0);
  data.description = weather.value("description").toString();
  data.parameters  = weather.value("main").toString();
  data.icon_id     = weather.value("icon").toString();
  data.weather_id  = weather.value("id").toDouble(0);
  data.wind_speed  = wind.value("speed").toDouble(0);
  data.wind_dir    = wind.value("deg").toDouble(0);
  data.snow        = snow.keys().contains("3h") ? snow.value("3h").toDouble(0) : 0;
  data.rain        = rain.keys().contains("3h") ? rain.value("3h").toDouble(0) : 0;
  data.sunrise     = sys.value("sunrise").toInt(0);
  data.sunset      = sys.value("sunset").toInt(0);
  data.name        = entry.keys().contains("name") ? entry.value("name").toString("Unknown") : "Unknown";

  auto country = sys.keys().contains("country") ? sys.value("country").toString("") : "";
  if(!country.isEmpty())
  {
    data.country = (ISO3166.values().contains(country)) ? ISO3166.key(country) : country;
  }
  else
  {
    data.country = "Unknown";
  }
}

//--------------------------------------------------------------------
void parsePollutionEntry(const QJsonObject &entry, PollutionData &data)
{
  const auto main       = entry.value("main").toObject();
  const auto components = entry.value("components").toObject();

  data.dt    = entry.value("dt").toInt(0);
  data.aqi   = main.value("aqi").toInt(1);
  data.co    = components.value("co").toDouble(0);
  data.no    = components.value("no").toDouble(0);
  data.no2   = components.value("no2").toDouble(0);
  data.o3    = components.value("o3").toDouble(0);
  data.so2   = components.value("so2").toDouble(0);
  data.pm2_5 = components.value("pm2_5").toDouble(0);
  data.pm10  = components.value("pm10").toDouble(0);
  data.nh3   = components.value("nh3").toDouble(0);

  switch(data.aqi)
  {
    case 1:  data.aqi_text = "Good"; break;
    case 2:  data.aqi_text = "Fair"; break;
    case 3:  data.aqi_text = "Moderate"; break;
    case 4:  data.aqi_text = "Poor"; break;
    default: data.aqi_text = "Very poor"; break;
  }
}

//--------------------------------------------------------------------
void unixTimeStampToDate(struct tm &time, const long long timestamp)
{
  const time_t stamp{timestamp};

  localtime_s(&time, &stamp);
}

//--------------------------------------------------------------------
int moonPhase(const time_t timestamp, double &percent)
{
  // taken from http://www.voidware.com/moon_phase.htm
  // but modified to add illumination computation.

  /*
   calculates the moon phase (0-7), accurate to 1 segment.
   0 = > new moon.
   4 => full moon.
   */

  int c, e;
  double jd;
  int b;

  struct tm t;
  localtime_s(&t, &timestamp);

  auto d = t.tm_mday;
  auto m = t.tm_mon + 1;
  auto y = t.tm_year + 1900;

  if (m < 3)
  {
    y--;
    m += 12;
  }

  ++m;

  c = 365.25 * y;
  e = 30.6 * m;
  jd = c + e + d - 694039.09; /* jd is total days elapsed                                      */
  jd /= 29.53;                /* divide by the moon cycle (29.53 days)                         */
  b = jd;                     /* int(jd) -> b, take integer part of jd                         */
  jd -= b;                    /* subtract integer part to leave fractional part of original jd */
  b = jd * 8 + 0.5;           /* scale fraction from 0-8 and round by adding 0.5               */
  b = b & 7;                  /* 0 and 8 are the same so turn 8 into 0                         */

  if(jd < 0.5)
    percent = 2 * jd;
  else
    percent = 1.0 - 2*(jd-0.5);

  return b;
}

//--------------------------------------------------------------------
const QString moonPhaseText(const time_t timestamp, double &percent)
{
  QString result;
  auto phase = moonPhase(timestamp, percent);

  switch(phase)
  {
    case 0:
      result += QObject::tr("New moon");
      break;
    case 1:
      result += QObject::tr("Waxing crescent");
      break;
    case 2:
      result += QObject::tr("First quarter");
      break;
    case 3:
      result += QObject::tr("Waxing gibbous");
      break;
    case 4:
      result += QObject::tr("Full moon");
      break;
    case 5:
      result += QObject::tr("Waning gibbous");
      break;
    case 6:
      result += QObject::tr("Last quarter");
      break;
    case 7:
      result += QObject::tr("Waxing crescent");
      break;
  }

  return result;
}

//--------------------------------------------------------------------
const QString moonTooltip(const time_t timestamp)
{
  double percent;
  auto result = moonPhaseText(timestamp, percent);
  result += QObject::tr(" (%1% illumination)").arg(static_cast<int>(percent * 100));

  return result;
}

//--------------------------------------------------------------------
const QPixmap weatherPixmap(const ForecastData& data)
{
  QString iconId = data.icon_id;

  if(data.icon_id == "01n")
  {
    double unused;
    iconId += QObject::tr("-%1").arg(moonPhase(data.dt, unused));
  }

  return QPixmap(Icons.value(iconId));
}

//--------------------------------------------------------------------
const QPixmap moonPixmap(const ForecastData& data)
{
  if(!data.icon_id.isEmpty())
  {
    double unused;
    QString iconId{"01n"};
    iconId += QObject::tr("-%1").arg(moonPhase(data.dt, unused));

    return QPixmap(Icons.value(iconId));
  }

  return QPixmap{":/TrayWeather/network_error.svg"};
}

//--------------------------------------------------------------------
const QString toTitleCase(const QString& string)
{
  QString returnValue;

  auto parts = string.split(' ');

  for(auto part: parts)
  {
    if(!part.isEmpty())
    {
      part = part.toLower();
      part.replace(0, 1, part.at(0).toUpper());

      returnValue += part;
      if(part != parts.last()) returnValue += QString(" ");
    }
  }

  return returnValue;
}

//--------------------------------------------------------------------
const QString windDegreesToName(const double degrees)
{
  if(degrees >  11.25 && degrees <=  33.75) return "NNE";
  if(degrees >  33.75 && degrees <=  56.25) return "NE";
  if(degrees >  56.25 && degrees <=  78.75) return "ENE";
  if(degrees >  78.75 && degrees <= 101.25) return "E";
  if(degrees > 101.25 && degrees <= 123.75) return "ESE";
  if(degrees > 123.75 && degrees <= 146.25) return "SE";
  if(degrees > 146.25 && degrees <= 168.75) return "SSE";
  if(degrees > 168.75 && degrees <= 191.25) return "S";
  if(degrees > 191.25 && degrees <= 213.75) return "SSW";
  if(degrees > 213.75 && degrees <= 236.25) return "SW";
  if(degrees > 236.25 && degrees <= 258.75) return "WSW";
  if(degrees > 258.75 && degrees <= 281.25) return "W";
  if(degrees > 281.25 && degrees <= 303.75) return "WNW";
  if(degrees > 303.75 && degrees <= 326.25) return "NW";
  if(degrees > 326.25 && degrees <= 348.75) return "NNW";

  return "N"; // (348.75 to 11.25)
}

//--------------------------------------------------------------------
const QString randomString(const int length)
{
  const QString possibleCharacters("abcdefghijklmnopqrstuvwxyz0123456789");

  QString randomString;
  for (int i = 0; i < length; ++i)
  {
    randomString.append(possibleCharacters.at(qrand() % possibleCharacters.length()));
  }

  return randomString;
}

//--------------------------------------------------------------------
const QStringList parseCSV(const QString& csvText)
{
  auto text = csvText;
  auto parts = text.remove('\n').split(',', QString::SplitBehavior::KeepEmptyParts, Qt::CaseInsensitive);

  QString current;
  QStringList result;

  for(auto part: parts)
  {
    if(!current.isEmpty())
    {
      current += "," + part;
      if(current.endsWith('"'))
      {
        current = current.mid(1, current.length()-2);
        result.push_back(current);
        current.clear();
      }
    }
    else
    {
      if(part.startsWith('"'))
      {
        current = part;
      }
      else
      {
        result.push_back(part);
      }
    }
  }

  return result;
}

//--------------------------------------------------------------------
double dpiScale()
{
  auto screen = QApplication::screens().at(0);
  const auto dpi = screen->logicalDotsPerInch();

  return dpi/DEFAULT_LOGICAL_DPI;
}

//--------------------------------------------------------------------
void scaleDialog(QDialog *window)
{
  if(window)
  {
    const auto scale = (window->logicalDpiX() == DEFAULT_LOGICAL_DPI) ? 1.:1.2;

    window->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    window->setMinimumSize(0,0);

    window->adjustSize();
    window->setFixedSize(window->size()*scale);
  }
}

//--------------------------------------------------------------------
void load(Configuration &configuration)
{
  QSettings settings("Felix de las Pozas Alvarez", "TrayWeather");

  configuration.longitude       = settings.value(LONGITUDE, -181.0).toDouble();
  configuration.latitude        = settings.value(LATITUDE, -91.0).toDouble();
  configuration.country         = settings.value(COUNTRY, QString()).toString();
  configuration.region          = settings.value(REGION, QString()).toString();
  configuration.city            = settings.value(CITY, QString()).toString();
  configuration.isp             = settings.value(ISP, QString()).toString();
  configuration.ip              = settings.value(IP, QString()).toString();
  configuration.timezone        = settings.value(TIMEZONE, QString()).toString();
  configuration.zipcode         = settings.value(ZIPCODE, QString()).toString();
  configuration.owm_apikey      = settings.value(OPENWEATHERMAP_APIKEY, QString()).toString();
  configuration.units           = static_cast<Temperature>(settings.value(TEMP_UNITS, 0).toInt());
  configuration.updateTime      = settings.value(UPDATE_INTERVAL, 15).toUInt();
  configuration.mapsEnabled     = settings.value(MAPS_TAB_ENABLED, true).toBool();
  configuration.useDNS          = settings.value(USE_DNS_GEOLOCATION, false).toBool();
  configuration.useGeolocation  = settings.value(USE_GEOLOCATION_SERVICE, true).toBool();
  configuration.roamingEnabled  = settings.value(ROAMING_ENABLED, false).toBool();
  configuration.lightTheme      = settings.value(THEME, true).toBool();
  configuration.iconType        = settings.value(TRAY_ICON_TYPE, 0).toUInt();
  configuration.trayTextColor   = QColor(settings.value(TRAY_TEXT_COLOR, "#FFFFFFFF").toString());
  configuration.trayTextMode    = settings.value(TRAY_TEXT_COLOR_MODE, true).toBool();
  configuration.trayTextSize    = settings.value(TRAY_TEXT_SIZE, 250).toUInt();
  configuration.minimumColor    = QColor(settings.value(TRAY_DYNAMIC_MIN_COLOR, "#FF0000FF").toString());
  configuration.maximumColor    = QColor(settings.value(TRAY_DYNAMIC_MAX_COLOR, "#FFFF0000").toString());
  configuration.minimumValue    = settings.value(TRAY_DYNAMIC_MIN_VALUE, -15).toInt();
  configuration.maximumValue    = settings.value(TRAY_DYNAMIC_MAX_VALUE, 45).toInt();
  configuration.update          = static_cast<Update>(settings.value(UPDATE_CHECKS_FREQUENCY, 2).toInt());
  configuration.lastCheck       = settings.value(UPDATE_LAST_CHECK, QDateTime()).toDateTime();
  configuration.autostart       = settings.value(AUTOSTART, false).toBool();
  configuration.lastTab         = settings.value(LAST_TAB, 0).toInt();
  configuration.lastLayer       = settings.value(LAST_MAP_LAYER, "temperature").toString();
  configuration.lastStreetLayer = settings.value(LAST_STREET_LAYER, "mapnik").toString();

  if(!MAP_LAYERS.contains(configuration.lastLayer, Qt::CaseSensitive))       configuration.lastLayer == MAP_LAYERS.first();
  if(!MAP_STREET.contains(configuration.lastStreetLayer, Qt::CaseSensitive)) configuration.lastStreetLayer == MAP_STREET.first();
}

//--------------------------------------------------------------------
void save(const Configuration &configuration)
{
  QSettings settings("Felix de las Pozas Alvarez", "TrayWeather");

  if(!MAP_LAYERS.contains(configuration.lastLayer, Qt::CaseSensitive))       configuration.lastLayer == MAP_LAYERS.first();
  if(!MAP_STREET.contains(configuration.lastStreetLayer, Qt::CaseSensitive)) configuration.lastStreetLayer == MAP_STREET.first();

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
  settings.setValue(TRAY_TEXT_SIZE,          configuration.trayTextSize);
  settings.setValue(TRAY_DYNAMIC_MIN_COLOR,  configuration.minimumColor.name(QColor::HexArgb));
  settings.setValue(TRAY_DYNAMIC_MAX_COLOR,  configuration.maximumColor.name(QColor::HexArgb));
  settings.setValue(TRAY_DYNAMIC_MIN_VALUE,  configuration.minimumValue);
  settings.setValue(TRAY_DYNAMIC_MAX_VALUE,  configuration.maximumValue);
  settings.setValue(UPDATE_CHECKS_FREQUENCY, static_cast<int>(configuration.update));
  settings.setValue(UPDATE_LAST_CHECK,       configuration.lastCheck);
  settings.setValue(AUTOSTART,               configuration.autostart);
  settings.setValue(LAST_TAB,                configuration.lastTab);
  settings.setValue(LAST_MAP_LAYER,          configuration.lastLayer);
  settings.setValue(LAST_STREET_LAYER,       configuration.lastStreetLayer);

  settings.sync();
}
