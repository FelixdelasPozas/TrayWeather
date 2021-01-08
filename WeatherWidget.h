/*
 File: WeatherWidget.h
 Created on: 26/11/2016
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

#ifndef WEATHERWIDGET_H_
#define WEATHERWIDGET_H_

// Project
#include <Utils.h>

// Qt
#include <QWidget>
#include "ui_WeatherWidget.h"

/** \class TooltipWidget
 * \brief Widget that acts as a tooltip for the chart dialog.
 *
 */
class WeatherWidget
: public QWidget
, public Ui::WeatherWidget
{
  public:
    /** \brief TooltipWidget class constructor.
     * \param[in] data forecast data entry.
     * \param[in] config application configuragion.
     *
     */
    explicit WeatherWidget(const ForecastData &data, const Configuration &config);

    /** \brief TooltipWidget class virtual destructor.
     *
     */
    virtual ~WeatherWidget()
    {};

  protected:
    virtual void paintEvent(QPaintEvent *event) override;
};

#endif // WEATHERWIDGET_H_
