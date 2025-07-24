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
#include <functional>
#include <list>

// Qt
#include <QString>
#include <QDebug>
#include <QJsonObject>
#include <QColor>
#include <QDateTime>
#include <QTranslator>
#include <QStyledItemDelegate>
#include <QAbstractItemModel>
#include <QStyleOption>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QNetworkAccessManager>

class QPainter;
class QPixmap;
class QDialog;
class QNetworkRequest;
class QIODevice;
class QCheckBox;

const QString INI_FILENAME = QString("TrayWeather.ini");

enum class Units:              char { METRIC = 0, IMPERIAL, CUSTOM };
enum class PressureUnits:      char { HPA =  0, PSI, MMGH, INHG };
enum class TemperatureUnits:   char { CELSIUS = 0, FAHRENHEIT };
enum class PrecipitationUnits: char { MM = 0, INCH };
enum class WindUnits:          char { METSEC = 0, MILHR, KMHR, FEETSEC, KNOTS };
enum class Update:             char { NEVER = 0, DAILY, WEEKLY, MONTHLY };
enum class Representation:     char { NONE = 0, SPLINE, BARS };

static const QStringList MAP_LAYERS = { "temperature", "rain", "clouds", "wind" };
static const QStringList MAP_STREET = { "mapnik", "googlemap", "googlesat" };

static const QStringList OWM_LANGUAGES = { "af", "al", "ar", "az", "bg", "ca", "cz", "da", "de", "el", "en",
                                           "eu", "fa", "fi", "fr", "gl", "he", "hi", "hr", "hu", "id", "it",
                                           "ja", "kr", "la", "lt", "mk", "no", "nl", "pl", "pt", "pt_br", "ro",
                                           "ru", "sv", "se", "sk", "sl", "sp", "es", "sr", "th", "tr", "ua",
                                           "uk", "vi", "zh_cn", "zh_tw", "zu" };

static const QStringList QT_LANGUAGES = { "bg", "ca", "cs", "da", "de", "en", "es", "fi", "fr", "gd", "he", "hu",
                                          "it", "ja", "ko", "lv", "pl", "pt_br", "ru", "sk", "sl", "tr", "uk", "zh_cn" };

static QTranslator s_appTranslator; /** application language translator.   */
static QTranslator s_qtTranslator;  /** application Qt dialogs translator. */

static const std::list<double> RAIN_MAP_LAYER_GRADES_MM      = { 0.1, 2, 6, 8, 10, 14, 16, 20, 26, 32, 42, 48, 52, 70 };
static const std::list<double> TEMP_MAP_LAYER_GRADES_CELSIUS = { -24, -20, -16, -8, -4, 0, 4, 8, 16, 20, 24, 32, 36 };
static const std::list<double> WIND_MAP_LAYER_GRADES_METSEC  = { 0, 1.5, 3, 5, 8.5, 12, 15.5, 19, 22.5, 25.5, 29 };

enum class TooltipText: char { LOCATION = 0, WEATHER, TEMPERATURE, CLOUDINESS, HUMIDITY,
                               PRESSURE, WIND_SPEED, SUNRISE, SUNSET, AIR_QUALITY,
                               AIR_CO, AIR_O3, AIR_NO, AIR_NO2, AIR_SO2, AIR_NH3, AIR_PM25,
                               AIR_PM10, WIND_DIR, UPDATE_TIME, UV, MAX };

static const QStringList TooltipTextFields = { QObject::tr("Location"), QObject::tr("Current Weather"), QObject::tr("Temperature"),
                                               QObject::tr("Cloudiness"), QObject::tr("Humidity"), QObject::tr("Ground Pressure"),
                                               QObject::tr("Wind Speed"), QObject::tr("Sunrise"), QObject::tr("Sunset"),
                                               QObject::tr("Air Quality"), QObject::tr("Air Quality (CO)"),
                                               QObject::tr("Air Quality (O<sub>3</sub>)"),
                                               QObject::tr("Air Quality (NO)"),
                                               QObject::tr("Air Quality (NO<sub>2</sub>)"),
                                               QObject::tr("Air Quality (SO<sub>2</sub>)"),
                                               QObject::tr("Air Quality (NH<sub>3</sub>)"),
                                               QObject::tr("Air Quality (PM<sub>2.5</sub>)"),
                                               QObject::tr("Air Quality (PM<sub>10</sub>)"), 
                                               QObject::tr("Wind Direction"),
                                               QObject::tr("Time of last update"), 
                                               QObject::tr("UV radiation index") };

static const QString POLLUTION_UNITS{"µg/m<sup>3</sup>"};

constexpr int ICON_TEXT_BORDER = 26;

extern QString REQUESTS_BUFFER; /** buffer to log network requests. */

/** \struct LanguageData
 * \brief Contains a translation data.
 *
 */
struct LanguageData
{
    QString name;
    QString icon;
    QString file;
    QString author;

    /** \brief LanguageData constructor.
     * \param[in] n Language name.
     * \param[in] i Language flag icon path in resources.
     * \param[in] f Filename of the translation file without extension.
     *
     */
    LanguageData(const QString n, const QString i, const QString f, const QString a): name(n), icon(i), file(f), author(a) {};

    bool operator==(const LanguageData &other)
    { return name == other.name; }

    bool operator<(const LanguageData &other)
    { return name < other.name; }
};

/** Translations
 *
 */
static QList<LanguageData> TRANSLATIONS = {
    { "English",                ":/TrayWeather/languages/en.svg", "en_EN", "Félix de las Pozas Álvarez"    },
    { "Español (España)",       ":/TrayWeather/languages/es.svg", "es_ES", "Félix de las Pozas Álvarez"    },
    { "Русский",                ":/TrayWeather/languages/ru.svg", "ru_RU", "Andrei Stepanov"               },
    { "Deutsch",                ":/TrayWeather/languages/de.svg", "de_DE", "Andreas Lüdeke"                },
    { "Français",               ":/TrayWeather/languages/fr.svg", "fr_FR", "Stephane D."                   },
    { "简体中文",                ":/TrayWeather/languages/cn.svg", "zh_CN", "Chow Yuk Hong"                 },
    { "Português (Brasileiro)", ":/TrayWeather/languages/br.svg", "pt_BR", "Autergame"                     },
    { "Українська",             ":/TrayWeather/languages/uk.svg", "uk_UA", "Aleksandr Popov"               },
    { "Slovenščina",            ":/TrayWeather/languages/sl.svg", "sl_SI", "datenshi888"                   },
    { "한국어",                  ":/TrayWeather/languages/kr.svg", "ko_KR", "isl6"                          },
    { "Polski",                 ":/TrayWeather/languages/pl.svg", "pl_PL", "Krzysztof Blachnicki & Fresta" },
    { "Türkçe",                 ":/TrayWeather/languages/tr.svg", "tr_TR", "cmhrky"                        },
    { "Italiano",               ":/TrayWeather/languages/it.svg", "it_IT", "Hanamichi27"                   },
	  { "Magyar",                 ":/TrayWeather/languages/hu.svg", "hu_HU", "Bukodi Gábor"                  }
};

/** \struct IconThemeData
 * \brief Contains icon theme information.
 *
 */
struct IconThemeData
{
    QString name;
    QString id;
    bool    colored;
    QString author;
};

/** Icon themes.
 *
 */
static const QList<IconThemeData> ICON_THEMES = { { "FlatIcon Colored", "flaticon",      true,  "https://www.flaticon.com/" },
                                                  { "FlatIcon Mono",    "flaticon_mono", false, "https://www.flaticon.com/"},
                                                  { "Tempestacons",     "tempestacons",  false, "https://github.com/zagortenay333/Tempestacons" },
                                                  { "Meteocons",        "meteocons",     true,  "https://github.com/basmilius/weather-icons" },
                                                  { "Climacons",        "climacons",     false, "http://adamwhitcroft.com/"}
};

/** \struct Location
 * \brief Just a struct to return the selected location information.
 *
 */
struct Location
{
    QString location;   /** location name.            */
    QString translated; /** location name translated. */
    float latitude;     /** latitude coordinate       */
    float longitude;    /** longitude coordinate      */
    QString country;    /** country                   */
    QString region;     /** region/state              */

    /** \brief Location constructor.
     * \param[in] loc  Location name.
     * \param[in] lat  Latitude coordinate.
     * \param[in] lon  Longitude coordinate.
     * \param[in] coun  Country.
     * \param[in] reg  Region/State.
     */
    explicit Location(const QString &loc, const QString &trans, const float lat, const float lon, const QString &coun, const QString &reg)
    : location{loc}, translated{trans}, latitude{lat}, longitude{lon}, country{coun}, region{reg}
    {
    }

    /** \brief Location empty constructor.
     *
     */
    Location()
    : location{""}, translated{""}, latitude{0.f}, longitude{0.f}, country{""}, region{""} {};

    /** \brief Returns true if the struct contains valid data and false otherwise. 
     *
     */
    bool isValid() const
    {
      return !location.isEmpty();
    }
};

using Locations = QList<Location>;

/** \struct Alert
 * \brief Contains weather alert information.
 *
 */
struct Alert
{
  QString sender;
  QString event;
  unsigned long long startTime;
  unsigned long long endTime;
  QString description;
  bool seen;

  /** \brief Alert struct constructor.
   */
  Alert(): sender{}, event{}, startTime{0}, endTime{0}, description{}, seen{false} {};

  /** \brief Equal comparison operator. Returns true if equals.
   * \param other Other alert object. 
   *
   */
  bool operator==(const Alert &other)
  {
    return sender.compare(other.sender, Qt::CaseInsensitive) == 0 && event.compare(other.event, Qt::CaseInsensitive) == 0 && 
          startTime == other.startTime && endTime == other.endTime && description.compare(other.description, Qt::CaseInsensitive) == 0;
  }
};

using Alerts = QList<Alert>;


/** \struct Configuration
 * \brief Contains the application configuration.
 *
 */
struct Configuration
{
    double             latitude;        /** location latitude in degrees.                                  */
    double             longitude;       /** location longitude in degrees.                                 */
    QString            country;         /** location's country.                                            */
    QString            region;          /** location's region.                                             */
    QString            city;            /** location's city.                                               */
    QString            ip;              /** internet address.                                              */
    QString            providerId;      /** weather provider name                                          */
    Units              units;           /** measurement units.                                             */
    unsigned int       updateTime;      /** time between updates.                                          */
    bool               mapsEnabled;     /** true if maps tab is visible, false otherwise.                  */
    bool               useDNS;          /** true to use DNS address for geo location instead of own IP.    */
    bool               useGeolocation;  /** true to use the ip-api.com services, false to use manual.      */
    bool               roamingEnabled;  /** true if georaphical coordinates are asked on each forecast.    */
    bool               lightTheme;      /** true if light theme is being used, false if dark theme.        */
    unsigned int       iconType;        /** 0 if just icon, 1 if just temperature, 2 and 3 if both.        */
    unsigned int       iconTheme;       /** icon theme. See ICON_THEMES var.                               */
    QColor             iconThemeColor;  /** icon theme color for monocolor themes.                         */
    QColor             trayTextColor;   /** Color of tray temperature text.                                */
    bool               trayTextMode;    /** true for fixed color, false for dynamic color.                 */
    bool               trayTextBorder;  /** true to draw a border around icon text, false otherwise.       */
    int                trayBorderWidth; /** width of the tray text border.                                 */
    bool               trayTextDegree;  /** true to display the degree symbol in the tray text.            */
    QString            trayTextFont;    /** font used for temperature icon.                                */
    int                trayFontSpacing; /** spacing between font characters (-25 to 25).                   */
    bool               trayBorderAuto;  /** true to use a automatic color for the temperature icon border. */
    QColor             trayBorderColor; /** color to use in the temperature icon border.                   */
    bool               trayBackAuto;    /** true to use transparent color for the icons background.        */
    QColor             trayBackColor;   /** color to use as background color in the icons.                 */
    bool               stretchTempIcon; /** true to strech the temp icon vertically, false otherwise.      */
    QColor             minimumColor;    /** minimum value dynamic color.                                   */
    QColor             maximumColor;    /** maximum value dynamic color.                                   */
    int                minimumValue;    /** dynamic color minimum value.                                   */
    int                maximumValue;    /** dynamic color maximum value.                                   */
    Update             update;          /** frequency of check for update.                                 */
    QDateTime          lastCheck;       /** time of last update check.                                     */
    bool               autostart;       /** true to autostart at login, false otherwise.                   */
    int                lastTab;         /** last tab visualized.                                           */
    QString            lastLayer;       /** last maps layer used: temperature, rain, clouds, wind.         */
    QString            lastStreetLayer; /** last street layer used: mapnik, googlemap, googlesat.          */
    QString            language;        /** application language.                                          */
    QList<TooltipText> tooltipFields;   /** tooltip fields in order.                                       */
    bool               showAlerts;      /** true to show weather alerts and false otherwise.               */
    bool               keepAlertIcon;   /** true to show the alert icon in tray until alerts expire.       */
    TemperatureUnits   tempUnits;       /** custom temperature units.                                      */
    PressureUnits      pressureUnits;   /** custom pressure units.                                         */
    PrecipitationUnits precUnits;       /** custom precipitation units.                                    */
    WindUnits          windUnits;       /** custom wind units.                                             */
    bool               swapTrayIcons;   /** true to swap tray icons and false otherwise.                   */
    int                trayIconSize;    /** size of tray icon in [50-100] %.                               */
    Representation     tempRepr;        /** temperature representation in the forecast graph.              */
    Representation     rainRepr;        /** rain representation in the forecast graph.                     */
    Representation     snowRepr;        /** snow representation in the forecast graph.                     */
    QColor             tempReprColor;   /** color of temperature representation in the forecast graph.     */
    QColor             rainReprColor;   /** color of rain representation in the forecast graph.            */
    QColor             snowReprColor;   /** color of snow representation in the forecast graph.            */
    float              cloudMapOpacity; /** opacity of the cloud layer in the map. Value in [0,1].         */
    float              windMapOpacity;  /** opacity of the wind layer in the map. Value in [0,1].          */
    float              rainMapOpacity;  /** opacity of the rain layer in the map. Value in [0,1].          */
    float              tempMapOpacity;  /** opacity of the temp layer in the map. Value in [0,1].          */
    double             barWidth;        /** Bar representation width in [0.5-2.0].                         */

    /** \brief Configuration struct constructor.
     *
     */
    Configuration()
    : latitude        {-91}
    , longitude       {-181}
    , country         {"Unknown"}
    , region          {"Unknown"}
    , city            {"Unknown"}
    , ip              {"Unknown"}
    , providerId      {""}
    , units           {Units::METRIC}
    , updateTime      {15}
    , mapsEnabled     {false}
    , useDNS          {false}
    , useGeolocation  {true}
    , roamingEnabled  {false}
    , lightTheme      {true}
    , iconType        {0}
    , iconTheme       {0}
    , iconThemeColor  {Qt::black}
    , trayTextColor   {Qt::white}
    , trayTextMode    {true}
    , trayTextBorder  {true}
    , trayBorderWidth {32}
    , trayTextDegree  {true}
    , trayTextFont    {""}
    , trayFontSpacing {0}
    , trayBorderAuto  {true}
    , trayBorderColor {Qt::black}
    , trayBackAuto    {true}
    , trayBackColor   {Qt::white}
    , stretchTempIcon {false}
    , minimumColor    {Qt::blue}
    , maximumColor    {Qt::red}
    , minimumValue    {-10}
    , maximumValue    {45}
    , update          {Update::WEEKLY}
    , lastCheck       (QDateTime::currentDateTime())
    , autostart       {false}
    , lastTab         {0}
    , lastLayer       {"temperature"}
    , lastStreetLayer {"mapnik"}
    , language        {"en_EN"}
    , tooltipFields   {TooltipText::LOCATION, TooltipText::WEATHER, TooltipText::TEMPERATURE}
    , showAlerts      {true}
    , keepAlertIcon   {false}
    , tempUnits       {TemperatureUnits::CELSIUS}
    , pressureUnits   {PressureUnits::HPA}
    , precUnits       {PrecipitationUnits::MM}
    , windUnits       {WindUnits::METSEC}
    , swapTrayIcons   {false}
    , trayIconSize    {100}
    , tempRepr        {Representation::SPLINE}
    , rainRepr        {Representation::BARS}
    , snowRepr        {Representation::NONE}
    , tempReprColor   {Qt::blue}
    , rainReprColor   {Qt::green}
    , snowReprColor   {Qt::red}
    , cloudMapOpacity {0.75}
    , windMapOpacity  {0.75}
    , rainMapOpacity  {0.75}
    , tempMapOpacity  {0.75}
    , barWidth        {1.0}
    {};

    /** \brief Returns true if the struct contains valid data and false otherwise. 
     *
     */
    bool isValid() const
    {
      return (latitude <= 90.0) &&   (latitude >= -90.0) &&
             (longitude <= 180.0) && (longitude >= -180.0) &&
             !providerId.isEmpty();
    }
};

/** \brief Loads the application configuration from the system registry.
 * \param[out] configuration Application Configuration struct.
 *
 */
void load(Configuration &configuration);

/** \brief Saves the application configuration to the system registry.
 * \param[in] configuration Application Configuration struct.
 *
 */
void save(const Configuration &configuration);

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

    /** \brief ForecastData default constructor. 
     */
    ForecastData(): dt{0}, temp{0}, temp_max{0}, temp_min{0}, cloudiness{0}, humidity{0}, pressure{0},
                    weather_id{0}, wind_speed{0}, wind_dir{0}, rain{0}, snow{0}, sunrise{0}, sunset{0},
                    name{"Unknown"}, country{"Unknown"} {};

    /** \brief Returns true if the struct contains valid data and false otherwise. 
     *
     */
    bool isValid() const { return dt != 0 && !icon_id.isEmpty(); };
};

using Forecast = QList<ForecastData>;

/** \struct PollutionData
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

    /** \brief PollutionData struct class constructor.
     *
     */
    PollutionData(): dt{0}, aqi{1}, co{0}, no{0}, no2{0}, o3{0}, so2{0}, pm2_5{0}, pm10{0}, nh3{0} {};

    /** \brief Returns true if the struct contains valid data and false otherwise. 
     *
     */
    bool isValid() const { return dt != 0; }
};

using Pollution = QList<PollutionData>;

/** \struct UVData
 * \brief Contains the UV index forecast data for a given time.
 *
 */
struct UVData
{
    long long int dt;  /** date and time of the data. */
    double        idx; /** uv index for that date.    */

    /** \brief UVData struct default constructor.
     *
     */
    UVData(): dt{0}, idx{0}{};

    /** \brief UVData struct constructor.
     * \param[in] dtime unix timestamp.
     * \param[in] index UV index.
     *
     */
    UVData(long long int dtime, double index)
    : dt{dtime}, idx{index}{};

    /** \brief Returns true if the struct contains valid data and false otherwise. 
     *
     */
    bool isValid() const { return dt != 0; }
};

using UV = QList<UVData>;

const QString CONCENTRATION_UNITS{"µg/m<sup>3</sup>"};

const QStringList CONCENTRATION_NAMES{"CO", "NO", "NO<sub>2</sub>", "O<sub>3</sub>", "SO<sub>2</sub>", "PM<sub>2.5</sub>", "PM<sub>10</sub>", "NH<sub>3</sub>"};

const QList<QColor> CONCENTRATION_COLORS{ QColor::fromHsv(0, 255, 255),   QColor::fromHsv(45, 255, 255),  QColor::fromHsv(90, 255, 255),
                                          QColor::fromHsv(135, 255, 255), QColor::fromHsv(180, 255, 255), QColor::fromHsv(225, 255, 255),
                                          QColor::fromHsv(270, 255, 255), QColor::fromHsv(315, 255, 255)};


/** \brief Returns the icon corresponding to the given data.
 * \param[in] data forecast data struct.
 * \param[in] theme Icon theme id.
 *
 */
const QPixmap weatherPixmap(const ForecastData &data, const unsigned int theme, const QColor &color = Qt::black);

/** \brief Returns the icon corresponding to the given data.
 * \param[in] iconId OpenWeatherMap icon id.
 * \param[in] theme Icon theme id.
 * \param[in] color Icon color for mono icon themes.
 *
 */
const QPixmap weatherPixmap(const QString &iconId, const unsigned int theme, const QColor &color = Qt::black);

/** \brief Returns the moon phase icon corresponding to the given data.
 * \param[in] data forecast data struct.
 * \param[in] theme Icon theme id.
 *
 */
const QPixmap moonPixmap(const ForecastData& data, const unsigned int theme, const QColor &color = Qt::black);

/** \brief Changes the non alpha pixels of the given image with the given color.
 * \param[in] img Source image reference.
 * \param[in] color Color to paint.
 *
 */
void paintPixmap(QImage &img, const QColor &color = Qt::black);

/** \brief Prints the contents of the data to the QDebug stream.
 * \param[in] debug QDebug stream.
 * \param[in] data ForecastData struct.
 *
 */
QDebug operator<< (QDebug d, const ForecastData &data);

/** \brief Prints the contents of the data to the QDebug stream.
 * \param[in] debug QDebug stream.
 * \param[in] data PollutionData struct.
 *
 */
QDebug operator<< (QDebug d, const PollutionData &data);

/** \brief Prints the contents of the data to the QDebug stream.
 * \param[in] debug QDebug stream.
 * \param[in] data UVData struct.
 *
 */
QDebug operator<< (QDebug d, const UVData &data);


/** \brief Converts the given mm value to inches.
 * \param[in] value mm value.
 *
 */
const double convertMmToInches(const double value);

/* \brief Converts the given celsius to fahrenheit.
 * \param[in] value Celsius degrees value.
 *
 */
const double convertCelsiusToFahrenheit(const double value);

/* \brief Converts the given fahrenheit to celsius.
 * \param[in] value Fahrenheit degrees value.
 *
 */
const double convertFahrenheitToCelsius(const double value);

/** \brief Converts the given meters/second to miles/hour.
 * \param[in] value Meters/sec value.
 *
 */
const double convertMetersSecondToMilesHour(const double value);

/** \brief Converts the given meters/second to knots.
 * \param[in] value Meters/sec value.
 *
 */
const double convertMetersSecondToKnots(const double value);

/** \brief Converts the given meters/second to kilometers/hour.
 * \param[in] value Meters/sec value.
 *
 */
const double convertMetersSecondToKilometersHour(const double value);

/** \brief Converts the given meters/second to feet/second.
 * \param[in] value Meters/sec value.
 *
 */
const double convertMetersSecondToFeetSecond(const double value);

/** \brief Converts hectoPascals to pounds per square inch.
 * \param[in] value hPa value.
 *
 */
const double converthPaToPSI(const double value);

/** \brief Converts hectoPascals to millimeters of mercury.
 * \param[in] value hPa value.
 *
 */
const double converthPaTommHg(const double value);

/** \brief Converts hectopascals to inches of mercury.
 * \param[in] value hPa value.
 *
 */
const double converthPaToinHg(const double value);

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

/** \brief Returs the dpi scale against an original scale of 96dpi.
 *
 */
double dpiScale();

/** \brief Scales the dialog according to dpi.
 * \param[in] window QDialog instance pointer.
 *
 */
void scaleDialog(QDialog *window);

/** \brief Returns the color corresponding to the given UV index.
 * \param[in] value UV index value.
 *
 */
QColor uvColor(const double value);

/** \brief Returns the value of the ROAMING_ENABLED registry key
 *  using the microsoft API.
 *
 */
bool getRoamingRegistryValue();

/** \brief Changes the application language to the given lang abbreviation.
 * \param[in] lang Language tranlation file.
 *
 */
void changeLanguage(const QString &lang);

/** \brief Returns the QString of concatenated grades applying the transformation function f.
 * \param[in] grades List of grades to transform.
 * \param[in] f Transformation function.
 *
 */
const QString generateMapGrades(const std::list<double> &grades, std::function<double(double)> f);

/** \brief Returns the temperature icon string for the given configuration.
 * \param[in] c Configuration struct reference.
 *
 */
QString temperatureIconString(const Configuration &c);

/** \brief Returns the temperature degrees text for the given configuration.
 * \param[in] c Configuration struct reference.
 *
 */
QString temperatureIconText(const Configuration &c);

/** \brief Creates and writes an image with all the theme icons.
 * \param[in] theme Icon theme index.
 * \param[in] size Icon size in pixels.
 * \param[in] color Icon theme color for monocolor themes.
 * \param[in] backColor background color for the icons.
 *
 */
QPixmap createIconsSummary(const unsigned int theme, const int size, const QColor &color, const QColor &backColor);

/** \brief Sets the background color of the given pixmap to the given color. The pixmap must have transparent background.
 * \param[in] color New background color.
 * \param[in] pix Pixmap to change color.
 *
 */
QPixmap setIconBackground(const QColor &color, const QPixmap &pix);

/** \brief Converts all pixels non transparent from other to transparent in base. Returns false if it fails.
 * \param[inout] base Base image to modify pixels. 
 * \param[in] other Other image to subtract.
 *
 */
bool subtractImage(QImage &base, const QImage &other);

/** \brief Converts all pixels non transparent from other to transparent in base. Returns false if it fails.
 * \param[inout] base Base pixmap to modify pixels. 
 * \param[in] other Other pixmap to subtract.
 *
 */
bool subtractPixmap(QPixmap &base, const QPixmap &other);

/** \brief Computes the Qt::Rect of drawn pixels in the given image.
 * \param[in] image QImage object reference.
 *
 */
QRect computeDrawnRect(const QImage &image);

/** \brief Computes and returns the sunrise and sunset time in unix time for the given ForecastData date and coordinates.
 * \param[in] data ForecastData struct.
 * \param[in] longitude
 * \param[in] latitude
 *
 */
std::pair<long long, long long> computeSunriseSunset (const ForecastData &data, const double longitude, const double latitude);

/** \brief Blurs and returns a copy of the given pixmap. 
 * \param[in] pixmap QPixmap object reference. 
 * \param[in] blurValue Value of the gaussian blur.
 * 
*/
QPixmap blurPixmap(const QPixmap &pixmap, const int blurValue);

/** \brief Returns the correct QSettings object for the application (ini file or registry)
 *
 */
QSettings applicationSettings();

/** \brief Returns true if the application is in portable mode, and false otherwise. 
 *
 */
bool isPortable();

/** \brief Helper method to convert the wmo code inside to fill a ForecastData object fields. 
 *         WMO codes are used to represent weather conditions (meteorological codes for use at observing stations).
 * \param[in] forecast ForecastData object reference, contains wmo code.
 *
 */
void fillWMOCodeInForecast(ForecastData &forecast);

/** \brief Changes the weather units of the forecast data according to the given configuration.
 * \param[in] config Configuration struct reference.
 * \param[in] forecast ForecastData object reference.
 *
 */
void changeWeatherUnits(const Configuration &config, ForecastData &forecast);

/** \class CustomComboBox
 * \brief ComboBox that uses rich text for selected item.
 *
 */
class CustomComboBox
: public QComboBox
{
  public:
    /** \brief CustomComboBox class constructor.
     * \param[in] parent Raw pointer of the QObject parent of this one.
     *
     */
    explicit CustomComboBox(QWidget *parent = nullptr)
    : QComboBox(parent)
    {};

    /** \brief CustomComboBox class virtual destructor.
     *
     */
    virtual ~CustomComboBox()
    {};

    virtual void paintEvent(QPaintEvent* e) override;
};


/** \class RichTextDelegate
 * \brief Item delegate for QListView and QComboBox.
 *
 */
class RichTextItemDelegate
: public QStyledItemDelegate
{
  public:
    /** \brief RichTextItemDelegate class constructor.
     * \param[in] parent Raw pointer of the QObject parent of this one.
     *
     */
    explicit RichTextItemDelegate(QObject *parent = nullptr)
    : QStyledItemDelegate(parent)
    {};

    /** \brief RichTextItemDelegate class virtual destructor.
     *
     */
    virtual ~RichTextItemDelegate()
    {};

    virtual void paint(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;
};

/** \class ClickableLabel
 * \brief QLabel subclass that emits a signal when clicked.
 *
 */
class ClickableLabel
: public QLabel
{
    Q_OBJECT
  public:
    /** \brief ClickableLabel class constructor.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \f Widget flags.
     *
     */
    explicit ClickableLabel(QWidget *parent=0, Qt::WindowFlags f=0)
    : QLabel(parent, f)
    {};

    /** \brief ClickableLabel class constructor.
     * \param[in] text Label text.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \f Widget flags.
     *
     */
    explicit ClickableLabel(const QString &text, QWidget *parent=0, Qt::WindowFlags f=0)
    : QLabel(text, parent, f)
    {};

    /** \brief ClickableLabel class virtual destructor.
     *
     */
    virtual ~ClickableLabel()
    {};

    /** \brief  Helper method to connect to a QCheckbox object. 
     * \param cb QCheckBox raw pointer. 
     *
     */
    void connectToCheckBox(QCheckBox *cb);

  signals:
    void clicked();

  protected:
    void mousePressEvent(QMouseEvent* e)
    {
      emit clicked();
      QLabel::mousePressEvent(e);
    }
};

/** \class ClickableHoverLabel
 * \brief ClickableLabel subclass that changes the mouse cursor when hovered.
 *
 */
class ClickableHoverLabel
: public ClickableLabel
{
    Q_OBJECT
  public:
    /** \brief ClickableHoverLabel class constructor.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \f Widget flags.
     *
     */
    explicit ClickableHoverLabel(QWidget *parent=0, Qt::WindowFlags f=0)
    : ClickableLabel(parent, f)
    {};

    /** \brief ClickableHoverLabel class constructor.
     * \param[in] text Label text.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \f Widget flags.
     *
     */
    explicit ClickableHoverLabel(const QString &text, QWidget *parent=0, Qt::WindowFlags f=0)
    : ClickableLabel(text, parent, f)
    {};
    
    /** \brief ClickableHoverLabel class virtual destructor.
     *
     */
    virtual ~ClickableHoverLabel()
    {};

  protected:
    virtual void enterEvent(QEvent *event) override
    {
      setCursor(Qt::PointingHandCursor);
      ClickableLabel::enterEvent(event);
    }

    virtual void leaveEvent(QEvent *event) override
    {
      setCursor(Qt::ArrowCursor);
      ClickableLabel::leaveEvent(event);
    }
};

class NetworkAccessManager
: public QNetworkAccessManager
{
    Q_OBJECT
  public:
    static bool LOG_REQUESTS; /** true to log network requests, false otherwise. */

    /** \brief NetworkAccessManager class constructor.
     * \param[in] parent Raw pointer of the QObject parent of this one.
     *
     */
    explicit NetworkAccessManager(QObject *parent = nullptr)
    : QNetworkAccessManager(parent)
    {};

    /** \brief NetworkAccessManager class virtual destructor.
     *
     */
    virtual ~NetworkAccessManager()
    {};

    QNetworkReply* createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData = 0) override;
};

#endif // UTILS_H_
