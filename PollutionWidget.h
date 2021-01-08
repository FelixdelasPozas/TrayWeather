/*
 File: PollutionWidget.h
 Created on: 08/01/2021
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

#ifndef POLLUTIONWIDGET_H_
#define POLLUTIONWIDGET_H_

// Project
#include <Utils.h>

// Qt
#include <QWidget>
#include "ui_PollutionWidget.h"

/** \class PollutionWidget
 * \brief Widget that acts as a tooltip for the pollution chart dialog.
 *
 */
class PollutionWidget
: public QWidget
, public Ui::PollutionWidget
{
  public:
    /** \brief PollutionWidget class constructor.
     * \param[in] data pollution forecast data entry.
     *
     */
    explicit PollutionWidget(const PollutionData &data);

    /** \brief PollutionWidget class virtual destructor.
     *
     */
    virtual ~PollutionWidget()
    {};

  protected:
    virtual void paintEvent(QPaintEvent *event) override;
};

#endif // POLLUTIONWIDGET_H_
