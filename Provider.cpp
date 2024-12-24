/*
 File: Provider.cpp
 Created on: 01/11/2024
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
#include <Provider.h>
#include <ISO 3166-1-alpha-2.h>

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
std::unique_ptr<WeatherProvider> WeatherProviderFactory::createProvider(const QString &name, Configuration &config)
{
  if(name.compare(OWM_25_PROVIDER) == 0)
    return std::make_unique<OWM25Provider>(config);

  if(name.compare(OPENMETEO_PROVIDER) == 0)    
    return std::make_unique<OpenMeteoProvider>(config);
  
  return nullptr;
}

//----------------------------------------------------------------------------
WeatherProvider::WeatherProvider(const QString &name, Configuration &config)
: m_name{name}
, m_config{config}
, m_keyValid{false}
{
  loadSettings();
}

//----------------------------------------------------------------------------
WeatherProvider::~WeatherProvider()
{
  saveSettings();
}

// OpenWeatherMap 2.5 API ----------------------------------------------------
//----------------------------------------------------------------------------
void OWM25Provider::requestData(std::shared_ptr<QNetworkAccessManager> netManager) const
{
  if(!m_apiKeyValid) return;

  QString lang = "en";
  if(!m_config.language.isEmpty() && m_config.language.contains('_'))
  {
    const auto settings_lang = m_config.language.split('_').first();
    if(OWM_LANGUAGES.contains(settings_lang, Qt::CaseSensitive)) lang = settings_lang;

    const auto settings_compl = m_config.language.toLower();
    if(OWM_LANGUAGES.contains(settings_compl, Qt::CaseInsensitive)) lang = settings_compl;
  }

  auto url = QUrl{QString("http://api.openweathermap.org/data/2.5/weather?lat=%1&lon=%2&lang=%3&units=%4&appid=%5").arg(m_config.latitude)
                                                                                                                   .arg(m_config.longitude)
                                                                                                                   .arg(lang)
                                                                                                                   .arg(unitsToText(m_config.units))
                                                                                                                   .arg(m_apiKey)};
  netManager->get(QNetworkRequest{url});

  url = QUrl{QString("http://api.openweathermap.org/data/2.5/forecast?lat=%1&lon=%2&lang=%3&units=%4&appid=%5").arg(m_config.latitude)
                                                                                                               .arg(m_config.longitude)
                                                                                                               .arg(lang)
                                                                                                               .arg(unitsToText(m_config.units))
                                                                                                               .arg(m_apiKey)};
  netManager->get(QNetworkRequest{url});

  url = QUrl{QString("http://api.openweathermap.org/data/2.5/air_pollution/forecast?lat=%1&lon=%2&lang=%3&units=%4&appid=%5").arg(m_config.latitude)
                                                                                                                             .arg(m_config.longitude)
                                                                                                                             .arg(lang)
                                                                                                                             .arg(unitsToText(m_config.units))
                                                                                                                             .arg(m_apiKey)};
  netManager->get(QNetworkRequest{url});
}

//----------------------------------------------------------------------------
QString OWM25Provider::mapsPage() const
{
  if(!m_apiKeyValid) return QString();
  
  QFile webfile(":/TrayWeather/webpage.html");
  if(webfile.open(QFile::ReadOnly))
  {
    QString webpage{webfile.readAll()};

    auto nullF = [](double d){return d;};
    QString isMetric, isImperial, degrees, windUnit, rainUnit, rainGrades, windGrades, tempGrades;
    switch(m_config.units)
    {
      case Units::IMPERIAL:
        isMetric   = "false";
        isImperial = "true";
        degrees    = "ºF";
        windUnit   = tr("mph");
        rainUnit   = tr("inches/h");
        rainGrades = generateMapGrades(RAIN_MAP_LAYER_GRADES_MM, convertMmToInches);
        windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, convertMetersSecondToMilesHour);
        tempGrades = generateMapGrades(TEMP_MAP_LAYER_GRADES_CELSIUS, convertCelsiusToFahrenheit);
        break;
      default:
      case Units::METRIC:
        isMetric   = "true";
        isImperial = "false";
        degrees    = "ºC";
        windUnit   = tr("m/s");
        rainUnit   = tr("mm/h");
        rainGrades = generateMapGrades(RAIN_MAP_LAYER_GRADES_MM, nullF);
        windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, nullF);
        tempGrades = generateMapGrades(TEMP_MAP_LAYER_GRADES_CELSIUS, nullF);
        break;
      case Units::CUSTOM:
        isMetric   = (m_config.windUnits == WindUnits::METSEC  || m_config.windUnits == WindUnits::KMHR || m_config.windUnits == WindUnits::KNOTS) ? "true":"false";
        isImperial = (m_config.windUnits == WindUnits::FEETSEC || m_config.windUnits == WindUnits::MILHR) ? "true":"false";
        degrees    = m_config.tempUnits == TemperatureUnits::CELSIUS ? "ºC" : "ºF";
        switch(m_config.precUnits)
        {
          case PrecipitationUnits::INCH:
            rainUnit   = tr("inches/h");
            rainGrades = generateMapGrades(RAIN_MAP_LAYER_GRADES_MM, convertMmToInches);
            break;
          default:
          case PrecipitationUnits::MM:
            rainUnit   = tr("mm/h");
            rainGrades = generateMapGrades(RAIN_MAP_LAYER_GRADES_MM, nullF);
            break;
        }
        switch(m_config.windUnits)
        {
          case WindUnits::FEETSEC:
            windUnit = tr("ft/s");
            windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, convertMetersSecondToFeetSecond);
            break;
          case WindUnits::KMHR:
            windUnit = tr("km/h");
            windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, convertMetersSecondToKilometersHour);
            break;
          case WindUnits::MILHR:
            windUnit = tr("mph");
            windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, convertMetersSecondToMilesHour);
            break;
          case WindUnits::KNOTS:
            windUnit = tr("kts");
            windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, convertMetersSecondToKnots);
            break;
          case WindUnits::METSEC:
            windUnit = tr("m/s");
            windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, nullF);
            break;
        }
        switch(m_config.tempUnits)
        {
          case TemperatureUnits::FAHRENHEIT:
            tempGrades = generateMapGrades(TEMP_MAP_LAYER_GRADES_CELSIUS, convertCelsiusToFahrenheit);
            break;
          default:
          case TemperatureUnits::CELSIUS:
            tempGrades = generateMapGrades(TEMP_MAP_LAYER_GRADES_CELSIUS, nullF);
            break;
        }
        break;
    }

    webpage.replace("%%metric%%", isMetric);
    webpage.replace("%%imperial%%", isImperial);
    webpage.replace("%%degrees%%", degrees);
    webpage.replace("%%windUnit%%", windUnit);
    webpage.replace("%%rainUnit%%", rainUnit);
    webpage.replace("%%rainGrades%%", rainGrades);
    webpage.replace("%%windGrades%%", windGrades);
    webpage.replace("%%tempGrades%%", tempGrades);

    webpage.replace("%%tempStr%%", tr("Temperature"), Qt::CaseSensitive);
    webpage.replace("%%rainStr%%", tr("Rain"), Qt::CaseSensitive);
    webpage.replace("%%windStr%%", tr("Wind"), Qt::CaseSensitive);
    webpage.replace("%%cloudStr%%", tr("Clouds"), Qt::CaseSensitive);

    webpage.replace("%%cloudsOpacity%%", QString::number(m_config.cloudMapOpacity, 'f', 2), Qt::CaseSensitive);
    webpage.replace("%%rainOpacity%%", QString::number(m_config.rainMapOpacity, 'f', 2), Qt::CaseSensitive);
    webpage.replace("%%windOpacity%%", QString::number(m_config.windMapOpacity, 'f', 2), Qt::CaseSensitive);
    webpage.replace("%%tempOpacity%%", QString::number(m_config.tempMapOpacity, 'f', 2), Qt::CaseSensitive);

    // config
    webpage.replace("%%lat%%", QString::number(m_config.latitude), Qt::CaseSensitive);
    webpage.replace("%%lon%%", QString::number(m_config.longitude), Qt::CaseSensitive);
    webpage.replace("{api_key}", m_apiKey, Qt::CaseSensitive);
    webpage.replace("%%layermap%%", m_config.lastLayer, Qt::CaseSensitive);
    webpage.replace("%%streetmap%%", m_config.lastStreetLayer, Qt::CaseSensitive);

    webfile.close();

    return webpage;
  }

  const auto message = tr("Unable to load weather webpage");
  return QString("<p style=\"color:red\"><h1>%1</h1></p>").arg(message);
}

//----------------------------------------------------------------------------
void OWM25Provider::processReply(QNetworkReply *reply)
{
  const auto originUrl = reply->request().url().toString();
  const auto contents  = reply->readAll();

  if(contents.contains(INVALID_MSG.toLocal8Bit()))
  {
    emit apiKeyValid(false);
    return;
  }

  if(originUrl.contains("pollution", Qt::CaseInsensitive))
  {
    if(reply->error() == QNetworkReply::NoError)
    {
      processPollutionData(contents);
    }
    else
    {
      const auto msg = tr("Error: ") + tr("No pollution data.") + QString("\n%1").arg(reply->errorString());
      emit errorMessage(msg);
    }
  }
  else
  {
    if(originUrl.contains("geo", Qt::CaseInsensitive))
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
    else if (originUrl.contains("openweathermap", Qt::CaseInsensitive))
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
void OWM25Provider::searchLocations(const QString &text, std::shared_ptr<QNetworkAccessManager> netManager) const
{
  auto url = QUrl{QString("http://api.openweathermap.org/geo/1.0/direct?q=%1&limit=5&appid=%2").arg(text).arg(m_apiKey)};
  netManager->get(QNetworkRequest{url});
}

//----------------------------------------------------------------------------
void OWM25Provider::loadSettings()
{
  QSettings settings = applicationSettings();
  m_apiKey = settings.value(OWM25Provider::OPENWEATHERMAP_APIKEY, QString()).toString();
}

//----------------------------------------------------------------------------
void OWM25Provider::saveSettings()
{
  if(!m_apiKey.isEmpty())
  {
    QSettings settings = applicationSettings();
    settings.setValue(OWM25Provider::OPENWEATHERMAP_APIKEY, m_apiKey);
  }
}

//----------------------------------------------------------------------------
void OWM25Provider::processWeatherData(const QByteArray &contents)
{
  const auto jsonDocument = QJsonDocument::fromJson(contents);

  if(!jsonDocument.isNull() && jsonDocument.isObject())
  {
    // to discard entries older than 'right now'.
    const auto currentDt = std::chrono::duration_cast<std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto jsonObj = jsonDocument.object();

    const auto keys = jsonObj.keys();

    if(keys.contains("cnt"))
    {
      m_forecast.clear();

      const auto values  = jsonObj.value("list").toArray();

      auto hasEntry = [this](unsigned long dt) { for(auto entry: this->m_forecast) if(entry.dt == dt) return true; return false; };

      for(auto i = 0; i < values.count(); ++i)
      {
        auto entry = values.at(i).toObject();

        const auto dt = entry.value("dt").toInt(0);
        if(dt < currentDt) continue;

        ForecastData data;
        parseForecastEntry(entry, data);

        if(!hasEntry(data.dt))
        {
          m_forecast << data;
          if(!m_config.useGeolocation)
          {
            if(data.name    != "Unknown") m_config.region = m_config.city = data.name;
            if(data.country != "Unknown") m_config.country = data.country;
          }
        }
      }

      if(!m_forecast.isEmpty())
      {
        auto lessThan = [](const ForecastData &left, const ForecastData &right) { if(left.dt < right.dt) return true; return false; };
        qSort(m_forecast.begin(), m_forecast.end(), lessThan);
      }

      if(!m_apiKeyValid) emit apiKeyValid(true);

      emit weatherForecastDataReady();
    }
    else
    {
      parseForecastEntry(jsonObj, m_current);
      if(!m_config.useGeolocation)
      {
        if(m_current.name    != "Unknown") m_config.region = m_config.city = m_current.name;
        if(m_current.country != "Unknown") m_config.country = m_current.country;
      }

      if(!m_apiKeyValid) emit apiKeyValid(true);

      emit weatherDataReady();
    }

    if(keys.contains("alerts"))
    {
      const auto alertObj = jsonObj.value("alerts").toObject();

      Alert alert;
      alert.sender = alertObj.value("sender_name").toString();
      alert.event = alertObj.value("event").toString();
      alert.startTime = alertObj.value("start").toVariant().toULongLong();
      alert.endTime = alertObj.value("end").toVariant().toULongLong();
      alert.description = alertObj.value("description").toString();

      Alerts alertList;
      alertList << alert;
      emit weatherAlert(alertList);
    }
  }
}

//----------------------------------------------------------------------------
void OWM25Provider::processPollutionData(const QByteArray &contents)
{
  const auto jsonDocument = QJsonDocument::fromJson(contents);
  m_pollution.clear();

  if(!jsonDocument.isNull() && jsonDocument.isObject())
  {
    // to discard entries older than 'right now'.
    const auto currentDt = std::chrono::duration_cast<std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto jsonObj = jsonDocument.object();
    const auto values  = jsonObj.value("list").toArray();

    auto hasEntry = [this](unsigned long dt) { for(auto entry: this->m_pollution) if(entry.dt == dt) return true; return false; };

    for(auto i = 0; i < values.count(); ++i)
    {
      auto entry = values.at(i).toObject();

      const auto dt = entry.value("dt").toInt(0);
      if(dt < currentDt) continue;

      PollutionData data;
      parsePollutionEntry(entry, data);

      if(!hasEntry(data.dt)) m_pollution << data;
    }

    if(!m_pollution.isEmpty())
    {
      auto lessThan = [](const PollutionData &left, const PollutionData &right) { if(left.dt < right.dt) return true; return false; };
      qSort(m_pollution.begin(), m_pollution.end(), lessThan);
    }

    emit pollutionForecastDataReady();
  }
}

//----------------------------------------------------------------------------
void OWM25Provider::processLocationsData(const QByteArray &contents)
{
  const auto jsonDocument = QJsonDocument::fromJson(contents);

  if (!jsonDocument.isNull() && jsonDocument.isArray())
  {
    // OpenWeatherMap Geolocation JSON keys.
    const QString NAME_KEY = "name";
    const QString LOCAL_NAMES_KEY = "local_names";
    const QString LATITUDE_KEY = "lat";
    const QString LONGITUDE_KEY = "lon";
    const QString COUNTRY_KEY = "country";
    const QString STATE_KEY = "state";

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
      const auto language = m_config.language.split("_").first();
      const auto locListObj = locObject.value(LOCAL_NAMES_KEY).toObject();

      Location locationData;

      locationData.location = locObject.value(NAME_KEY).toString();
      locationData.translated = locListObj.value(language).toString();
      locationData.latitude = locObject.value(LATITUDE_KEY).toDouble();
      locationData.longitude = locObject.value(LONGITUDE_KEY).toDouble();
      locationData.country = locObject.value(COUNTRY_KEY).toString();

      auto country = locObject.value(COUNTRY_KEY).toString();
      if (!country.isEmpty())
      {
        const auto countryCode = std::find_if(ISO3166.cbegin(), ISO3166.cend(), [&country](const QString &code)
                                              { return code.compare(country, Qt::CaseInsensitive) == 0; });
        if (countryCode != ISO3166.cend())
          country = (ISO3166.key(*countryCode));
      }
      locationData.country = country;
      locationData.region = locObject.value(STATE_KEY).toString();

      locations << locationData;
    }

    emit foundLocations(locations);
  }
}

//--------------------------------------------------------------------
void OWM25Provider::parseForecastEntry(const QJsonObject& entry, ForecastData& data)
{
  const auto keys = entry.keys();
  if(!keys.contains("main"))
    return;
    
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
void OWM25Provider::parsePollutionEntry(const QJsonObject &entry, PollutionData &data)
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
    case 1:  data.aqi_text = tr("Good"); break;
    case 2:  data.aqi_text = tr("Fair"); break;
    case 3:  data.aqi_text = tr("Moderate"); break;
    case 4:  data.aqi_text = tr("Poor"); break;
    default: data.aqi_text = tr("Very poor"); break;
  }
}

//----------------------------------------------------------------------------
void OWM25Provider::testApiKey(std::shared_ptr<QNetworkAccessManager> netManager)
{
  // protect from abuse
  static auto lastRequest = std::chrono::steady_clock::now();
  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<double> interval = now - lastRequest;
  const auto duration = interval.count();

  if(duration > 0.1 && duration < 2.5) return;
  lastRequest = now;

  auto url = QUrl{QString("http://api.openweathermap.org/data/2.5/weather?lat=%1&lon=%2&appid=%3").arg(m_config.latitude)
                                                                                                  .arg(m_config.longitude)
                                                                                                  .arg(m_apiKey)};
  netManager->get(QNetworkRequest{url});
}

// Open-Meteo API ------------------------------------------------------------
//----------------------------------------------------------------------------
void OpenMeteoProvider::requestData(std::shared_ptr<QNetworkAccessManager> netManager) const
{
  // Weather & UV forecast.
  auto url = QUrl{QString("https://api.open-meteo.com/v1/forecast?latitude=%1&longitude=%2"
                          "&current=temperature_2m,relative_humidity_2m,rain,snowfall,weather_code,"
                          "cloud_cover,surface_pressure,wind_speed_10m,wind_direction_10m"
                          "&hourly=temperature_2m,relative_humidity_2m,rain,snowfall,weather_code,"
                          "surface_pressure,cloud_cover,wind_speed_10m,wind_direction_10m,uv_index"
                          "&timeformat=unixtime").arg(m_config.latitude).arg(m_config.longitude)};

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

  auto url = QUrl{QString("https://geocoding-api.open-meteo.com/v1/search?name=%1&count=10&language=%2&format=json").arg(text).arg(lang)};
  netManager->get(QNetworkRequest{url});
}

//----------------------------------------------------------------------------
void OpenMeteoProvider::processWeatherData(const QByteArray &contents)
{
  const auto jsonDocument = QJsonDocument::fromJson(contents);

  if(!jsonDocument.isNull() && jsonDocument.isObject())
  {
    // to discard entries older than 'right now'.
    const auto currentDt = std::chrono::duration_cast<std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto jsonObj = jsonDocument.object();

    const auto keys = jsonObj.keys();

    if(keys.contains("current"))
    {
      const auto current = jsonObj.value("current").toObject();

      m_current.dt         = current.value("time").toInt(0);
      auto dayInterval     = computeSunriseSunset(m_current, m_config.longitude, m_config.latitude);
      m_current.sunrise    = dayInterval.first;
      m_current.sunset     = dayInterval.second;

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
      m_current.name       = "Unknown";
      m_current.country    = "Unknown";      

      fillWMOCodeInForecast(m_current);

      emit weatherDataReady();
    }

    if(keys.contains("hourly"))
    {
      m_forecast.clear();
      m_uv.clear();

      if(m_current.isValid())
        m_forecast << m_current;

      const auto hourly = jsonObj.value("hourly").toObject();        

      const auto times        = hourly.value("time").toObject();
      const auto temperatures = hourly.value("temperature_2m").toObject();
      const auto humidities   = hourly.value("relative_humidity_2m").toObject();
      const auto rain         = hourly.value("rain").toObject();
      const auto snow         = hourly.value("snowfall").toObject();
      const auto owmCodes     = hourly.value("weather_code").toObject();
      const auto pressures    = hourly.value("surface_pressure").toObject();
      const auto clouds       = hourly.value("cloud_cover").toObject();
      const auto windSpeeds   = hourly.value("wind_speed_10m").toObject();
      const auto windDirs     = hourly.value("wind_direction_10m").toObject();
      const auto uvIndexes    = hourly.value("uv_index").toObject();

      auto hasEntry = [this](unsigned long dt) { for(auto entry: this->m_forecast) if(entry.dt == dt) return true; return false; };
      auto hasUVEntry = [this](unsigned long dt) { for(auto entry: this->m_uv) if(entry.dt == dt) return true; return false; };

      for(auto i = 0; i < times.count(); ++i)
      {
        const auto key = QString("%1").arg(i);
        const auto dt = times.value(key).toInt(0);
        if(dt < currentDt) continue;

        UVData uvData;
        uvData.dt = dt;
        uvData.idx = uvIndexes.value(key).toDouble(0);

        if(!hasUVEntry(uvData.dt))
          m_uv << uvData;

        ForecastData data;
        data.dt          = dt;
        auto dayInterval = computeSunriseSunset(data, m_config.longitude, m_config.latitude);
        data.sunrise     = dayInterval.first;
        data.sunset      = dayInterval.second;

        data.cloudiness  = clouds.value(key).toDouble(0);
        data.temp        = temperatures.value(key).toDouble(0);
        data.temp_min    = data.temp;
        data.temp_max    = data.temp;
        data.humidity    = humidities.value(key).toDouble(0);
        data.pressure    = pressures.value(key).toDouble(0);
        data.weather_id  = owmCodes.value(key).toInt(0);
        data.wind_speed  = windSpeeds.value(key).toDouble(0);
        data.wind_dir    = windDirs.value(key).toDouble(0);
        data.snow        = snow.value(key).toDouble(0) * 10; // units are cm not mm
        data.rain        = rain.value(key).toDouble(0);
        data.name        = "Unknown";
        data.country     = "Unknown";      

        fillWMOCodeInForecast(data);

        if(!hasEntry(data.dt))
          m_forecast << data;
      }

      if(!m_forecast.isEmpty())
      {
        auto lessThan = [](const ForecastData &left, const ForecastData &right) { if(left.dt < right.dt) return true; return false; };
        qSort(m_forecast.begin(), m_forecast.end(), lessThan);
        emit weatherForecastDataReady();
      }

      if(!m_uv.isEmpty())
      {
        auto lessThan = [](const UVData &left, const UVData &right) { if(left.dt < right.dt) return true; return false; };
        qSort(m_uv.begin(), m_uv.end(), lessThan);
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
      data.dt    = current.value("time").toInt(0);
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

      const auto times = hourly.value("time").toObject();
      const auto pm10s = hourly.value("pm10").toObject();
      const auto pm25s = hourly.value("pm2_5").toObject();
      const auto COs   = hourly.value("carbon_monoxide").toObject();
      const auto NO2s  = hourly.value("nitrogen_dioxide").toObject();
      const auto SO2s  = hourly.value("sulphur_dioxide").toObject();
      const auto O3s   = hourly.value("ozone").toObject();
      const auto NH3s  = hourly.value("ammonia").toObject();
      const auto AQIs  = hourly.value("european_aqi").toObject();

      for(auto i = 0; i < times.count(); ++i)
      {
        const auto key = QString("%1").arg(i);
        const auto dt = times.value(key).toInt(0);
        if(dt < currentDt) continue;

        PollutionData data;
        data.dt    = times.value(key).toInt(0);
        data.aqi   = AQIs.value(key).toDouble(20);
        data.co    = COs.value(key).toDouble(0);
        data.no    = 0;
        data.no2   = NO2s.value(key).toDouble(0);
        data.o3    = O3s.value(key).toDouble(0);
        data.so2   = SO2s.value(key).toDouble(0);
        data.pm2_5 = pm25s.value(key).toDouble(0);
        data.pm10  = pm10s.value(key).toDouble(0);
        data.nh3   = NH3s.value(key).toDouble(0);
        processAQIvalue(data);

        if(!hasEntry(data.dt))
          m_pollution << data;
      }
    }

    if (!m_forecast.isEmpty())
    {
      auto lessThan = [](const PollutionData &left, const PollutionData &right){ if(left.dt < right.dt) return true; return false; };
      qSort(m_pollution.begin(), m_pollution.end(), lessThan);
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
      const QString msg = tr("No locations found for '%1'.");
      emit errorMessage(msg);
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
      location.region = extra.join('/');

      locations << location;
    }

    emit foundLocations(locations);
  }
}

