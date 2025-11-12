/*
 File: AlertsWidget.h
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

#ifndef ALERTSWIDGET_H_
#define ALERTSWIDGET_H_

// Project
#include "ui_AlertsWidget.h"
#include <Utils.h>

// Qt
#include <QWidget>

/** \class AlertsWidget
 * \brief Implements a widget to show alerts.
 *
 */
class AlertsWidget
: public QWidget
, private Ui::AlertWidget
{
    Q_OBJECT
  public:
    /** \brief AlertsWidget class constructor.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \param[in] f Dialog flags.
     *
     */
    explicit AlertsWidget(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /** \brief AlertsWidget class virtual destructor.
     *
     */
    virtual ~AlertsWidget()
    {};

    /** \brief Sets the alert data.
     * \param[in] data Alerts list.
     *
     */
    void setAlertData(const Alerts &data);

  signals:
    void alertsSeen();

  protected:
    virtual void changeEvent(QEvent *e) override;
    virtual void showEvent(QShowEvent *) override;

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

#endif // ALERTSWIDGET_H_
