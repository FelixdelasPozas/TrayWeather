/*
 File: PollutionWidget.cpp
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

// Project
#include <PollutionWidget.h>
#include <Utils.h>

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QToolTip>
#include <QPainter>
#include <QDateTime>

// C++
#include <time.h>

//--------------------------------------------------------------------
PollutionWidget::PollutionWidget(const PollutionData& data)
: QWidget(QApplication::desktop(), Qt::WindowFlags())
{
  setupUi(this);
  setWindowFlags(Qt::ToolTip|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::WindowTransparentForInput);
  setPalette(QToolTip::palette());
  layout()->setMargin(3);
  layout()->setContentsMargins(QMargins{5,5,5,5});

  const QString units{"µg/m<sup>3</sup>"};

  struct tm t;
  unixTimeStampToDate(t, data.dt);
  QDateTime dtTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};

  m_dateTime->setText(toTitleCase(dtTime.toString("dddd dd/MM, hh:mm")));
  m_description->setText(data.aqi_text);
  m_co->setText(QString("<font color=%1><b>CO:</b></font> %2 %3").arg(CONCENTRATION_COLORS.at(0).name()).arg(data.co).arg(units));
  m_no->setText(QString("<font color=%1><b>NO:</b></font> %2 %3").arg(CONCENTRATION_COLORS.at(1).name()).arg(data.no).arg(units));
  m_no2->setText(QString("<font color=%1><b>NO<sub>2</sub>:</b></font> %2 %3").arg(CONCENTRATION_COLORS.at(2).name()).arg(data.no2).arg(units));
  m_o3->setText(QString("<font color=%1><b>O<sub>3</sub>:</b></font> %2 %3").arg(CONCENTRATION_COLORS.at(3).name()).arg(data.o3).arg(units));
  m_so2->setText(QString("<font color=%1><b>SO<sub>2</sub>:</b></font> %2 %3").arg(CONCENTRATION_COLORS.at(4).name()).arg(data.so2).arg(units));
  m_pm25->setText(QString("<font color=%1><b>PM<sub>2.5</sub>:</b></font> %2 %3").arg(CONCENTRATION_COLORS.at(5).name()).arg(data.pm2_5).arg(units));
  m_pm10->setText(QString("<font color=%1><b>PM<sub>10</sub>:</b></font> %2 %3").arg(CONCENTRATION_COLORS.at(6).name()).arg(data.pm10).arg(units));
  m_nh3->setText(QString("<font color=%1><b>NH<sub>3</sub>:</b></font> %2 %3").arg(CONCENTRATION_COLORS.at(7).name()).arg(data.nh3).arg(units));

  const auto airStr = tr("Air Quality");

  QString colorStr, qualityStr;
  switch(data.aqi)
  {
    case 1:
      colorStr = "green";
      qualityStr = tr("Good");
      break;
    case 2:
      colorStr = "cyan";
      qualityStr = tr("Fair");
      break;
    case 3:
      colorStr = "blue";
      qualityStr = tr("Moderate");
      break;
    case 4:
      colorStr = "purple";
      qualityStr = tr("Poor");
      break;
    default:
      colorStr = "red";
      qualityStr = tr("Very poor");
      break;
  }

  m_description->setText(QString("<b>%1: <font color=%2>%3</font></b>").arg(airStr).arg(colorStr).arg(qualityStr));

  adjustSize();
  setFixedSize(size());
}

//--------------------------------------------------------------------
void PollutionWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.drawRect(0, 0, width()-1, height()-1);
  painter.end();

  QWidget::paintEvent(event);
}

//--------------------------------------------------------------------
void PollutionWidget::changeEvent(QEvent *e)
{
  if(e && e->type() == QEvent::LanguageChange)
  {
    retranslateUi(this);
  }

  QWidget::changeEvent(e);
}
