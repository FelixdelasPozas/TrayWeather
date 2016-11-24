/*
 File: WeatherDialog.h
 Created on: 24/11/2016
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

#ifndef WEATHERDIALOG_H_
#define WEATHERDIALOG_H_

// Project
#include <Utils.h>

// Qt
#include <QDialog>
#include "ui_WeatherDialog.h"

namespace QtCharts
{
  class QChartView;
}

/** \class WeatherDialog
 * \brief Implements the dialog showing the current weather and the forecast.
 *
 */
class WeatherDialog
: public QDialog
, public Ui_WeatherDialog
{
    Q_OBJECT
  public:
    /** \brief WeatherDialog class constructor.
     * \param[in] parent pointer of the widget parent of this one.
     * \param[in] flags window flags.
     *
     */
    WeatherDialog(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    /** \brief WeatherDialog class virtual destructor.
     *
     */
    virtual ~WeatherDialog()
    {};

    /** \brief Sets the weather and forecast data.
     * \param[in] current current weather data.
     * \param[in] data forecast data.
     * \param[in] units temperature units.
     *
     */
    void setData(const ForecastData &current, const Forecast &data, const Temperature units);

  private slots:
    /** \brief Resets the chart's zoom to the original one.
     *
     */
    void onResetPressed();

    /** \brief Updates the GUI when the user changes the tab.
     * \param[in] index index of the current tab.
     *
     */
    void onTabChanged(int index);
  private:
    QtCharts::QChartView *m_chartView;
};

#endif // WEATHERDIALOG_H_
