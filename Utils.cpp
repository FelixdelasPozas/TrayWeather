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

// C++
#include <functional>
#include <exception>
#include <memory>
#include <time.h>
#include <cmath>

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
