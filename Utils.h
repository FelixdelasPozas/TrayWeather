/*
		File: Utils.h
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


#ifndef UTILS_H_
#define UTILS_H_

// C++
#include <time.h>

// Qt
#include <QString>
#include <QDebug>
#include <QJsonObject>

enum class Temperature: char { CELSIUS = 0, FAHRENHEIT };

/** \struct Configuration
 * \brief Contains the application configuration.
 *
 */
struct Configuration
{
    double latitude;         /** location latitude in degrees.  */
    double longitude;        /** location longitude in degrees. */
    QString country;         /** location's country.            */
    QString region;          /** location's region.             */
    QString city;            /** location's city.               */
    QString zipcode;         /** location's zip code.           */
    QString isp;             /** internet service provider.     */
    QString ip;              /** internet address.              */
    QString timezone;        /** location's timezone.           */
    QString owm_apikey;      /** OpenWeatherMap API Key.        */
    Temperature units;       /** temperature units.             */
    unsigned int updateTime; /** time between updates.  */

    bool isValid() const
    {
      return (latitude <= 90.0) &&   (latitude >= -90.0) &&
             (longitude <= 180.0) && (longitude >= -180) &&
             !owm_apikey.isEmpty();
    }
};

/** \struct ForecastData
 * \brief Contains the forecast data for a given time.
 *
 */
struct ForecastData
{
    long long int dt;          /** date and time of the data.         */
    double        temp;        /** temperature in Kelvin.             */
    double        temp_max;    /** max temperature in Kelvin.         */
    double        temp_min;    /** min temperature in Kelvin.         */
    double        cloudiness;  /** cloudiness %                       */
    double        humidity;    /** humidity %                         */
    double        pressure;    /** pressure on the ground.            */
    QString       description; /** weather description.               */
    QString       icon_id;     /** weather icon id.                   */
    double        weather_id;  /** weather identifier.                */
    QString       parameters;  /** weather parameters (rain, snow...) */
    double        wind_speed;  /** wind speed.                        */
    double        wind_dir;    /** wind direction in degrees.         */
    double        rain;        /** rain accumulation in last 3 hours. */
    double        snow;        /** snow accumulation in last 3 hours. */

    ForecastData(): dt{0}, temp{0}, temp_max{0}, temp_min{0}, cloudiness{0}, humidity{0}, pressure{0}, weather_id{0}, wind_speed{0}, wind_dir{0}, rain{0}, snow{0} {};

    bool isValid() { return dt != 0; };
};

using Forecast = QList<ForecastData>;

/** \brief Returns the icon corresponding to the given data.
 * \param[in] data forecast data struct.
 *
 */
const QIcon weatherIcon(const ForecastData &data);

/** \brief Parses the information in the entry to the data object.
 * \param[in] entry JSON object.
 * \param[out] data Forecast struct.
 *
 */
void parseForecastEntry(const QJsonObject &entry, ForecastData &data);

/** \brief Prints the contents of the data to the QDebug stream.
 * \param[in] debug QDebug stream.
 * \param[in] data ForecastData struct.
 *
 */
QDebug operator<< (QDebug d, const ForecastData &data);

/** \brief Converts the given temp to the given units and returns the value.
 * \param[in] temp temperature in Kelvin.
 * \param[in] units units to convert to.
 *
 */
const double convertKelvinTo(const double temp, const Temperature units);

/** \brief Converts the given unix timestamp to a struct tm and returns it.
 * \param[in] timestamp unix timestamp.
 *
 */
const struct tm* unixTimeStampToDate(const long long timestamp);

/** \brief Returns the moon phase for the given date (given in unix timestamp) Answer range [0-7] (0 = new moon, 4 = full moon).
 * \param[in] timestamp unix timestamp.
 *
 */
int moonPhase(const time_t timestamp);

/** \brief Transforms and returns the string in title case.
 * \param[in] string string to transform.
 *
 */
const QString toTitleCase(const QString &string);

#endif // UTILS_H_
