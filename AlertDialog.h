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

// Project
#include "ui_AlertDialog.h"
#include <Utils.h>

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
    explicit AlertDialog(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /** \brief AlertDialog class virtual destructor.
     *
     */
    virtual ~AlertDialog()
    {};

    /** \brief Sets the alert data.
     * \param[in] data Alerts list.
     *
     */
    void setAlertData(const Alerts &data);

    /** \brief Returns the value of the checkbox to show again the alert in the future.
     *
     */
    bool showAgain() const;

  protected:
    virtual void showEvent(QShowEvent *e) override;
    virtual void changeEvent(QEvent *e) override;

  private slots:
    /** \brief Updates the dialog with the next alert, if it exists. 
     *
     */
    void onNextButtonClicked();

    /** \brief Updates the dialog with the previous alert.
     *
     */
    void onPreviousButtonClicked();

  private:
    /** \brief Helper method to connect the signals with the slots.
     *
     */
    void connectSignals();

    /** \brief Updates the dialog with the information on the alert of given index.
     * \param index Alert index.
     *
     */
    void showAlert(const int index);

    Alerts m_alerts; /** Alerts list. */
};

#endif // ALERTDIALOG_H_
