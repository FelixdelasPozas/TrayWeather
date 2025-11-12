/*
 File: CurrentWeatherWidget.cpp
 Created on: 09/11/2025
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

#ifndef _CURRENT_WEATHER_WIDGET_H_
#define _CURRENT_WEATHER_WIDGET_H_

// Project
#include <Utils.h>

// Qt
#include <QWidget>
#include "ui_CurrentWeatherWidget.h"

/** \class CurrentWeatherWidget
 * \brief Implements a widget to show current weather in the desktop.
 *
 */
class CurrentWeatherWidget: public QWidget, private Ui::CurrentWeatherWidget
{
  Q_OBJECT
    public:
      /** \brief CurrentWeatherWidget class constructor. 
       * \param[in] data Weather data to show.
       * \param[in] config Current application configuration.
       * \param[in] position Tray icon position.
       *
       */
      CurrentWeatherWidget(const ForecastData &data, const PollutionData &pData, const UVData &uData, const Configuration &config, const QPoint &trayPosition);

      /** \brief CurrentWeatherWidget class virtual destructor. 
       *
       */
      virtual ~CurrentWeatherWidget()
      {};

      /** \brief Returns the duration in seconds of the dialog.
       *
       */
      static int durationInMs() 
      {
          return 5000; // duration in seconds of the widget.
      }

  protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void changeEvent(QEvent *e) override;
    virtual void showEvent(QShowEvent *e) override;

  private slots:
    /** \brief Closes the dialog and deletes the widget.
     */
    void onFinished();

  private:
    /** \brief Helper method to compute the widget position based on the tray icon position.
     */
    QPoint computeWidgetPosition();

    QPoint m_trayPos; /** position of the tray icon on screen */

};

#endif