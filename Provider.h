/*
 File: Provider.h
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

#ifndef _WEATHER_PROVIDER_H_
#define _WEATHER_PROVIDER_H_

// Qt
#include <QObject>
class QNetworkAccessManager;
class QNetworkReply;

// Project
#include <Utils.h>
class Configuration;

// C++
#include <memory>

// List of providers
static const QString OWM_25_PROVIDER = "OpenWeatherMap2.5";
static const QStringList PROVIDERS = { OWM_25_PROVIDER };

/** \struct ProviderCapabilities
 * \brief Describes the capabilities of the weather provider.
 */
struct ProviderCapabilities
{
  bool hasCurrentWeather;    /** true if provides current weather and false otherwise. */
  bool hasWeatherForecast;   /** true if provides weather forecast and false otherwise. */
  bool hasPollutionForecast; /** true if provides pollution forecast and false otherwise. */
  bool hasUVForecast;        /** true if provides UV forecast and false otherwise. */
  bool hasMaps;              /** true if provides interactive maps. */
  bool requiresKey;          /** true if the provider requires a key to request data. */

  /** \brief ProviderCapabilities struct constructor. 
   * \param[in] weather true if provides current weather and false otherwise.
   * \param[in] forecast true if provides weather forecast and false otherwise.
   * \param[in] pollution true if provides pollution forecast and false otherwise.
   * \param[in] uv true if provides UV forecast and false otherwise.
   * \param[in] maps true if provides interactive maps.
   * \param[in] key true if the provider requires a key to request data.
   */
  ProviderCapabilities(const bool weather, const bool forecast, const bool pollution, const bool uv, const bool maps, const bool key)
  : hasCurrentWeather{weather}, hasWeatherForecast{forecast}, hasPollutionForecast{pollution}, hasUVForecast{uv}, hasMaps{maps}, requiresKey{key}
  {};

  /** \brief ProviderCapabilities struct empty constructor.
   */
  ProviderCapabilities()
  : ProviderCapabilities(false, false, false, false, false, false)
  {};
};

/**
 * @class WeatherProvider
 * @brief Base class of weather providers.
 */
class WeatherProvider
: public QObject
{
    Q_OBJECT
  public:
    /** \brief WeatherProvider class constructor.
     * \param name Provider name.
     * \param config Application configuration reference. 
     */
    WeatherProvider(const QString &name, Configuration &config);

    /** \brief WeatherProvider class virtual destructor.
     *
     */
    virtual ~WeatherProvider();

    /** \brief Returns the weather provider capabilities.
     *
     */
    virtual ProviderCapabilities capabilities() const
    { return ProviderCapabilities(); };

    /** \brief Returns the current weather data. 
     *    
     */
    virtual ForecastData weather() const
    { return m_current; };

    /** \brief Returns the weather forecast data. 
     *    
     */
    virtual Forecast weatherForecast() const
    { return m_forecast; };

    /** \brief Returns the pollution forecast data. 
     *    
     */
    virtual Pollution pollutionForecast() const
    { return m_pollution; };

    /** \brief Returns the UV forecast data. 
     *    
     */
    virtual UV uvForecast() const
    { return m_uv; };

    /** \brief Requests weather data.
     * \param[in] netManager Application network manager. 
     *
     */
    virtual void requestData(std::shared_ptr<QNetworkAccessManager> netManager) const
    {};

    /** \brief Processes the webpage doing the necessary substitutions.
     * \param[in] webpage Maps webpage code. 
     *
     */
    virtual QString mapsPage() const
    { return QString(); };

    /**
     * @brief Returns the api key for the weather provider.
     */
    virtual QString apikey() const
    { return QString(); };

    /**
     * @brief Sets the api key for the provider.
     * @param key api key the for the weather provider.
     */
    virtual void setApiKey(const QString &key)
    {};

    /** \brief Tests the api key valdity, if the provider needs one.
     * \param[in] netManager Application network manager. 
     *
     */
    virtual void testApiKey(std::shared_ptr<QNetworkAccessManager> netManager)
    {};

    /**
     * @brief Returns the name of the provider.
     */
    QString name() const
    { return m_name; };

    /** \brief Processes the information of the network reply.
     * \param[in] reply Network reply information.
     */
    virtual void processReply(QNetworkReply *reply) = 0;

  signals:
    void weatherDataReady();
    void weatherForecastDataReady();
    void pollutionForecastDataReady();
    void uvForecastDataReady();
    void apiKeyValid(bool);
    void errorMessage(const QString &);

  protected:
    /** \brief Loads provider settings from the registry.
     */
    virtual void loadSettings(){};

    /** \brief Saves provider settings to the registry.
     */
    virtual void saveSettings(){};

    const QString m_name;    /** Weather provider name */
    Configuration &m_config; /** Application configuration */
    bool m_keyValid;         /** true when checking the api key and false otherwise. */
    ForecastData m_current;  /** current weather data. */
    Forecast m_forecast;     /** weather forecast data. */
    Pollution m_pollution;   /** pollution forecast data. */
    UV m_uv;                 /** uv forecast data. */
};

/**
 * @class OWM25Provider
 * @brief Weather provider that uses OpenWeatherMap 2.5 API.
 */
class OWM25Provider
: public WeatherProvider
{
    Q_OBJECT
  public:
    /**
     * @brief OMV25Provider class constructor.
     * @param config Application configuration information.
     */
    OWM25Provider(Configuration &config);

    /**
     * @brief OWM25Provider class virtual destructor.
     */
    virtual ~OWM25Provider()
    {};

    virtual ProviderCapabilities capabilities() const override;
    virtual void requestData(std::shared_ptr<QNetworkAccessManager> netManager) const override;
    virtual QString mapsPage() const override;
    virtual QString apikey() const override;
    virtual void setApiKey(const QString &key) override;
    virtual void testApiKey(std::shared_ptr<QNetworkAccessManager> netManager) override;
    virtual void processReply(QNetworkReply *reply) override;

  protected:
    virtual void loadSettings() override;
    virtual void saveSettings() override;

  private:
    const QString OPENWEATHERMAP_APIKEY = QString("OpenWeatherMap API Key");
    const QString INVALID_MSG = QString("Invalid API key");

    /** \brief Processes the weather forecast data stream.
     * \param contents Contents of network reply.
     *
     */
    void processWeatherData(const QByteArray &contents);

    /** \brief Processes the pollution data stream.
     * \param contents Contents of network reply.
     *
     */
    void processPollutionData(const QByteArray &contents);

    QString m_apiKey;       /** provider api key */
    bool m_apiKeyValid;     /** true if the api key is valid and false otherwise. */
};

/** \class WeatherProviderFactory
 * \brief Factory of weather providers. 
 */
namespace WeatherProviderFactory
{
    /** \brief Creates and returns the given weather provider or nullptr if not present. 
     * \param name Name of the weather provider. 
     * \param config Application configuration reference. 
     */
    std::unique_ptr<WeatherProvider> createProvider(const QString &name, Configuration &config);
};

#endif