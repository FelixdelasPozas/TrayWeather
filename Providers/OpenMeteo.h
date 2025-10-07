/*
 File: OpenMeteo.h
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

#ifndef __OPEN_METEO_PROVIDER_H_
#define __OPEN_METEO_PROVIDER_H_

// Project
#include <Provider.h>

// C++
#include <memory>

// Qt
#include <QString>

class QNetworkAccessManager;
class QByteArray;

static const QString OPENMETEO_PROVIDER = "OpenMeteo API";

/** \class OpenMeteoProvider
 * \brief Weather provider that uses OpenMeteo API.
 *
 */
class OpenMeteoProvider
: public WeatherProvider
{
    Q_OBJECT
  public:
    /** \brief OpenMeteo class constructor.
     * \param[in] config Application configuration information.
     *
     */
    OpenMeteoProvider(Configuration &config)
    : WeatherProvider(OPENMETEO_PROVIDER, config)
    {};

    /** \brief OWM25Provider class virtual destructor.
     *
     */
    virtual ~OpenMeteoProvider()
    {};

    virtual ProviderCapabilities capabilities() const override
    { return ProviderCapabilities(true, true, true, true, false, true, false, false); };

    virtual QString name() const override
    { return "Open-Meteo"; };

    virtual QString website() const override
    { return "https://open-meteo.com/"; };

    virtual void requestData(std::shared_ptr<QNetworkAccessManager> netManager) override;
    virtual void processReply(QNetworkReply *reply) override;
    virtual void searchLocations(const QString &text, std::shared_ptr<QNetworkAccessManager> netManager) const override;

  private:
    const QString INVALID_MSG = QString("\"error\": true,");

    /** \brief Processes the weather forecast data stream.
     * \param[in] contents Contents of the network reply.
     *
     */
    void processWeatherData(const QByteArray &contents);

    /** \brief Processes the pollution data stream.
     * \param[in] contents Contents of the network reply.
     *
     */
    void processPollutionData(const QByteArray &contents);

    /** \brief Processes the locations data stream.
     * \param[in] contents Contents of the network reply.
     *
     */
    void processLocationsData(const QByteArray &contents);

    /** \brief Helper method to convert the wmo code inside to fill a ForecastData object fields. 
    *         WMO codes are used to represent weather conditions (meteorological codes for use at observing stations).
    * \param[in] forecast ForecastData object reference, contains wmo code.
    *
    */
    void fillWMOCodeInForecast(ForecastData &forecast);
};

#endif