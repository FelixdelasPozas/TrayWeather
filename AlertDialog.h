/*
 File: AlertDialog.h
 Created on: 12/12/2021
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

#ifndef ALERTDIALOG_H_
#define ALERTDIALOG_H_

#include "ui_AlertDialog.h"

// Qt
#include <QDialog>

/** \class AlertDialog
 * \brief Implements the dialog to show alerts.
 *
 */
class AlertDialog
: public QDialog
, private Ui::AlertDialog
{
    Q_OBJECT
  public:
    /** \brief AlertDialog class constructor.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \param[in] f Dialog flags.
     *
     */
    explicit AlertDialog(QWidget *parent = nullptr, Qt::WindowFlags f = 0);

    /** \brief AlertDialog class virtual destructor.
     *
     */
    virtual ~AlertDialog()
    {};

    /** \brief Sets the alert data.
     * \param[in] obj Alert Json object.
     *
     */
    void setAlertData(const QJsonObject &data);

    /** \brief Returns the value of the checkbox to show again the alert in the future.
     *
     */
    bool showAgain() const;

  protected:
    virtual void showEvent(QShowEvent *e) override;
    virtual void changeEvent(QEvent *e) override;
};

#endif // ALERTDIALOG_H_
