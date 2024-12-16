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

// C++
#include <chrono>

// OpenWeatherMap Geolocation JSON keys.
const QString NAME_KEY = "name";
const QString LOCAL_NAMES_KEY = "local_names";
const QString LATITUDE_KEY = "lat";
const QString LONGITUDE_KEY = "lon";
const QString COUNTRY_KEY = "country";
const QString STATE_KEY = "state";

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

//----------------------------------------------------------------------------
OWM25Provider::OWM25Provider(Configuration &config)
: WeatherProvider(OWM_25_PROVIDER, config)
{
}

//----------------------------------------------------------------------------
ProviderCapabilities OWM25Provider::capabilities() const
{
  return ProviderCapabilities(true, true, true, false, true, true, true);
}

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
QString OWM25Provider::apikey() const
{
  return m_apiKey;
}

//----------------------------------------------------------------------------
void OWM25Provider::setApiKey(const QString &key)
{
  if(!key.isEmpty())
    m_apiKey = key;
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
QString OWM25Provider::name() const
{
  return "OpenWeatherMap";
}

//----------------------------------------------------------------------------
QString OWM25Provider::website() const
{
  return "https://openweathermap.org/appid";
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

//----------------------------------------------------------------------------
std::unique_ptr<WeatherProvider> WeatherProviderFactory::createProvider(const QString &name, Configuration &config)
{
  std::unique_ptr<WeatherProvider> provider = nullptr;

  if(name.compare(OWM_25_PROVIDER) == 0)
    provider = std::make_unique<OWM25Provider>(config);
  
  return provider;
}
