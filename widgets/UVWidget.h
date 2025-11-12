/*
 File: UVWidget.h
 Created on: 24/07/2021
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

#ifndef UVWIDGET_H_
#define UVWIDGET_H_

// Project
#include <Utils.h>

// Qt
#include <QWidget>
#include "ui_UVWidget.h"

class UVWidget
: public QWidget
, public Ui::UVWidget
{
    Q_OBJECT
  public:
    /** \brief UVWidget class constructor.
     * \param[in] data uv forecast data entry.
     *
     */
    explicit UVWidget(const UVData &data);

    /** \brief UVWidget class virtual destructor.
     *
     */
    virtual ~UVWidget()
    {};

  protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void changeEvent(QEvent *e) override;
};

#endif // UVWIDGET_H_
