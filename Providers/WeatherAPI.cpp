/*
 File: WeatherAPI.cpp
 Created on: 10/03/2025
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
#include "WeatherAPI.h"
#include <Utils.h>

// Qt
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDir>
#include <QSettings>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStringList>

// C++
#include <chrono>
#include <cmath>

static const QStringList WEATHERAPI_LANGUAGES = { "ar", "bn", "bg", "zh", "zh_tw", "cs", "da", "nl", "fi", "fr", "de", "el", "hi",
                                                  "hu", "it", "ja", "jv", "ko", "zh_cmn", "mr", "pl", "pt", "pa", "ro", "ru", "sr",
                                                  "si", "sk", "es", "sv", "ta", "te", "tr", "uk", "ur", "vi", "zh_wuu", "zh_hsn", 
                                                  "zh_yue", "zu" };

//----------------------------------------------------------------------------
void WeatherAPIProvider::requestData(std::shared_ptr<QNetworkAccessManager> netManager)
{
  if(m_apiKey.isEmpty())
  { 
    const QString msg = tr("WeatherAPI API Key is missing.");
    emit errorMessage(msg);
    return; 
  }

  QString lang = "en";
  if(!m_config.language.isEmpty() && m_config.language.contains('_'))
  {
    const auto settings_lang = m_config.language.split('_').first();
    if(WEATHERAPI_LANGUAGES.contains(settings_lang, Qt::CaseSensitive)) lang = settings_lang;

    const auto settings_compl = m_config.language.toLower();
    if(WEATHERAPI_LANGUAGES.contains(settings_compl, Qt::CaseInsensitive)) lang = settings_compl;
  }

  // just in case, replace comma for dot.
  const auto latStr = QString::number(m_config.latitude).replace(",",".");
  const auto lonStr = QString::number(m_config.longitude).replace(",",".");
  const auto query = latStr + "," + lonStr;

  auto url = QUrl{QString("http://api.weatherapi.com/v1/forecast.json?key=%1&q=%2&days=3&aqi=yes&alerts=yes&lang=%3").arg(m_apiKey)
                                                                                                                     .arg(query)
                                                                                                                     .arg(lang)};

  netManager->get(QNetworkRequest{url});
}

//----------------------------------------------------------------------------
void WeatherAPIProvider::processReply(QNetworkReply *reply)
{
  const auto originUrl = reply->request().url().toString();
  const auto contents  = reply->readAll();

  if(contents.contains(INVALID_MSG.toLocal8Bit()))
  {
    m_apiKeyValid = false;
    emit apiKeyValid(false);
    return;
  }

  if(originUrl.contains("current", Qt::CaseInsensitive))
  {
    if(reply->error() == QNetworkReply::NoError)
    {
      if(!m_apiKeyValid)
      {
        m_apiKeyValid = true;
        emit apiKeyValid(true);
        return;
      } 
    }
  }
  else
  {
    if(originUrl.contains("search", Qt::CaseInsensitive))
    {
      if(reply->error() == QNetworkReply::NoError)
      {
        processLocationsData(contents);
      }
      else
      {
        const auto msg = tr("Error: ") + tr("Couldn't get location information.") + QString("\n%1").arg(reply->errorString());
        emit errorMessage(msg);
      }
    }
    else if (originUrl.contains("forecast", Qt::CaseInsensitive))
    {
      if (reply->error() == QNetworkReply::NoError)
      {
        processWeatherData(contents);
      }
      else
      {
        const auto msg = tr("Error: ") + tr("No weather data.") + QString("\n%1").arg(reply->errorString());;
        emit errorMessage(msg);
      }
    }
  }
}

//----------------------------------------------------------------------------
void WeatherAPIProvider::searchLocations(const QString &text, std::shared_ptr<QNetworkAccessManager> netManager) const
{
  const auto url = QUrl(QString("http://api.weatherapi.com/v1/search.json?key=%1&q=%2").arg(m_apiKey).arg(text));
  netManager->get(QNetworkRequest{url});
}

//----------------------------------------------------------------------------
void WeatherAPIProvider::loadSettings()
{
  QSettings settings = applicationSettings();
  m_apiKey = settings.value(WeatherAPIProvider::WEATHERAPI_APIKEY, QString()).toString();
}

//----------------------------------------------------------------------------
void WeatherAPIProvider::saveSettings()
{
  if(!m_apiKey.isEmpty())
  {
    QSettings settings = applicationSettings();
    settings.setValue(WeatherAPIProvider::WEATHERAPI_APIKEY, m_apiKey);
  }
}

//----------------------------------------------------------------------------
void WeatherAPIProvider::processWeatherData(const QByteArray &contents)
{
  const auto jsonDocument = QJsonDocument::fromJson(contents);

  if(!jsonDocument.isNull() && jsonDocument.isObject())
  {
    // to discard entries older than 'right now'.
    const auto currentDt = std::chrono::duration_cast<std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto jsonObj = jsonDocument.object();
    const auto keys = jsonObj.keys();

    m_forecast.clear();
    m_pollution.clear();
    m_uv.clear();

    if(keys.contains("location"))
    {
      const auto location = jsonObj.value("location").toObject();
      m_current.country = m_config.country = location.value("country").toString();
      m_config.region = location.value("region").toString();
      m_current.name = m_config.city = location.value("name").toString();
    }

    if(keys.contains("current"))
    {
      const auto currentData = jsonObj.value("current").toObject();
      parseForecastEntry(currentData, m_current);
      changeWeatherUnits(m_config, m_current);
      m_forecast << m_current;

      emit weatherDataReady();

      const auto airData = currentData.value("air_quality").toObject();

      PollutionData pollutionData;
      pollutionData.dt = m_current.dt;
      parsePollutionEntry(airData, pollutionData);
      m_pollution << pollutionData;

      UVData uvData;
      uvData.dt = m_current.dt;
      uvData.idx = currentData.value("uv").toDouble(0);
      m_uv << uvData;
    }

    if(keys.contains("forecast"))
    {
      const auto forecast = jsonObj.value("forecast").toObject();
      const auto days = forecast.value("forecastday").toArray();

      auto hasForecastEntry = [this](unsigned long dt) { for(auto entry: this->m_forecast) if(entry.dt == dt) return true; return false; };
      auto hasUVEntry = [this](unsigned long dt) { for(auto entry: this->m_uv) if(entry.dt == dt) return true; return false; };
      auto hasPollutionEntry = [this](unsigned long dt) { for(auto entry: this->m_pollution) if(entry.dt == dt) return true; return false; };

      for(auto i = 0; i < days.count(); ++i)
      {
        auto day = days.at(i).toObject();

        auto hours = day.value("hour").toArray();
        for(auto j = 0; j < hours.count(); ++j)
        {
          const auto hour = hours.at(j).toObject();
          const auto dt = hour.value("time_epoch").toInt(0);
          if(dt < currentDt) continue;

          ForecastData data;
          parseForecastEntry(hour, data);
          changeWeatherUnits(m_config, data);

          if(!hasForecastEntry(data.dt))
          {
            m_forecast << data;
          }

          PollutionData pData;
          pData.dt = data.dt;
          const auto airData = hour.value("air_quality").toObject();
          parsePollutionEntry(airData, pData);

          if(!hasPollutionEntry(pData.dt))
          {
            m_pollution << pData;
          }

          UVData uvData;
          uvData.dt = data.dt;
          uvData.idx = hour.value("uv").toDouble(0);

          if(!hasUVEntry(uvData.dt))
          {
            m_uv << uvData;
          }
        }
      }

      if(!m_forecast.isEmpty())
      {
        auto lessThan = [](const ForecastData &left, const ForecastData &right) { if(left.dt < right.dt) return true; return false; };
        std::sort(m_forecast.begin(), m_forecast.end(), lessThan);

        emit weatherForecastDataReady();
      }

      if(!m_uv.isEmpty())
      {
        auto lessThan = [](const UVData &left, const UVData &right) { if(left.dt < right.dt) return true; return false; };
        std::sort(m_uv.begin(), m_uv.end(), lessThan);

        emit uvForecastDataReady();
      }

      if(!m_pollution.isEmpty())
      {
        auto lessThan = [](const PollutionData &left, const PollutionData &right) { if(left.dt < right.dt) return true; return false; };
        std::sort(m_pollution.begin(), m_pollution.end(), lessThan);

        emit pollutionForecastDataReady();
      }
    }

    if(keys.contains("alerts"))
    {
      const auto alertObj = jsonObj.value("alerts").toObject();
      const auto alerts = alertObj.value("alert").toArray();
      
      Alerts alertList;
      for(auto i = 0; i < alerts.count(); ++i)
      {
        auto alertData = alerts.at(i).toObject();

        Alert alert;
        alert.sender = QObject::tr("Unknown");
        alert.event = alertData.value("event").toString();
        alert.startTime = QDateTime::fromString(alertData.value("effective").toString(), Qt::ISODate).toMSecsSinceEpoch();
        alert.endTime = QDateTime::fromString(alertData.value("expires").toString(),  Qt::ISODate).toMSecsSinceEpoch();
        alert.description = QString("<b>%1</b><br>%2").arg(alertData.value("headline").toString()).arg(alertData.value("desc").toString());
        alertList << alert;
      }

      if(!alertList.isEmpty())
      {
        emit weatherAlerts(alertList);
      }
    }
  }
}

//----------------------------------------------------------------------------
void WeatherAPIProvider::processLocationsData(const QByteArray &contents)
{
  const auto jsonDocument = QJsonDocument::fromJson(contents);

  if (!jsonDocument.isNull() && jsonDocument.isArray())
  {
    // WeatherAPI Geolocation JSON keys.
    const QString NAME_KEY = "name";
    const QString LATITUDE_KEY = "lat";
    const QString LONGITUDE_KEY = "lon";
    const QString COUNTRY_KEY = "country";
    const QString STATE_KEY = "region";

    const auto locationsArray = jsonDocument.array();

    if(locationsArray.isEmpty())
    {
      const QString msg = tr("No locations found for '%1'.");
      emit errorMessage(msg);
      return;
    }

    Locations locations;

    for (auto location : locationsArray)
    {
      const auto locObject = location.toObject();

      Location locationData;

      locationData.location = locObject.value(NAME_KEY).toString();
      locationData.translated = locationData.location; // no translations.
      locationData.latitude = locObject.value(LATITUDE_KEY).toDouble();
      locationData.longitude = locObject.value(LONGITUDE_KEY).toDouble();
      locationData.country = locObject.value(COUNTRY_KEY).toString();
      locationData.region = locObject.value(STATE_KEY).toString();

      locations << locationData;
    }

    emit foundLocations(locations);
  }
}

//--------------------------------------------------------------------
void WeatherAPIProvider::parseForecastEntry(const QJsonObject& entry, ForecastData& data)
{
  const auto keys = entry.keys();
  if(!keys.contains("condition"))
    return;

  const auto condition = entry.value("condition").toObject();    

  if(keys.contains("last_updated_epoch")) 
    data.dt = entry.value("last_updated_epoch").toVariant().toULongLong();
  else
    data.dt = entry.value("time_epoch").toVariant().toULongLong();

  data.temp_max = data.temp_min = data.temp = entry.value("temp_c").toDouble(0);
  data.cloudiness = entry.value("cloud").toInt(0);
  data.humidity = entry.value("humidity").toInt(0);
  data.pressure = entry.value("pressure_mb").toInt(0);
  data.description = condition.value("text").toString();
  data.icon_id = condition.value("icon").toString();
  data.weather_id = condition.value("code").toInt();
  data.wind_speed = entry.value("wind_kph").toDouble(0) * (1000.0/3600); // are in km/h not meters/s
  data.wind_dir = entry.value("wind_degree").toInt(0);
  data.rain = entry.value("precip_mm").toDouble(0);
  data.snow = entry.value("snow_cm").toDouble(0) * 10; // are in cm not mm
  const auto [sunrise, sunset] = computeSunriseSunset(data, m_config.longitude, m_config.latitude);
  data.sunrise = sunrise;
  data.sunset = sunset;

  fillWAPICodeInForecast(data);
}

//--------------------------------------------------------------------
void WeatherAPIProvider::parsePollutionEntry(const QJsonObject &entry, PollutionData &data)
{
  // dt set in caller.
  data.aqi   = entry.value("us-epa-index").toInt(1);
  data.co    = entry.value("co").toDouble(0);
  data.no2   = entry.value("no2").toDouble(0);
  data.o3    = entry.value("o3").toDouble(0);
  data.so2   = entry.value("so2").toDouble(0);
  data.pm2_5 = entry.value("pm2_5").toDouble(0);
  data.pm10  = entry.value("pm10").toDouble(0);

  switch(data.aqi)
  {
    case 1:  data.aqi_text = tr("Good"); break;
    case 2:  data.aqi_text = tr("Fair"); break;
    case 3:  data.aqi_text = tr("Moderate"); break;
    case 4:  data.aqi_text = tr("Poor"); break;
    default: data.aqi_text = tr("Very poor"); break;
  }
}

//----------------------------------------------------------------------------
void WeatherAPIProvider::setApiKey(const QString& key)
{
  if(!m_apiKey.isEmpty() && m_apiKey.compare(key, Qt::CaseSensitive) == 0)
    return;

  if (!key.isEmpty()) {
      m_apiKey = key;
      m_apiKeyValid = false;
  }
};

//----------------------------------------------------------------------------
void WeatherAPIProvider::testApiKey(std::shared_ptr<QNetworkAccessManager> netManager)
{
  if(m_apiKeyValid)
  {
    emit apiKeyValid(true);
    return;
  }

  // protect from abuse
  static auto lastRequest = std::chrono::steady_clock::now();
  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<double> interval = now - lastRequest;
  const auto duration = interval.count();

  if(duration > 0.1 && duration < 2.5) return;
  lastRequest = now;

  const auto latStr = QString::number(m_config.latitude).replace(",",".");
  const auto lonStr = QString::number(m_config.longitude).replace(",",".");
  const auto query = latStr + "," + lonStr;

  auto url = QUrl{QString("http://api.weatherapi.com/v1/current.json?key=%1&q=%2").arg(m_apiKey).arg(query)};

  netManager->get(QNetworkRequest{url});
}

//--------------------------------------------------------------------
void WeatherAPIProvider::fillWAPICodeInForecast(ForecastData& forecast)
{
  const auto code = static_cast<int>(forecast.weather_id);
  const bool isDay = (forecast.sunrise <= forecast.dt) && (forecast.dt <= forecast.sunset);
  const auto iconSuffix = isDay ? QString("d") : QString("n");

  switch(code)
  {
    case 1000: // clear
      forecast.icon_id = "01" + iconSuffix;
      forecast.description = QApplication::translate("QObject", CLEAR_SKY.toStdString().c_str());
      forecast.parameters = "clear";
      break;
    case 1003: // partly cloudy
      forecast.icon_id = "02" + iconSuffix;
      forecast.description = QApplication::translate("QObject", MAINLY_CLEAR.toStdString().c_str());
      forecast.parameters = "clear";
    case 1006: // cloudy
      forecast.icon_id = "03" + iconSuffix;
      forecast.description = QApplication::translate("QObject", PARTLY_CLOUDY.toStdString().c_str());
      forecast.parameters = "cloudy";
      break;
    case 1009: // overcast
      forecast.icon_id = "04" + iconSuffix;
      forecast.description = QApplication::translate("QObject", OVERCAST.toStdString().c_str());
      forecast.parameters = "overcast";
      break;
    case 1030: // mist
    case 1135: // fog
    case 1147: //	Freezing fog
      forecast.icon_id = "50" + iconSuffix;
      forecast.description = QApplication::translate("QObject", FOG.toStdString().c_str());
      forecast.parameters = "fog";
      break;
    case 1063: //	Patchy rain possible
    case 1150: //	Patchy light drizzle
    case 1180: //	Patchy light rain
    case 1183: //	Light rain
      forecast.icon_id = "10" + iconSuffix;
      forecast.description = QApplication::translate("QObject", SLIGHT_RAIN.toStdString().c_str());
      forecast.parameters = "rain";
      break;
    case 1066: // Patchy snow possible
    case 1069: //	Patchy sleet possible
    case 1114: //	Blowing snow
      forecast.icon_id = "13" + iconSuffix;
      forecast.description = QApplication::translate("QObject", SLIGHT_SNOW.toStdString().c_str());
      forecast.parameters = "snow";
      break;
    case 1072: //	Patchy freezing drizzle possible
    case 1168: //	Freezing drizzle
      forecast.icon_id = "09" + iconSuffix;
      forecast.description = QApplication::translate("QObject", LIGHT_FREEZING_DRIZZLE.toStdString().c_str());
      forecast.parameters = "freezing drizzle";
      break;
    case 1117: //	Blizzard
      forecast.icon_id = "13" + iconSuffix;
      forecast.description = QApplication::translate("QObject", HEAVY_SNOW_SHOWERS.toStdString().c_str());
      forecast.parameters = "snow";
      break;
    case 1171: //	Heavy freezing drizzle
      forecast.icon_id = "09" + iconSuffix;
      forecast.description = QApplication::translate("QObject", DENSE_FREEZING_DRIZZLE.toStdString().c_str());
      forecast.parameters = "freezing drizzle";
      break;
    case 1186: //	Moderate rain at times
    case 1189: //	Moderate rain
      forecast.icon_id = "10" + iconSuffix;
      forecast.description = QApplication::translate("QObject", MODERATE_RAIN.toStdString().c_str());
      forecast.parameters = "moderate rain";
      break;
    case 1192: //	Heavy rain at times
    case 1195: //	Heavy rain
      forecast.icon_id = "10" + iconSuffix;
      forecast.description = QApplication::translate("QObject", HEAVY_RAIN.toStdString().c_str());
      forecast.parameters = "heavy rain";
      break;
    case 1198: //	Light freezing rain
    case 1201: //	Moderate or heavy freezing rain
      forecast.icon_id = "13" + iconSuffix;
      forecast.description = code == 1198 ? QApplication::translate("QObject", LIGHT_FREEZING_RAIN.toStdString().c_str()) : 
                                            QApplication::translate("QObject", HEAVY_FREEZING_RAIN.toStdString().c_str());
      forecast.parameters = "freezing rain";
      break;
    case 1204: //	Light sleet
    case 1207: //	Moderate or heavy sleet
    case 1210: //	Patchy light snow
    case 1213: //	Light snow
    case 1249: //	Light sleet showers
      forecast.icon_id = "13" + iconSuffix;
      forecast.description = QApplication::translate("QObject", SLIGHT_SNOW.toStdString().c_str());
      forecast.parameters = "light snow";
      break;
    case 1216: //	Patchy moderate snow
    case 1219: //	Moderate snow
    case 1255: //	Light snow showers
    case 1252: //	Moderate or heavy sleet showers
      forecast.icon_id = "13" + iconSuffix;
      forecast.description = QApplication::translate("QObject", MODERATE_SNOW.toStdString().c_str());
      forecast.parameters = "moderate snow";
      break;
    case 1222: //	Patchy heavy snow
    case 1225: //	Heavy snow
    case 1258: //	Moderate or heavy snow showers
      forecast.icon_id = "13" + iconSuffix;
      forecast.description = QApplication::translate("QObject", HEAVY_SNOW.toStdString().c_str());
      forecast.parameters = "heavy snow";
      break;
    case 1237: //	Ice pellets
    case 1261: //	Light showers of ice pellets
    case 1264: //	Moderate or heavy showers of ice pellets
      forecast.icon_id = "13" + iconSuffix;
      forecast.description = QApplication::translate("QObject", SNOW_GRAINS.toStdString().c_str());
      forecast.parameters = "ice pellets";
      break;
    case 1240: //	Light rain shower
    case 1243: //	Moderate or heavy rain shower
      forecast.icon_id = "09" + iconSuffix;
      forecast.description = code == 1240 ? QApplication::translate("QObject", SLIGHT_RAIN_SHOWERS.toStdString().c_str()) :
                                            QApplication::translate("QObject", MODERATE_RAIN_SHOWERS.toStdString().c_str());
      forecast.parameters = "rain showers";
      break;
    case 1246: //	Torrential rain shower
      forecast.icon_id = "09" + iconSuffix;
      forecast.description = QApplication::translate("QObject", VIOLENT_RAIN_SHOWERS.toStdString().c_str());
      forecast.parameters = "rain showers";
      break;
    case 1087: // Thundery outbreaks possible
    case 1273: //	Patchy light rain with thunder
    case 1276: //	Moderate or heavy rain with thunder
    case 1279: //	Patchy light snow with thunder
    case 1282: //	Moderate or heavy snow with thunder
      forecast.icon_id = "11" + iconSuffix;
      forecast.description = QApplication::translate("QObject", THUNDERSTORM.toStdString().c_str());
      forecast.parameters = "thunderstorm";
      break;
    default:
      forecast.icon_id = "01" + iconSuffix;
      forecast.description = QApplication::translate("QObject", "Unknown");
      forecast.parameters = "unknown";
      break;
  }
}
