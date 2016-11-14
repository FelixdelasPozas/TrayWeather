/*
 File: LocationConfigDialog.h
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

#ifndef LOCATIONCONFIGDIALOG_H_
#define LOCATIONCONFIGDIALOG_H_

// Qt
#include "ui_LocationConfigDialog.h"
#include <QDialog>
#include <QNetworkAccessManager>

// C++
#include <memory>

class QNetworkReply;
class QDomNode;

class LocationConfigDialog
: public QDialog
, private Ui_LocationConfigDialog
{
    Q_OBJECT
  public:
    /** \brief LocationConfigDialog class constructor.
     * \param[in] parent pointer to the widget parent of this one.
     * \param[in] flags window flags.
     */
    LocationConfigDialog(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    /** \brief LocationConfigDialog class virtual destructor.
     *
     */
    virtual ~LocationConfigDialog()
    {}

  public slots:
    void replyFinished(QNetworkReply *reply);

  private:
    std::shared_ptr<QNetworkAccessManager> m_netManager;
};

#endif // LOCATIONCONFIGDIALOG_H_
