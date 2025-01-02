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

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QToolTip>
#include <QPainter>
#include <QDateTime>

// C++
#include <time.h>
#include <numeric>

const QStringList STRINGS = {"<font color=%1><b>CO:</b></font> %2 %3",
                             "<font color=%1><b>NO:</b></font> %2 %3",
                             "<font color=%1><b>NO<sub>2</sub>:</b></font> %2 %3",
                             "<font color=%1><b>O<sub>3</sub>:</b></font> %2 %3",
                             "<font color=%1><b>SO<sub>2</sub>:</b></font> %2 %3",
                             "<font color=%1><b>PM<sub>2.5</sub>:</b></font> %2 %3",
                             "<font color=%1><b>PM<sub>10</sub>:</b></font> %2 %3",
                             "<font color=%1><b>NH<sub>3</sub>:</b></font> %2 %3"};

//--------------------------------------------------------------------
bool isNearZero (const double value)
{
  return std::abs(value) < std::numeric_limits<double>::epsilon();
}

//--------------------------------------------------------------------
PollutionWidget::PollutionWidget(const PollutionData& data)
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

  m_dateTime->setText(toTitleCase(dtTime.toString("dddd dd/MM, hh:mm")));
  m_description->setText(data.aqi_text);

  const double values[8] = {data.co, data.no, data.no2, data.o3, data.so2, data.pm2_5, data.pm10, data.nh3};
  int count = 0;
  for(int i = 0; i < 8; ++i)
  {
    if(isNearZero(values[i])) continue;
    const auto color = CONCENTRATION_COLORS.at(i);
    const auto str = STRINGS.at(i).arg(color.name()).arg(values[i]).arg(POLLUTION_UNITS);
    auto label = new QLabel(str, this);
    gridLayout->addWidget(label, count / 2, count % 2);
    ++count;
  }

  const auto airStr = tr("Air Quality");

  QString colorStr, qualityStr;
  switch(data.aqi)
  {
    case 1:
      colorStr = "green";
      break;
    case 2:
      colorStr = "cyan";
      break;
    case 3:
      colorStr = "blue";
      break;
    case 4:
      colorStr = "purple";
      break;
    default:
      colorStr = "red";
      break;
  }
  qualityStr = data.aqi_text;

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
