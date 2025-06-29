/*
 File: OpenMeteo.cpp
 Created on: 08/02/2025
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
#include "OpenMeteo.h"
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

//----------------------------------------------------------------------------
void OpenMeteoProvider::requestData(std::shared_ptr<QNetworkAccessManager> netManager)
{
  // Weather & UV forecast.
  auto url = QUrl{QString("https://api.open-meteo.com/v1/forecast?latitude=%1&longitude=%2"
                          "&current=temperature_2m,relative_humidity_2m,rain,snowfall,weather_code,"
                          "cloud_cover,surface_pressure,wind_speed_10m,wind_direction_10m"
                          "&hourly=temperature_2m,relative_humidity_2m,rain,snowfall,weather_code,"
                          "surface_pressure,cloud_cover,wind_speed_10m,wind_direction_10m,uv_index"
                          "&timeformat=unixtime&timezone=auto").arg(m_config.latitude).arg(m_config.longitude)};

  netManager->get(QNetworkRequest{url});

  // Pollution forecast.
  url = QUrl{QString("https://air-quality-api.open-meteo.com/v1/air-quality?latitude=%1&longitude=%2&"
                     "current=european_aqi,pm10,pm2_5,carbon_monoxide,nitrogen_dioxide,sulphur_dioxide,"
                     "ozone,ammonia&hourly=pm10,pm2_5,carbon_monoxide,nitrogen_dioxide,sulphur_dioxide,"
                     "ozone,ammonia,european_aqi&timeformat=unixtime").arg(m_config.latitude).arg(m_config.longitude)};

  netManager->get(QNetworkRequest{url});
}

//----------------------------------------------------------------------------
void OpenMeteoProvider::processReply(QNetworkReply *reply)
{
  const auto originUrl = reply->request().url().toString();
  const auto contents  = reply->readAll();

  if(contents.contains(INVALID_MSG.toLocal8Bit()))
  {
    const QString msg = tr("Error: ") + tr("Unable to get weather data.");
    emit errorMessage(msg);
  }  

  if(originUrl.contains("forecast", Qt::CaseInsensitive))
  {
    if(reply->error() == QNetworkReply::NoError)
    {
      processWeatherData(contents);
    }
    else
    {
      const auto msg = tr("Error: ") + tr("No weather data.") + QString("\n%1").arg(reply->errorString());;
      emit errorMessage(msg);
    }
  }
  else
  {
    if (originUrl.contains("air-quality", Qt::CaseInsensitive))
    {
      if (reply->error() == QNetworkReply::NoError)
      {
        processPollutionData(contents);
      }
      else
      {
        const auto msg = tr("Error: ") + tr("No pollution data.") + QString("\n%1").arg(reply->errorString());
        emit errorMessage(msg);
      }
    }
    else if(originUrl.contains("search", Qt::CaseInsensitive))
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
  }
}

//----------------------------------------------------------------------------
void OpenMeteoProvider::searchLocations(const QString &text, std::shared_ptr<QNetworkAccessManager> netManager) const
{
  if(!netManager || text.isEmpty()) return;

  QString lang = "en";
  if(!m_config.language.isEmpty() && m_config.language.contains('_'))
  {
    const auto settings_lang = m_config.language.split('_').first();
    if(OWM_LANGUAGES.contains(settings_lang, Qt::CaseSensitive)) lang = settings_lang;

    const auto settings_compl = m_config.language.toLower();
    if(OWM_LANGUAGES.contains(settings_compl, Qt::CaseInsensitive)) lang = settings_compl;
  }

  auto cleanText = text.trimmed();
  cleanText = cleanText.remove(',');
  const auto words = cleanText.split(' ');
  const auto request_text = words.join('+');

  auto url = QUrl{QString("https://geocoding-api.open-meteo.com/v1/search?name=%1&count=10&language=%2&format=json").arg(request_text).arg(lang)};
  netManager->get(QNetworkRequest{url});
}

//----------------------------------------------------------------------------
void OpenMeteoProvider::processWeatherData(const QByteArray &contents)
{
  const auto jsonDocument = QJsonDocument::fromJson(contents);

  if(!jsonDocument.isNull() && jsonDocument.isObject())
  {
    // to discard entries older than 'right now'.
    const long long currentDt = std::chrono::duration_cast<std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto jsonObj = jsonDocument.object();
    const auto city    = m_config.city.isEmpty() ? tr("Unknown") : m_config.city;
    const auto region  = m_config.region.isEmpty() ? tr("Unknown") : m_config.region;
    auto country = m_config.country.isEmpty() ? tr("Unknown") : m_config.country;      
    if(city.compare(region, Qt::CaseInsensitive) != 0) country = region;

    const auto keys = jsonObj.keys();

    if(keys.contains("current"))
    {
      const auto current = jsonObj.value("current").toObject();
      const auto currentTime = current.value("time").toVariant().toLongLong(0);

      m_current.dt         = std::max(currentTime, currentDt);
      m_current.cloudiness = current.value("cloud_cover").toDouble(0);
      m_current.temp       = current.value("temperature_2m").toDouble(0);
      m_current.temp_min   = m_current.temp;
      m_current.temp_max   = m_current.temp;
      m_current.humidity   = current.value("relative_humidity_2m").toDouble(0);
      m_current.pressure   = current.value("surface_pressure").toDouble(0);
      m_current.weather_id = current.value("weather_code").toInt(0);
      m_current.wind_speed = current.value("wind_speed_10m").toDouble(0);
      m_current.wind_dir   = current.value("wind_direction_10m").toDouble(0);
      m_current.snow       = current.value("snowfall").toDouble(0) * 10; // units are cm not mm
      m_current.rain       = current.value("rain").toDouble(0);
      m_current.name       = city;
      m_current.country    = country;

      const auto [sunrise, sunset] = computeSunriseSunset(m_current, m_config.longitude, m_config.latitude);
      m_current.sunrise    = sunrise;
      m_current.sunset     = sunset;

      fillWMOCodeInForecast(m_current);

      changeWeatherUnits(m_config, m_current);

      emit weatherDataReady();
    }

    if(keys.contains("hourly"))
    {
      m_forecast.clear();
      m_uv.clear();

      if(m_current.isValid())
        m_forecast << m_current;
        
      const auto hourly = jsonObj.value("hourly").toObject();        

      const auto times        = hourly.value("time").toArray();
      const auto temperatures = hourly.value("temperature_2m").toArray();
      const auto humidities   = hourly.value("relative_humidity_2m").toArray();
      const auto rain         = hourly.value("rain").toArray();
      const auto snow         = hourly.value("snowfall").toArray();
      const auto owmCodes     = hourly.value("weather_code").toArray();
      const auto pressures    = hourly.value("surface_pressure").toArray();
      const auto clouds       = hourly.value("cloud_cover").toArray();
      const auto windSpeeds   = hourly.value("wind_speed_10m").toArray();
      const auto windDirs     = hourly.value("wind_direction_10m").toArray();
      const auto uvIndexes    = hourly.value("uv_index").toArray();

      auto hasEntry = [this](unsigned long dt) { for(auto entry: this->m_forecast) if(entry.dt == dt) return true; return false; };
      auto hasUVEntry = [this](unsigned long dt) { for(auto entry: this->m_uv) if(entry.dt == dt) return true; return false; };

      for(auto i = 0; i < times.count(); ++i)
      {
        const auto dt = times.at(i).toInt(0);
        if(dt < currentDt) continue;

        UVData uvData;
        uvData.dt = dt;
        uvData.idx = uvIndexes.at(i).toDouble(0);

        if(!hasUVEntry(uvData.dt))
          m_uv << uvData;

        ForecastData data;
        data.dt          = dt;
        data.cloudiness  = clouds.at(i).toDouble(0);
        data.temp        = temperatures.at(i).toDouble(0);
        data.temp_min    = data.temp;
        data.temp_max    = data.temp;
        data.humidity    = humidities.at(i).toDouble(0);
        data.pressure    = pressures.at(i).toDouble(0);
        data.weather_id  = owmCodes.at(i).toInt(0);
        data.wind_speed  = windSpeeds.at(i).toDouble(0);
        data.wind_dir    = windDirs.at(i).toDouble(0);
        data.snow        = snow.at(i).toDouble(0) * 10; // units are cm not mm
        data.rain        = rain.at(i).toDouble(0);
        data.name        = city;
        data.country     = country;      

        const auto [sunrise, sunset] = computeSunriseSunset(data, m_config.longitude, m_config.latitude);
        data.sunrise     = sunrise;
        data.sunset      = sunset;

        fillWMOCodeInForecast(data);

        if(!hasEntry(data.dt))
        {
          changeWeatherUnits(m_config, data);
          m_forecast << data;
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
    }
  }
}

//----------------------------------------------------------------------------
void OpenMeteoProvider::processPollutionData(const QByteArray &contents)
{
  const auto jsonDocument = QJsonDocument::fromJson(contents);

  if(!jsonDocument.isNull() && jsonDocument.isObject())
  {
    // to discard entries older than 'right now'.
    const auto currentDt = std::chrono::duration_cast<std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto jsonObj = jsonDocument.object();

    const auto keys = jsonObj.keys();

    m_pollution.clear();
    auto hasEntry = [this](unsigned long dt) { for(auto entry: this->m_pollution) if(entry.dt == dt) return true; return false; };

    auto processAQIvalue = [](PollutionData &data)
    {
      // European AQI: 0-20 (good), 20-40 (fair), 40-60 (moderate), 60-80 (poor), 80-100 (very poor) and exceeds 100 for extremely poor.
      data.aqi   = std::max(1, std::min(5, static_cast<int>(std::nearbyint(std::ceil(data.aqi / 20.0)))));
      
      switch(data.aqi)
      {
        case 1:  data.aqi_text = tr("Good"); break;
        case 2:  data.aqi_text = tr("Fair"); break;
        case 3:  data.aqi_text = tr("Moderate"); break;
        case 4:  data.aqi_text = tr("Poor"); break;
        default: data.aqi_text = tr("Very poor"); break;
      }
    };

    if(keys.contains("current"))
    {
      const auto current = jsonObj.value("current").toObject();   

      PollutionData data;
      data.dt    = current.value("time").toVariant().toLongLong(0);
      data.aqi   = current.value("european_aqi").toDouble(20);
      data.co    = current.value("carbon_monoxide").toDouble(0);
      data.no    = 0;
      data.no2   = current.value("nitrogen_dioxide").toDouble(0);
      data.o3    = current.value("ozone").toDouble(0);
      data.so2   = current.value("sulphur_dioxide").toDouble(0);
      data.pm2_5 = current.value("pm2_5").toDouble(0);
      data.pm10  = current.value("pm10").toDouble(0);
      data.nh3   = current.value("ammonia").toDouble(0);
      processAQIvalue(data);

      if(!hasEntry(data.dt))
        m_pollution << data;
    }

    if(keys.contains("hourly"))
    {
      const auto hourly = jsonObj.value("hourly").toObject();   

      const auto times = hourly.value("time").toArray();
      const auto pm10s = hourly.value("pm10").toArray();
      const auto pm25s = hourly.value("pm2_5").toArray();
      const auto COs   = hourly.value("carbon_monoxide").toArray();
      const auto NO2s  = hourly.value("nitrogen_dioxide").toArray();
      const auto SO2s  = hourly.value("sulphur_dioxide").toArray();
      const auto O3s   = hourly.value("ozone").toArray();
      const auto NH3s  = hourly.value("ammonia").toArray();
      const auto AQIs  = hourly.value("european_aqi").toArray();

      for(auto i = 0; i < times.count(); ++i)
      {
        const auto dt = times.at(i).toInt(0);
        if(dt < currentDt) continue;

        PollutionData data;
        data.dt    = times.at(i).toInt(0);
        data.aqi   = AQIs.at(i).toDouble(20);
        data.co    = COs.at(i).toDouble(0);
        data.no    = 0;
        data.no2   = NO2s.at(i).toDouble(0);
        data.o3    = O3s.at(i).toDouble(0);
        data.so2   = SO2s.at(i).toDouble(0);
        data.pm2_5 = pm25s.at(i).toDouble(0);
        data.pm10  = pm10s.at(i).toDouble(0);
        data.nh3   = NH3s.at(i).toDouble(0);
        processAQIvalue(data);

        if(!hasEntry(data.dt))
          m_pollution << data;
      }
    }

    if (!m_pollution.isEmpty())
    {
      auto lessThan = [](const PollutionData &left, const PollutionData &right){ if(left.dt < right.dt) return true; return false; };
      std::sort(m_pollution.begin(), m_pollution.end(), lessThan);
      emit pollutionForecastDataReady();
    }
  }  
}

//----------------------------------------------------------------------------
void OpenMeteoProvider::processLocationsData(const QByteArray &contents)
{
  const auto jsonDocument = QJsonDocument::fromJson(contents);

  if(!jsonDocument.isNull() && jsonDocument.isObject())
  {
    auto results = jsonDocument.object();
    const auto locationsArray = results.value("results").toArray();

    if(!results.keys().contains("results") || locationsArray.isEmpty())
    {
      emit foundLocations(Locations());
      return;
    }

    Locations locations;
    for(int i = 0; i < locationsArray.count(); ++i)
    {
      auto foundLocation = locationsArray.at(i).toObject();
      Location location;
      location.location = foundLocation.value("name").toString(); 
      location.translated = location.location;
      location.latitude = foundLocation.value("latitude").toDouble(0);
      location.longitude = foundLocation.value("longitude").toDouble(0);
      location.country = foundLocation.value("country").toString();
      QStringList extra{ foundLocation.value("admin1").toString(), foundLocation.value("admin2").toString() };
      extra.removeAll("");
      extra.removeDuplicates();
      location.region = extra.join('/');

      locations << location;
    }

    emit foundLocations(locations);
  }
}
