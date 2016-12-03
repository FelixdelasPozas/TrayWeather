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

// Qt
#include <QJsonArray>
#include <QIcon>

// C++
#include <exception>
#include <memory>
#include <time.h>
#include <cmath>

constexpr auto PI   = 3.14159265358979323846;
constexpr auto PI_2 = 1.57079632679489661923;
constexpr auto PI_4 = 0.78539816339744830962;
#define Degrees2Radians(a) ((a) / (180 / PI))
#define Radians2Degrees(a) ((a) * (180 / PI))
const unsigned int EARTH_RADIUS = 6378137;

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
  auto main    = entry.value("main").toObject();
  auto weather = entry.value("weather").toArray().first().toObject();
  auto wind    = entry.value("wind").toObject();
  auto rain    = entry.value("rain").toObject();
  auto snow    = entry.value("snow").toObject();
  auto sys     = entry.value("sys").toObject();

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
}

//--------------------------------------------------------------------
void unixTimeStampToDate(struct tm &time, const long long timestamp)
{
  const time_t stamp{timestamp};

  localtime_s(&time, &stamp);
}

//--------------------------------------------------------------------
int moonPhase(const time_t timestamp)
{
  // taken from http://www.voidware.com/moon_phase.htm

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

  return b;
}

//--------------------------------------------------------------------
const QIcon weatherIcon(const ForecastData& data)
{
  QString iconId = data.icon_id;

  if(data.icon_id == "01n")
  {
    iconId += QObject::tr("-%1").arg(moonPhase(data.dt));
  }

  return QIcon(Icons.value(iconId));
}

//--------------------------------------------------------------------
const QIcon moonIcon(const ForecastData& data)
{
  if(!data.icon_id.isEmpty())
  {
    QString iconId{"01n"};
    iconId += QObject::tr("-%1").arg(moonPhase(data.dt));

    return QIcon(Icons.value(iconId));
  }

  return QIcon{":/TrayWeather/network_error.svg"};
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
const double latitudeToYMercator(double latitude)
{
  return std::log(std::tan(Degrees2Radians(latitude) / 2 + PI_4 )) * EARTH_RADIUS;
}

//--------------------------------------------------------------------
const double longitudeToXMercator(const double longitude)
{
  return Degrees2Radians(longitude) * EARTH_RADIUS;
}
