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
#include "ConfigurationDialog.h"

// Qt
#include <QSystemTrayIcon>

class QNetworkReply;
class QNetworkAccessManager;

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
    TrayWeather(const Configuration &configuration, QObject *parent = nullptr);

    /** \brief TrayWeather class virtual destructor.
     *
     */
    virtual ~TrayWeather()
    {};

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

  private:
    /** \brief Creates the tray icon menu.
     *
     */
    void createMenuEntries();

    /** \brief Makes a network request for weather forecast data.
     *
     */
    void requestForecastData() const;

    const Configuration                   &m_configuration; /** application configuration. */
    std::shared_ptr<QNetworkAccessManager> m_netManager;    /** network manager.           */
};


#endif // TRAYWEATHER_H_
