/*
 File: UVWidget.cpp
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

// Project
#include <UVWidget.h>

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QToolTip>
#include <QPainter>
#include <QDateTime>

// C++
#include <time.h>
#include <cmath>

//-----------------------------------------------------------------------------
UVWidget::UVWidget(const UVData &data)
: QWidget(QApplication::desktop(), Qt::WindowFlags())
{
  setupUi(this);
  setWindowFlags(Qt::ToolTip|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::WindowTransparentForInput);
  setPalette(QToolTip::palette());
  layout()->setMargin(3);
  layout()->setContentsMargins(QMargins{5,5,5,5});

  struct tm t;
  unixTimeStampToDate(t, data.dt);
  QDateTime dtTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};

  const auto index = static_cast<int>(std::nearbyint(data.idx));
  m_dateTime->setText(toTitleCase(dtTime.toString("dddd dd/MM, hh:mm")));

  QString indexStr, suggestionStr;
  switch(index)
  {
    case 0:
    case 1:
    case 2:
      indexStr = tr("Low");
      suggestionStr = tr("No protection required.");
      break;
    case 3:
    case 4:
    case 5:
      indexStr = tr("Moderate");
      suggestionStr = tr("Protection required.");
      break;
    case 6:
    case 7:
      indexStr = tr("High");
      suggestionStr = tr("Protection required.");
      break;
    case 8:
    case 9:
    case 10:
      indexStr = tr("Very high");
      suggestionStr = tr("Extra protection.\nAvoid being outside\nduring midday hours.");
      break;
    default:
      indexStr = tr("Extreme");
      suggestionStr = tr("Extra protection.\nAvoid being outside\nduring midday hours.");
      break;
  }

  const auto color = index == 0 ? "black":uvColor(data.idx).name();
  m_level->setText(QString("<font color=%1><b>%2</b></font>").arg(color).arg(index));
  m_description->setText(QString("<b><font color=%1>%2</font></b>").arg(color).arg(indexStr));
  m_suggestion->setText(suggestionStr);

  adjustSize();
  setFixedSize(size());
}

//-----------------------------------------------------------------------------
void UVWidget::paintEvent(QPaintEvent *event)
{
  QPainter painter(this);
  painter.drawRect(0, 0, width()-1, height()-1);
  painter.end();

  QWidget::paintEvent(event);
}

//-----------------------------------------------------------------------------
void UVWidget::changeEvent(QEvent *e)
{
  if(e && e->type() == QEvent::LanguageChange)
  {
    retranslateUi(this);
  }

  QWidget::changeEvent(e);
}
