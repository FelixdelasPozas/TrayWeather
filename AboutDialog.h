/*
 File: AboutDialog.h
 Created on: 15/11/2016
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

#ifndef ABOUTDIALOG_H_
#define ABOUTDIALOG_H_

// Project
#include "ui_AboutDialog.h"

// Qt
#include <QDialog>

/** \class AboutDialog
 * \brief Egocentrical dialog with version and date of build.
 *
 */
class AboutDialog
: public QDialog
, public Ui_AboutDialog
{
    Q_OBJECT
  public:
    /** \brief AboutDialog class constructor.
     * \param[in] parent pointer to the widget parent of this one.
     * \param[in] flags window flags.
     *
     */
    AboutDialog(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);

    /** \brief AboutDialog class virtual destructor.
     *
     */
    virtual ~AboutDialog()
    {};

    static const QString VERSION; /** application version string. */

  protected:
    virtual void showEvent(QShowEvent *e) override;
};

#endif // ABOUTDIALOG_H_
