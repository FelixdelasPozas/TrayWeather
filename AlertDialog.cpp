/*
 File: AlertDialog.cpp
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

// Project
#include <AlertDialog.h>
#include <Utils.h>

// Qt
#include <QJsonObject>

// C++
#include <cassert>

//--------------------------------------------------------------------
AlertDialog::AlertDialog(QWidget *p, Qt::WindowFlags f)
: QDialog(p, f)
{
  setupUi(this);
}

//--------------------------------------------------------------------
void AlertDialog::setAlertData(const Alerts &alerts)
{
  assert(!alerts.empty());

  const QString UNKNOWN = tr("Unknown");
  

  if(alerts.count() > 0)
  {
    const auto data = alerts.first();
    m_sender->setText(data.sender);
    m_event->setText(data.event);
    m_description->setText(data.description);

    struct tm t;
    unixTimeStampToDate(t, data.startTime);
    QDateTime startTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
    m_start->setText(startTime.toString());

    unixTimeStampToDate(t, data.endTime);
    QDateTime endTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
    m_end->setText(endTime.toString());
  }
}

//--------------------------------------------------------------------
void AlertDialog::showEvent(QShowEvent *e)
{
  QDialog::showEvent(e);

  scaleDialog(this);
}

//--------------------------------------------------------------------
bool AlertDialog::showAgain() const
{
  return m_showAgain->isChecked();
}

//--------------------------------------------------------------------
void AlertDialog::changeEvent(QEvent *e)
{
  if(e && e->type() == QEvent::LanguageChange)
  {
    retranslateUi(this);
  }

  QDialog::changeEvent(e);
}
