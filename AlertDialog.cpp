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

int currentAlert = 0; /** index of current alert being shown. */
constexpr int DEFAULT_LOGICAL_DPI = 96;

//--------------------------------------------------------------------
AlertDialog::AlertDialog(QWidget *p, Qt::WindowFlags f)
: QDialog(p, f)
{
  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & ~(Qt::WindowMaximizeButtonHint) & ~(Qt::WindowMinimizeButtonHint));
  setupUi(this);

  connectSignals();
}

//--------------------------------------------------------------------
void AlertDialog::setAlertData(const Alerts &alerts)
{
  assert(!alerts.empty());
  m_alerts = alerts;

  const auto numAlerts = m_alerts.count();
  const bool isEnabled = numAlerts > 1;

  m_alertsNum->setVisible(isEnabled);
  m_alertsNumLabel->setVisible(isEnabled);
  m_alertsNum->setText(QString("%1/%2").arg(currentAlert + 1).arg(numAlerts));
  m_next->setVisible(isEnabled);
  m_previous->setVisible(isEnabled);
  m_previous->setEnabled(isEnabled);
  m_nextLayout->setEnabled(isEnabled);

  showAlert(currentAlert);
}

//--------------------------------------------------------------------
void AlertDialog::showEvent(QShowEvent *e)
{
  QDialog::showEvent(e);

  scaleAlertDialog();
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

//--------------------------------------------------------------------
void AlertDialog::onNextButtonClicked()
{
  if(currentAlert == m_alerts.count() - 1) return;
  showAlert(++currentAlert);
  scaleAlertDialog();
}

//--------------------------------------------------------------------
void AlertDialog::onPreviousButtonClicked()
{
  if(currentAlert == 0) return;
  showAlert(--currentAlert);
  scaleAlertDialog();
}

//--------------------------------------------------------------------
void AlertDialog::connectSignals()
{
  connect(m_next, SIGNAL(clicked()), this, SLOT(onNextButtonClicked()));
  connect(m_previous, SIGNAL(clicked()), this, SLOT(onPreviousButtonClicked()));
}

//--------------------------------------------------------------------
void AlertDialog::showAlert(const int index)
{
  const auto data = m_alerts.at(index);
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

  if(m_alerts.count() != 1)
  {
    m_alertsNum->setText(QString("%1/%2").arg(index + 1).arg(m_alerts.count()));
    m_previous->setEnabled(index > 0);
    m_next->setEnabled(index < m_alerts.count() - 1);
  }
}

//--------------------------------------------------------------------
void AlertDialog::scaleAlertDialog()
{
  const auto scale = (this->logicalDpiX() == DEFAULT_LOGICAL_DPI) ? 1. : 1.25;

  setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
  setMinimumSize(400, 0);

  adjustSize();
  setFixedSize(size() * scale);
}
