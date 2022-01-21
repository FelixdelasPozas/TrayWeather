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
#include <QApplication>
#include <QTextDocument>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QRgb>
#include <QLayout>
#include <QObject>

// C++
#include <functional>
#include <exception>
#include <memory>
#include <cmath>
#include <windows.h>
#include <iostream>

static const QString LONGITUDE               = QString("Longitude");
static const QString LATITUDE                = QString("Latitude");
static const QString COUNTRY                 = QString("Country");
static const QString REGION                  = QString("Region");
static const QString CITY                    = QString("City");
static const QString ISP                     = QString("Service provider");
static const QString IP                      = QString("IP Address");
static const QString TIMEZONE                = QString("Timezone");
static const QString ZIPCODE                 = QString("Zipcode");
static const QString OPENWEATHERMAP_APIKEY   = QString("OpenWeatherMap API Key");
static const QString UNITS                   = QString("Units");
static const QString UPDATE_INTERVAL         = QString("Update interval");
static const QString MAPS_TAB_ENABLED        = QString("Maps tab enabled");
static const QString USE_DNS_GEOLOCATION     = QString("Use DNS Geolocation");
static const QString USE_GEOLOCATION_SERVICE = QString("Use ip-api.com services");
static const QString ROAMING_ENABLED         = QString("Roaming enabled");
static const QString THEME                   = QString("Light theme used");
static const QString TRAY_ICON_TYPE          = QString("Tray icon type");
static const QString TRAY_ICON_THEME         = QString("Tray icon theme");
static const QString TRAY_ICON_THEME_COLOR   = QString("Tray icon theme color");
static const QString TRAY_TEXT_COLOR         = QString("Tray text color");
static const QString TRAY_TEXT_COLOR_MODE    = QString("Tray text color mode");
static const QString TRAY_TEXT_BORDER        = QString("Tray text border");
static const QString TRAY_TEXT_FONT          = QString("Tray text font");
static const QString TRAY_DYNAMIC_MIN_COLOR  = QString("Tray text color dynamic minimum");
static const QString TRAY_DYNAMIC_MAX_COLOR  = QString("Tray text color dynamic maximum");
static const QString TRAY_DYNAMIC_MIN_VALUE  = QString("Tray text color dynamic minimum value");
static const QString TRAY_DYNAMIC_MAX_VALUE  = QString("Tray text color dynamic maximum value");
static const QString UPDATE_CHECKS_FREQUENCY = QString("Update checks frequency");
static const QString UPDATE_LAST_CHECK       = QString("Update last check");
static const QString AUTOSTART               = QString("Autostart");
static const QString LAST_TAB                = QString("Last tab");
static const QString LAST_MAP_LAYER          = QString("Last map layer");
static const QString LAST_STREET_LAYER       = QString("Last street layer");
static const QString LANGUAGE                = QString("Language");
static const QString CUSTOM_TEMP_UNITS       = QString("Custom temperature units");
static const QString CUSTOM_PRES_UNITS       = QString("Custom pressure units");
static const QString CUSTOM_PREC_UNITS       = QString("Custom precipitation units");
static const QString CUSTOM_WIND_UNITS       = QString("Custom wind units");
static const QString TOOLTIP_FIELDS          = QString("Tooltip text fields");
static const QString GRAPH_USE_RAIN          = QString("Forecast graph uses rain data");
static const QString SHOW_ALERTS             = QString("Show weather alerts");
static const QString STRETCH_TEMP_ICON       = QString("Stretch temperature icon vertically");

static const QMap<QString, QString> ICONS = { { "01d", ":/TrayWeather/iconThemes/%1/01d.svg" },
                                              { "01n-0", ":/TrayWeather/iconThemes/%1/01n-0.svg" },
                                              { "01n-1", ":/TrayWeather/iconThemes/%1/01n-1.svg" },
                                              { "01n-2", ":/TrayWeather/iconThemes/%1/01n-2.svg" },
                                              { "01n-3", ":/TrayWeather/iconThemes/%1/01n-3.svg" },
                                              { "01n-4", ":/TrayWeather/iconThemes/%1/01n-4.svg" },
                                              { "01n-5", ":/TrayWeather/iconThemes/%1/01n-5.svg" },
                                              { "01n-6", ":/TrayWeather/iconThemes/%1/01n-6.svg" },
                                              { "01n-7", ":/TrayWeather/iconThemes/%1/01n-7.svg" },
                                              { "02d", ":/TrayWeather/iconThemes/%1/02d.svg" },
                                              { "02n", ":/TrayWeather/iconThemes/%1/02n.svg" },
                                              { "03d", ":/TrayWeather/iconThemes/%1/03d.svg" },
                                              { "03n", ":/TrayWeather/iconThemes/%1/03n.svg" },
                                              { "04d", ":/TrayWeather/iconThemes/%1/04d.svg" },
                                              { "04n", ":/TrayWeather/iconThemes/%1/04n.svg" },
                                              { "09d", ":/TrayWeather/iconThemes/%1/09d.svg" },
                                              { "09n", ":/TrayWeather/iconThemes/%1/09n.svg" },
                                              { "10d", ":/TrayWeather/iconThemes/%1/10d.svg" },
                                              { "10n", ":/TrayWeather/iconThemes/%1/10n.svg" },
                                              { "11d", ":/TrayWeather/iconThemes/%1/11d.svg" },
                                              { "11n", ":/TrayWeather/iconThemes/%1/11n.svg" },
                                              { "13d", ":/TrayWeather/iconThemes/%1/13d.svg" },
                                              { "13n", ":/TrayWeather/iconThemes/%1/13n.svg" },
                                              { "50d", ":/TrayWeather/iconThemes/%1/50d.svg" },
                                              { "50n", ":/TrayWeather/iconThemes/%1/50n.svg" } };

constexpr int DEFAULT_LOGICAL_DPI = 96;

//--------------------------------------------------------------------
const double convertMmToInches(const double value)
{
  // return only 2 digits.
  return static_cast<int>(value * 0.0393701 * 100)/100.;
}

//--------------------------------------------------------------------
const double convertCelsiusToFahrenheit(const double value)
{
  // return only 2 digits.
  return static_cast<int>(((value * 9.)/5.)*100)/100. + 32;
}

//--------------------------------------------------------------------
const double convertMetersSecondToMilesHour(const double value)
{
  // return only 2 digits.
  return static_cast<int>(value * 2.23694 * 100)/100.;
}

//--------------------------------------------------------------------
const double convertMetersSecondToKilometersHour(const double value)
{
  // return only 2 digits.
  return static_cast<int>(value * 3.6 * 100)/100.;
}

//--------------------------------------------------------------------
const double convertMetersSecondToFeetSecond(const double value)
{
  // return only 2 digits.
  return static_cast<int>(value * 3.28084 * 100)/100.;
}

//--------------------------------------------------------------------
const double converthPaToPSI(const double value)
{
  // return only 2 digits.
  return static_cast<int>(value * 0.014503773773 * 100)/100.;
}

//--------------------------------------------------------------------
const double converthPaTommHg(const double value)
{
  // return only 2 digits.
  return static_cast<int>(value * 0.750064 * 100)/100.;
}

//--------------------------------------------------------------------
const double converthPaToinHg(const double value)
{
  // return only 2 digits.
  return static_cast<int>(value * 0.02953 * 100)/100.;
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
void parseForecastEntry(const QJsonObject& entry, ForecastData& data)
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
  const auto illuStr = QObject::tr("illumination");

  double percent;
  auto result = moonPhaseText(timestamp, percent);

  result += QString(" (%1% %2)").arg(static_cast<int>(percent * 100)).arg(illuStr);

  return result;
}

//--------------------------------------------------------------------
void paintPixmap(QImage &img, const QColor &color)
{
  auto ptr = reinterpret_cast<QRgb *>(img.bits());
  const auto transparentPixel = QColor{Qt::transparent}.rgba();

  for(long int i = 0; i < img.byteCount()/4; ++i)
  {
    // Need to make white transparent to paint "monocolor" images (black usually).
    if((qAlpha(*ptr) != 0) && (qRed(*ptr) == 0xFF) && (qGreen(*ptr) == 0xFF) && (qBlue(*ptr) == 0xFF))
    {
      *ptr = transparentPixel;
    }

    ++ptr;
  }

  QPainter painter(&img);
  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.fillRect(img.rect(), color);
  painter.end();
}

//--------------------------------------------------------------------
const QPixmap weatherPixmap(const ForecastData& data, const unsigned int theme, const QColor &color)
{
  if(!data.icon_id.isEmpty())
  {
    QString iconId = data.icon_id;

    if(data.icon_id == "01n")
    {
      double unused;
      iconId += QString("-%1").arg(moonPhase(data.dt, unused));
    }

    auto pix = QPixmap(ICONS.value(iconId).arg(ICON_THEMES.at(theme).id)).toImage();

    if(!ICON_THEMES.at(theme).colored)
    {
      paintPixmap(pix, color);
    }

    return QPixmap::fromImage(pix);
  }

  return QPixmap{":/TrayWeather/network_error.svg"};
}

//--------------------------------------------------------------------
const QPixmap weatherPixmap(const QString &iconId, const unsigned int theme, const QColor &color)
{
  if(!iconId.isEmpty())
  {
    QImage pix;

    if(iconId == "01n")
    {
      pix = QPixmap(ICONS.value("01n-1").arg(ICON_THEMES.at(theme).id)).toImage();
    }
    else
    {
      pix = QPixmap(ICONS.value(iconId).arg(ICON_THEMES.at(theme).id)).toImage();
    }

    if(!ICON_THEMES.at(theme).colored)
    {
      paintPixmap(pix, color);
    }

    return QPixmap::fromImage(pix);
  }

  return QPixmap{":/TrayWeather/network_error.svg"};
}

//--------------------------------------------------------------------
const QPixmap moonPixmap(const ForecastData& data, const unsigned int theme, const QColor &color)
{
  if(!data.icon_id.isEmpty())
  {
    double unused;
    QString iconId{"01n"};
    iconId += QString("-%1").arg(moonPhase(data.dt, unused));

    auto pix = QPixmap(ICONS.value(iconId).arg(ICON_THEMES.at(theme).id)).toImage();

    if(!ICON_THEMES.at(theme).colored)
    {
      paintPixmap(pix, color);
    }

    return QPixmap::fromImage(pix);
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
  if(degrees >  11.25 && degrees <=  33.75) return QObject::tr("NNE");
  if(degrees >  33.75 && degrees <=  56.25) return QObject::tr("NE");
  if(degrees >  56.25 && degrees <=  78.75) return QObject::tr("ENE");
  if(degrees >  78.75 && degrees <= 101.25) return QObject::tr("E");
  if(degrees > 101.25 && degrees <= 123.75) return QObject::tr("ESE");
  if(degrees > 123.75 && degrees <= 146.25) return QObject::tr("SE");
  if(degrees > 146.25 && degrees <= 168.75) return QObject::tr("SSE");
  if(degrees > 168.75 && degrees <= 191.25) return QObject::tr("S");
  if(degrees > 191.25 && degrees <= 213.75) return QObject::tr("SSW");
  if(degrees > 213.75 && degrees <= 236.25) return QObject::tr("SW");
  if(degrees > 236.25 && degrees <= 258.75) return QObject::tr("WSW");
  if(degrees > 258.75 && degrees <= 281.25) return QObject::tr("W");
  if(degrees > 281.25 && degrees <= 303.75) return QObject::tr("WNW");
  if(degrees > 303.75 && degrees <= 326.25) return QObject::tr("NW");
  if(degrees > 326.25 && degrees <= 348.75) return QObject::tr("NNW");

  return QObject::tr("N"); // (348.75 to 11.25)
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
  const auto units = settings.value(UNITS, 0).toInt();
  configuration.units           = static_cast<Units>(units);
  configuration.updateTime      = settings.value(UPDATE_INTERVAL, 15).toUInt();
  configuration.mapsEnabled     = settings.value(MAPS_TAB_ENABLED, true).toBool();
  configuration.useDNS          = settings.value(USE_DNS_GEOLOCATION, false).toBool();
  configuration.useGeolocation  = settings.value(USE_GEOLOCATION_SERVICE, true).toBool();
  configuration.roamingEnabled  = settings.value(ROAMING_ENABLED, false).toBool();
  configuration.lightTheme      = settings.value(THEME, true).toBool();
  configuration.iconType        = settings.value(TRAY_ICON_TYPE, 0).toUInt();
  configuration.iconTheme       = settings.value(TRAY_ICON_THEME, 0).toUInt();
  configuration.iconThemeColor  = QColor(settings.value(TRAY_ICON_THEME_COLOR, "#FF000000").toString());
  configuration.trayTextColor   = QColor(settings.value(TRAY_TEXT_COLOR, "#FFFFFFFF").toString());
  configuration.trayTextMode    = settings.value(TRAY_TEXT_COLOR_MODE, true).toBool();
  configuration.trayTextBorder  = settings.value(TRAY_TEXT_BORDER, true).toBool();
  configuration.trayTextFont    = settings.value(TRAY_TEXT_FONT, QString()).toString();
  configuration.stretchTempIcon = settings.value(STRETCH_TEMP_ICON, false).toBool();
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
  configuration.language        = settings.value(LANGUAGE, "en_EN").toString();
  configuration.graphUseRain    = settings.value(GRAPH_USE_RAIN, true).toBool();
  configuration.showAlerts      = settings.value(SHOW_ALERTS, true).toBool();

  // if CUSTOM_UNITS values doesn't exists (first run) use units value.
  configuration.tempUnits        = static_cast<TemperatureUnits>(settings.value(CUSTOM_TEMP_UNITS, units).toInt());
  configuration.pressureUnits    = static_cast<PressureUnits>(settings.value(CUSTOM_PRES_UNITS, units).toInt());
  configuration.precUnits        = static_cast<PrecipitationUnits>(settings.value(CUSTOM_PREC_UNITS, units).toInt());
  configuration.windUnits        = static_cast<WindUnits>(settings.value(CUSTOM_WIND_UNITS, units).toInt());

  configuration.tooltipFields.clear();
  const auto fields = settings.value(TOOLTIP_FIELDS, "0,1,2").toString().split(',');
  bool ok = false;
  for(int i = 0; i < fields.size(); ++i)
  {
    auto number = fields.at(i).toInt(&ok);
    if(!ok) continue;
    const auto field = static_cast<TooltipText>(number);
    if(!configuration.tooltipFields.contains(field))
    {
      configuration.tooltipFields << field;
    }
  }

  if(!MAP_LAYERS.contains(configuration.lastLayer, Qt::CaseSensitive))       configuration.lastLayer == MAP_LAYERS.first();
  if(!MAP_STREET.contains(configuration.lastStreetLayer, Qt::CaseSensitive)) configuration.lastStreetLayer == MAP_STREET.first();

  if(configuration.trayTextFont.isEmpty())
  {
    QFont font;
    font.setFamily(font.defaultFamily());
    configuration.trayTextFont = font.toString();
  }
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
  settings.setValue(UNITS,                   static_cast<int>(configuration.units));
  settings.setValue(UPDATE_INTERVAL,         configuration.updateTime);
  settings.setValue(MAPS_TAB_ENABLED,        configuration.mapsEnabled);
  settings.setValue(USE_DNS_GEOLOCATION,     configuration.useDNS);
  settings.setValue(USE_GEOLOCATION_SERVICE, configuration.useGeolocation);
  settings.setValue(ROAMING_ENABLED,         configuration.roamingEnabled);
  settings.setValue(THEME,                   configuration.lightTheme);
  settings.setValue(TRAY_ICON_TYPE,          configuration.iconType);
  settings.setValue(TRAY_ICON_THEME,         configuration.iconTheme);
  settings.setValue(TRAY_ICON_THEME_COLOR,   configuration.iconThemeColor.name(QColor::HexArgb));
  settings.setValue(TRAY_TEXT_COLOR,         configuration.trayTextColor.name(QColor::HexArgb));
  settings.setValue(TRAY_TEXT_COLOR_MODE,    configuration.trayTextMode);
  settings.setValue(TRAY_TEXT_BORDER,        configuration.trayTextBorder);
  settings.setValue(TRAY_TEXT_FONT,          configuration.trayTextFont);
  settings.setValue(STRETCH_TEMP_ICON,       configuration.stretchTempIcon);
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
  settings.setValue(LANGUAGE,                configuration.language);
  settings.setValue(CUSTOM_TEMP_UNITS,       static_cast<int>(configuration.tempUnits));
  settings.setValue(CUSTOM_PRES_UNITS,       static_cast<int>(configuration.pressureUnits));
  settings.setValue(CUSTOM_PREC_UNITS,       static_cast<int>(configuration.precUnits));
  settings.setValue(CUSTOM_WIND_UNITS,       static_cast<int>(configuration.windUnits));
  settings.setValue(GRAPH_USE_RAIN,          configuration.graphUseRain);
  settings.setValue(SHOW_ALERTS,             configuration.showAlerts);

  QStringList fieldList;
  QList<int> fieldNums;
  for(int i = 0; i < configuration.tooltipFields.size(); ++i)
  {
    const auto field = configuration.tooltipFields.at(i);
    const auto number = static_cast<int>(field);
    if(!fieldNums.contains(number))
    {
      fieldNums << number;
      fieldList << QString("%1").arg(number);
    }
  }
  settings.setValue(TOOLTIP_FIELDS, fieldList.join(","));

  settings.sync();
}

//--------------------------------------------------------------------
QColor uvColor(const double uvIndex)
{
  const int value = static_cast<int>(std::nearbyint(uvIndex));

  QColor gradientColor;
  switch(value)
  {
    case 0:
      gradientColor = QColor("#FFFFFF");
      break;
    case 1:
      gradientColor = QColor("#4EB400");
      break;
    case 2:
      gradientColor = QColor("#A0CE00");
      break;
    case 3:
      gradientColor = QColor("#F7E400");
      break;
    case 4:
      gradientColor = QColor("#F8B600");
      break;
    case 5:
      gradientColor = QColor("#F88700");
      break;
    case 6:
      gradientColor = QColor("#F85900");
      break;
    case 7:
      gradientColor = QColor("#E82C0E");
      break;
    case 8:
      gradientColor = QColor("#D8001D");
      break;
    case 9:
      gradientColor = QColor("#FF0099");
      break;
    case 10:
      gradientColor = QColor("#B54CFF");
      break;
    default:
      gradientColor = QColor("#998CFF");
      break;
  }

  return gradientColor;
}

//--------------------------------------------------------------------
QDebug operator <<(QDebug d, const UVData &data)
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
  d << "UV Index: " << data.idx << "\n";
  return d;
}

//--------------------------------------------------------------------
bool getRoamingRegistryValue()
{
  const std::wstring KEY_PATH = L"SOFTWARE\\Felix de las Pozas Alvarez\\TrayWeather";
  const std::wstring KEY = L"Roaming enabled";

  HKEY key;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, KEY_PATH.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
  {
    std::cout << "getRoamingRegistryValue: Cannot open the registry!" << std::endl;
    return false;
  }

  DWORD type;
  DWORD cbData;
  if (RegQueryValueExW(key, KEY.c_str(), NULL, &type, NULL, &cbData) != ERROR_SUCCESS)
  {
    std::cout << "getRoamingRegistryValue: Could not read registry value!" << std::endl;
    RegCloseKey(key);
    return false;
  }

  if (type != REG_SZ)
  {
    std::cout << "getRoamingRegistryValue: Invalid type of registry value." << std::endl;
    RegCloseKey(key);
    return false;
  }

  std::wstring value(cbData / sizeof(wchar_t), L'\0');
  if (RegQueryValueExW(key, KEY.c_str(), NULL, NULL, reinterpret_cast<LPBYTE>(&value[0]), &cbData) != ERROR_SUCCESS)
  {
    std::cout << "getRoamingRegistryValue: Could not read registry value" << std::endl;
    RegCloseKey(key);
    return false;
  }

  RegCloseKey(key);

  size_t firstNull = value.find_first_of(L'\0');
  if (firstNull != std::wstring::npos) value.resize(firstNull);

  return value == L"true";
}

//--------------------------------------------------------------------
void changeLanguage(const QString &lang)
{
  if(!s_appTranslator.isEmpty())
  {
    qApp->removeTranslator(&s_appTranslator);
    s_appTranslator.load(QString());
  }

  if(!s_qtTranslator.isEmpty())
  {
    qApp->removeTranslator(&s_qtTranslator);
    s_qtTranslator.load(QString());
  }

  if(lang.compare("en_EN") != 0)
  {
    s_appTranslator.load(QString(":/TrayWeather/%1.qm").arg(lang));

    auto lang_single = lang.split('_').first();
    if(QT_LANGUAGES.contains(lang_single, Qt::CaseInsensitive))
    {
      s_qtTranslator.load(QString(":/TrayWeather/translations/qt_%1.qm").arg(lang_single));
    }
    else if(QT_LANGUAGES.contains(lang, Qt::CaseInsensitive))
    {
      s_qtTranslator.load(QString(":/TrayWeather/translations/qt_%1.qm").arg(lang));
    }
  }

  qApp->installTranslator(&s_appTranslator);
  qApp->installTranslator(&s_qtTranslator);
}

//--------------------------------------------------------------------
const QString unitsToText(const Units &u)
{
  const QStringList UNITS_TEXT{"metric", "imperial", "metric"};

  return UNITS_TEXT.at(static_cast<int>(u));
}

//--------------------------------------------------------------------
const QString generateMapGrades(const std::list<double> &grades, std::function<double(double)> f)
{
  QStringList result;

  for(auto value: grades)
  {
    value = f(value);
    result << QString::number(value, 'f', 2);
  }

  return result.join(',');
}

//--------------------------------------------------------------------
QString temperatureIconString(const Configuration &c)
{
  switch(c.units)
  {
    default:
    case Units::METRIC:
      return ":/TrayWeather/temp-celsius.svg";
      break;
    case Units::IMPERIAL:
      return ":/TrayWeather/temp-fahrenheit.svg";
      break;
    case Units::CUSTOM:
      switch(c.tempUnits)
      {
        default:
        case TemperatureUnits::CELSIUS:
          return ":/TrayWeather/temp-celsius.svg";
          break;
        case TemperatureUnits::FAHRENHEIT:
          return ":/TrayWeather/temp-fahrenheit.svg";
          break;
      }
  }

  return ":/TrayWeather/temp-celsius.svg";
}

//--------------------------------------------------------------------
QString temperatureIconText(const Configuration &c)
{
  switch(c.units)
  {
    default:
    case Units::METRIC:
      return "ºC";
      break;
    case Units::IMPERIAL:
      return "ºF";
      break;
    case Units::CUSTOM:
      switch(c.tempUnits)
      {
        default:
        case TemperatureUnits::CELSIUS:
          return "ºC";
          break;
        case TemperatureUnits::FAHRENHEIT:
          return "ºF";
          break;
      }
  }

  return "ºC";
}

//--------------------------------------------------------------------
void RichTextItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QStyleOptionViewItemV4 optionV4 = option;
  initStyleOption(&optionV4, index);

  auto style = optionV4.widget ? optionV4.widget->style() : QApplication::style();

  QTextDocument doc;
  doc.setHtml(optionV4.text);

  // Painting item without text
  optionV4.text = QString();
  style->drawControl(QStyle::CE_ItemViewItem, &optionV4, painter);

  QAbstractTextDocumentLayout::PaintContext ctx;
  ctx.palette = optionV4.palette;

  const auto textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &optionV4);
  painter->save();
  painter->translate(textRect.topLeft());
  painter->setClipRect(textRect.translated(-textRect.topLeft()));
  doc.documentLayout()->draw(painter, ctx);
  painter->restore();}

//--------------------------------------------------------------------
QSize RichTextItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QStyleOptionViewItemV4 options = option;
  initStyleOption(&options, index);

  QTextDocument doc;
  doc.setHtml(options.text);
  doc.setTextWidth(options.rect.width());
  return QSize(doc.idealWidth(), doc.size().height());
}

//--------------------------------------------------------------------
void CustomComboBox::paintEvent(QPaintEvent *e)
{
  const auto idx = this->currentIndex();

  QString text;
  if(idx != -1)
  {
    text = this->itemData(idx, Qt::DisplayRole).toString();
    this->setItemData(idx, QString(), Qt::DisplayRole);
  }

  QComboBox::paintEvent(e);

  if(idx == -1) return;

  this->setItemData(idx, text, Qt::DisplayRole);

  QPainter p(this);
  p.setClipRect(rect());

  QAbstractTextDocumentLayout::PaintContext ctx;
  ctx.palette = this->parentWidget()->palette();

  QTextDocument doc;
  doc.setHtml(text);
  doc.documentLayout()->draw(&p, ctx);
}

//--------------------------------------------------------------------
QPixmap createIconsSummary(const unsigned int theme, const int size, const QColor &color)
{
  QImage poster(QSize{size*5,size*5}, QImage::Format_ARGB32);
  const auto invertedColor = QColor{color.red() ^ 0xFF, color.green() ^ 0xFF, color.blue() ^ 0xFF};
  poster.fill(ICON_THEMES.at(theme).colored ? Qt::darkGray : invertedColor);
  QPainter painter(&poster);

  int x = 0, y = 0;
  const QStringList names = {"01d", "01n-0", "01n-1", "01n-2", "01n-3", "01n-4", "01n-5", "01n-6", "01n-7",
                             "02d", "02n", "03d", "03n", "04d", "04n", "09d", "09n", "10d", "10n",
                             "11d", "11n", "13d", "13n", "50d", "50n" };

  for(int i = 0; i < names.size();  ++i)
  {
    auto pixmap = weatherPixmap(names.at(i), theme, color);
    pixmap = pixmap.scaled(QSize{size,size}, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    painter.drawPixmap(QPoint{x,y}, pixmap);
    x += size; if(x == size*5) { x = 0; y+=size; }
  }
  painter.end();

  return QPixmap::fromImage(poster);
}

//--------------------------------------------------------------------
QRect computeDrawnRect(const QImage &image)
{
  const auto size = image.size();
  if(!size.isValid()) return QRect();

  std::vector<int> countX(size.width(), 0);
  std::vector<int> countY(size.height(), 0);

  for(int x = 0; x < size.width(); ++x)
  {
    for(int y = 0; y < size.height(); ++y)
    {
      if(qAlpha(image.pixel(x,y)) != 0)
      {
        countX[x]++;
        countY[y]++;
      }
    }
  }

  const auto ident = [](const auto& x) { return x != 0; };
  const auto firstX = std::distance(countX.cbegin(), std::find_if(countX.cbegin(), countX.cend(), ident));
  const auto firstY = std::distance(countY.cbegin(), std::find_if(countY.cbegin(), countY.cend(), ident));
  const auto lastX = image.width() - std::distance(countX.crbegin(), std::find_if(countX.crbegin(), countX.crend(), ident));
  const auto lastY = image.height() - std::distance(countY.crbegin(), std::find_if(countY.crbegin(), countY.crend(), ident));
  const auto maxX = lastX-firstX;
  const auto maxY = lastY-firstY;
  return QRect(firstX, firstY, maxX, maxY);
}
