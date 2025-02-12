/*
 File: TrayWeather.h
 Created on: 13/11/2016
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

#ifndef TRAYWEATHER_H_
#define TRAYWEATHER_H_

// Project
#include <ConfigurationDialog.h>
#include <Utils.h>
#include <WeatherDialog.h>
#include <Provider.h>

// Qt
#include <QSystemTrayIcon>
#include <QTimer>
#include <QTranslator>
#include <QAbstractNativeEventFilter>

class QNetworkReply;
class QNetworkAccessManager;
class AboutDialog;
class TrayWeather;
class AlertDialog;

/** \class NativeEventFilter
 * \brief Filters the events from windows to catch the one for 'wake up' signal.
 *
 */
class NativeEventFilter
: public QAbstractNativeEventFilter
{
  public:
    /** \brief NativeEventFilter class constructor.
     * \param[in] tw TrayWeather application pointer.
     *
     */
    explicit NativeEventFilter(TrayWeather *tw)
    : QAbstractNativeEventFilter()
    , m_tw{tw}
    {};

    /** \brief NativeEventFilter class virtual destructor.
     *
     */
    virtual ~NativeEventFilter()
    {};

    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);
  private:
    TrayWeather *m_tw; /** tray weather application reference. */
};

/** \class TrayWeather
 * \brief Implements the tray icon and application logic.
 *
 */
class TrayWeather
: public QSystemTrayIcon
{
    Q_OBJECT
  public:
    /** \brief TrayWeather class constructor.
     * \param[in] configuration application configuration values, assumed to be valid.
     * \param[in] parent pointer to the object parent of this one.
     *
     */
    TrayWeather(Configuration &configuration, QObject *parent = nullptr);

    /** \brief TrayWeather class virtual destructor.
     *
     */
    virtual ~TrayWeather();

  private slots:
    /** \brief Handles network replies.
     * \param[in] reply network reply object pointer.
     *
     */
    void replyFinished(QNetworkReply *reply);

    /** \brief Exits the application.
     *
     */
    void exitApplication();

    /** \brief Shows the "About" dialog.
     *
     */
    void showAboutDialog();

    /** \brief Shows a tab depending on the caller.
     *
     */
    void showTab();

    /** \brief Shows the configuration dialog.
     *
     */
    void showConfiguration();

    /** \brief Handles icon activation.
     * \param[in] reason activation reason.
     *
     */
    void onActivation(QSystemTrayIcon::ActivationReason reason);

    /** \brief Makes a network request for weather forecast data or geolocation.
     *
     */
    void requestData();

    /** \brief Updates the tray icon context menu with the state of the maps.
     * \param[in] value true if maps enabled and false otherwise.
     *
     */
    void onMapsStateChanged(bool value);

    /** \brief Changes the application language to the one specified.
     * \param[in] lang Language id.
     *
     */
    void onLanguageChanged(const QString &lang);

    /** \brief Updates the alert data when the user closes an AlertDialog
     *
     */
    void onAlertDialogClosed();

    /** \brief Creates or raises the alert dialog.
     *
     */
    void showAlert();

    /** \brief Invalidates all weather data and asks for new data.
     *
     */
    void forceRequestData();

    /** \brief Gets weather and forecast data from the provider and updates the interface.
     *
     */
    void processWeatherData();

    /** \brief Gets the pollution forecast and updates the interface.
     *
     */
    void processPollutionData();

    /** \brief Gets the UV forecast and updates the interface.
     *
     */
    void processUVData();

    /** \brief Fills the alert list with the latest alert.
     * \param[in] alerts Weather alerts list. 
     *
     */
    void processAlerts(const Alerts &alerts);

    /** \brief Sets an error message in the tooltip text. 
     *
     */
    void setErrorTooltip(const QString &msg);

  private:
    /** \brief Updates the tray icon tooltip.
     *
     */
    void updateTooltip();

    /** \brief Returns the tray icon tooltip text according to current configuration.
     *
     */
    QString tooltipText() const;

    /** \brief Helper method to connect all the signals and slots.
     *
     */
    void connectSignals();

    /** \brief Helper method to connect all the signals of the provider to the slots. 
     *
     */
    void connectProviderSignals();

    /** \brief Helper method to disconnect all the signals and slots.
     *
     */
    void disconnectSignals();

    /** \brief Helper method to disconnect all the signals of the provider from the slots. 
     *
     */
    void disconnectProviderSignals();

    /** \brief Creates the tray icon menu.
     *
     */
    void createMenuEntries();

    /** \brief Returns true if the data is valid.
     *
     */
    bool validData() const;

    /** \brief Request geolocation information, can ask for DNS IP first if enabled on configuration.
     *
     */
    void requestGeolocation();

    /** \brief Request weather data from network.
     *
     */
    void requestForecastData();

    /** \brief Helper method that checks for application updates.
     *
     */
    void checkForUpdates();

    /** \brief Updates the context menu translations.
     *
     */
    void translateMenu();

    /** \brief Parses Gihub reply data.
     * \param[in] data Github reply data.
     *
     */
    void processGithubData(const QByteArray &data);

    /** \brief Parses geo-location data.
     * \param[in] data Geo-location data in CSV.
     *
     */
    void processGeolocationData(const QByteArray &data, const bool isDNS);

    /** \brief Updates, if needed, the network manager to use by the application and providers. 
    *
    */
    void updateNetworkManager();

    /** \brief Modifies the Ui based on the capabilities of the current weather provider.
     *
     */
    void updateUi();

    Configuration                         &m_configuration;   /** application configuration.                        */
    std::shared_ptr<QNetworkAccessManager> m_netManager;      /** network manager.                                  */
    Forecast                               m_data;            /** list of forecast data.                            */
    ForecastData                           m_current;         /** weather conditions now.                           */
    Pollution                              m_pData;           /** list pollution data.                              */
    UV                                     m_vData;           /** list of uv data.                                  */
    QTimer                                 m_timer;           /** timer for updates and retries.                    */
    WeatherDialog                         *m_weatherDialog;   /** dialog to show weather and forecast data.         */
    AboutDialog                           *m_aboutDialog;     /** pointer to current (if any) about dialog.         */
    ConfigurationDialog                   *m_configDialog;    /** pointer to current (if any) configuration dialog. */
    QString                                m_DNSIP;           /** DNS IP used for geolocation.                      */
    QTimer                                 m_updatesTimer;    /** timer to check for application updates.           */
    QSystemTrayIcon                       *m_additionalTray;  /** Additional tray icon for two icon mode.           */
    NativeEventFilter                      m_eventFilter;     /** Windows OS event filter.                          */
    AlertDialog                           *m_alertsDialog;    /** Alerts dialog.                                    */
    Alerts                                 m_alerts;          /** Last shown alert.                                 */
    std::shared_ptr<WeatherProvider>       m_provider;        /** Weather data provider                             */

    friend class NativeEventFilter;
};


#endif // TRAYWEATHER_H_
