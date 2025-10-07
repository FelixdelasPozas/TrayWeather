/*
 File: WeatherAPI.h
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

#ifndef _WEATHERAPI_PROVIDER_H_
#define _WEATHERAPI_PROVIDER_H_

// Project
#include <Provider.h>

// C++
#include <memory>

// Qt
#include <QString>

class QNetworkAccessManager;
class QByteArray;

static const QString WEATHERAPI_PROVIDER = "WeatherAPI API";

/** \class WeatherAPI
 * \brief Weather provider that uses WeatherAPI API.
 *
 */
class WeatherAPIProvider
: public WeatherProvider
{
    Q_OBJECT
  public:
    /** \brief WeatherAPI class constructor.
     * \param[in] config Application configuration information.
     *
     */
    WeatherAPIProvider(Configuration &config)
    : WeatherProvider(WEATHERAPI_PROVIDER, config)
    {
      loadSettings();
    };

    /** \brief WeatherAPI class virtual destructor.
     *
     */
    virtual ~WeatherAPIProvider()
    {
      saveSettings();
    };

    virtual ProviderCapabilities capabilities() const override
    { return ProviderCapabilities(true, true, true, true, false, true, true, true); }

    virtual QString apikey() const override
    { return m_apiKey; };

    virtual void setApiKey(const QString &key) override;

    virtual QString name() const override
    { return "WeatherAPI"; };

    virtual QString website() const override
    { return "https://www.weatherapi.com/"; };

    virtual void requestData(std::shared_ptr<QNetworkAccessManager> netManager) override;
    virtual void testApiKey(std::shared_ptr<QNetworkAccessManager> netManager) override;
    virtual void processReply(QNetworkReply *reply) override;
    virtual void searchLocations(const QString &text, std::shared_ptr<QNetworkAccessManager> netManager) const override;

  protected:
    virtual void loadSettings() override;
    virtual void saveSettings() override;

  private:
    const QString WEATHERAPI_APIKEY = QString("WeatherAPI API Key");
    const QString INVALID_MSG = QString("API key is invalid.");

    /** \brief Processes the weather forecast data stream.
     * \param[in] contents Contents of the network reply.
     *
     */
    void processWeatherData(const QByteArray &contents);

    /** \brief Processes the locations data stream.
     * \param[in] contents Contents of the network reply.
     *
     */
    void processLocationsData(const QByteArray &contents);

    /** \brief Parses the information in the entry to the weather data object.
    * \param[in] entry JSON object.
    * \param[out] data ForecastData struct.
    *
    */
    void parseForecastEntry(const QJsonObject &entry, ForecastData &data);

    /** \brief Parses the information in the entry to the pollution data object.
    * \param[in] entry JSON object.
    * \param[in] data PollutionData struct.
    *
    */
    void parsePollutionEntry(const QJsonObject &entry, PollutionData &data);

    /** \brief Helper method to convert the WeatherAPI code inside to fill a ForecastData object fields. 
    *         WweatherAPI codes are used to represent weather conditions (meteorological codes for use at observing stations).
    * \param[in] forecast ForecastData object reference, contains wmo code.
    *
    */
    void fillWAPICodeInForecast(ForecastData &forecast);

    QString m_apiKey; /** provider api key */
};

#endif