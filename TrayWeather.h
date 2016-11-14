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

// Qt
#include <QSystemTrayIcon>

/** \class TrayWeather
 * \brief Implements the tray icon and application logic.
 *
 */
class TrayWeather
: public QSystemTrayIcon
{
  public:
    /** \brief TrayWeather class constructor.
     * \param[in] parent pointer to the object parent of this one.
     *
     */
    TrayWeather(QObject *parent = nullptr);

    /** \brief TrayWeather class virtual destructor.
     *
     */
    virtual ~TrayWeather()
    {};

  private:

};

#endif // TRAYWEATHER_H_
