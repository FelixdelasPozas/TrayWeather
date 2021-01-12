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
#include <QColor>

enum class Temperature: char { CELSIUS = 0, FAHRENHEIT };

/** \struct Configuration
 * \brief Contains the application configuration.
 *
 */
struct Configuration
{
    double       latitude;       /** location latitude in degrees.                               */
    double       longitude;      /** location longitude in degrees.                              */
    QString      country;        /** location's country.                                         */
    QString      region;         /** location's region.                                          */
    QString      city;           /** location's city.                                            */
    QString      zipcode;        /** location's zip code.                                        */
    QString      isp;            /** internet service provider.                                  */
    QString      ip;             /** internet address.                                           */
    QString      timezone;       /** location's timezone.                                        */
    QString      owm_apikey;     /** OpenWeatherMap API Key.                                     */
    Temperature  units;          /** temperature units.                                          */
    unsigned int updateTime;     /** time between updates.                                       */
    bool         mapsEnabled;    /** true if maps tab is visible, false otherwise.               */
    bool         useDNS;         /** true to use DNS address for geo location instead of own IP. */
    bool         useGeolocation; /** true to use the ip-api.com services, false to use manual.   */
    bool         roamingEnabled; /** true if georaphical coordinates are asked on each forecast. */
    bool         lightTheme;     /** true if light theme is being used, false if dark theme.     */
    unsigned int iconType;       /** 0 if just icon, 1 if just temperature, 2 if both.           */
    QColor       trayTextColor;  /** Color of tray temperature text.                             */
    bool         trayTextMode;   /** true for fixed, false for dynamic.                          */
    QColor       minimumColor;   /** minimum value dynamic color.                                */
    QColor       maximumColor;   /** maximum value dynamic color.                                */
    int          minimumValue;   /** dynamic color minimum value.                                */
    int          maximumValue;   /** dynamic color maximum value.                                */


    /** \brief Configuration struct constructor.
     *
     */
    Configuration()
    : latitude      {-91}
    , longitude     {-181}
    , country       {"Unknown"}
    , region        {"Unknown"}
    , city          {"Unknown"}
    , zipcode       {"Unknown"}
    , isp           {"Unknown"}
    , ip            {"Unknown"}
    , timezone      {"Unknown"}
    , owm_apikey    {""}
    , units         {Temperature::CELSIUS}
    , updateTime    {0}
    , mapsEnabled   {false}
    , useDNS        {false}
    , useGeolocation{true}
    , roamingEnabled{false}
    , lightTheme    {true}
    , iconType      {0}
    , trayTextColor {Qt::white}
    , trayTextMode  {true}
    , minimumColor  {Qt::blue}
    , maximumColor  {Qt::red}
    , minimumValue  {-10}
    , maximumValue  {45}
    {};

    bool isValid() const
    {
      return (latitude <= 90.0) &&   (latitude >= -90.0) &&
             (longitude <= 180.0) && (longitude >= -180.0) &&
             !owm_apikey.isEmpty();
    }
};

/** \struct ForecastData
 * \brief Contains the weather forecast data for a given time.
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
    long long int sunrise;     /** time of sunrise.                   */
    long long int sunset;      /** time of sunset.                    */
    QString       name;        /** place name.                        */
    QString       country;     /** country.                           */

    ForecastData(): dt{0}, temp{0}, temp_max{0}, temp_min{0}, cloudiness{0}, humidity{0}, pressure{0},
                    weather_id{0}, wind_speed{0}, wind_dir{0}, rain{0}, snow{0}, sunrise{0}, sunset{0},
                    name{"Unknown"}, country{"Unknown"} {};

    bool isValid() const { return dt != 0 && !icon_id.isEmpty(); };
};

using Forecast = QList<ForecastData>;

/** \brief PollutionData
 * \brief Contains the pollution forecast data for a given time.
 *
 *   Air quality index = 1 Good, 2 Fair, 3 Moderate, 4 Poor and 5 Very Poor.
 *   Concentrations units: micro-grams/cubic meter.
 */
struct PollutionData
{
    long long int dt;       /** date and time of the data.                 */
    unsigned int  aqi;      /** air quality index in [1-5].                */
    QString       aqi_text; /** air quality as text.                       */
    double        co;       /** concentration of carbon monoxide.          */
    double        no;       /** concentration of nitrogen monoxide.        */
    double        no2;      /** concentration of nitrogen dioxide.         */
    double        o3;       /** concentration of ozone.                    */
    double        so2;      /** concentration of sulphur dioxide.          */
    double        pm2_5;    /** concentration of fine particles matter.    */
    double        pm10;     /** concentration of coarse particles martter. */
    double        nh3;      /** concentration of ammonia.                  */

    PollutionData(): dt{0}, aqi{1}, co{0}, no{0}, no2{0}, o3{0}, so2{0}, pm2_5{0}, pm10{0}, nh3{0} {};

    bool isValid() const { return dt != 0; }
};

using Pollution = QList<PollutionData>;

const QString CONCENTRATION_UNITS{"µg/m<sup>3</sup>"};

const QStringList CONCENTRATION_NAMES{"CO", "NO", "NO<sub>2</sub>", "O<sub>3</sub>", "SO<sub>2</sub>", "PM<sub>2.5</sub>", "PM<sub>10</sub>", "NH<sub>3</sub>"};

const QList<QColor> CONCENTRATION_COLORS{ QColor::fromHsv(0, 255, 255),   QColor::fromHsv(45, 255, 255),  QColor::fromHsv(90, 255, 255),
                                          QColor::fromHsv(135, 255, 255), QColor::fromHsv(180, 255, 255), QColor::fromHsv(225, 255, 255),
                                          QColor::fromHsv(270, 255, 255), QColor::fromHsv(315, 255, 255)};


/** \brief Returns the icon corresponding to the given data.
 * \param[in] data forecast data struct.
 *
 */
const QPixmap weatherPixmap(const ForecastData &data);

/** \brief Returns the moon phase icon corresponding to the given data.
 * \param[in] data forecast data struct.
 *
 */
const QPixmap moonPixmap(const ForecastData& data);

/** \brief Parses the information in the entry to the weather data object.
 * \param[in] entry JSON object.
 * \param[out] data ForecastData struct.
 * \param[in] unit temperature units.
 *
 */
void parseForecastEntry(const QJsonObject &entry, ForecastData &data, const Temperature unit);

/** \brief Parses the information in the entry to the pollution data object.
 * \param[in] entry JSON object.
 * \param[in] data PollutionData struct.
 *
 */
void parsePollutionEntry(const QJsonObject &entry, PollutionData &data);

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
 * \param[out] time struct tm.
 * \param[in] timestamp unix timestamp.
 *
 */
void unixTimeStampToDate(struct tm &time, const long long timestamp);

/** \brief Returns the moon phase for the given date (given in unix timestamp) Answer range [0-7] (0 = new moon, 4 = full moon).
 * \param[in] timestamp unix timestamp.
 * \param[out] percent % of phase.
 *
 */
int moonPhase(const time_t timestamp, double &percent);

/** \brief Returns the moon phase as text for the given date (given in unix timestamp) Answer range [0-7] (0 = new moon, 4 = full moon).
 * \param[in] timestamp unix timestamp.
 * \param[out] percent % of phase.
 *
 */
const QString moonPhaseText(const time_t timestamp, double &percent);

/** \brief Returns the tooltip for the moon phase for the given date.
 * \param[in] timestamp unix timestamp.
 *
 */
const QString moonTooltip(const time_t timestamp);

/** \brief Transforms and returns the string in title case.
 * \param[in] string string to transform.
 *
 */
const QString toTitleCase(const QString &string);

/** \brief Returns the wind direction string of the given degrees.
 * \param[in] degrees Wind direction in degrees.
 *
 */
const QString windDegreesToName(const double degrees);

/** \brief Returns a random alphanumeric string with the given length.
 * \param[in] length String length.
 *
 */
const QString randomString(const int length = 32);

/** \brief Parses a CSV string and returns the parts separated by commas in the text.
 * \param[in] csvText CSV text string.
 *
 */
const QStringList parseCSV(const QString &csvText);

#endif // UTILS_H_
