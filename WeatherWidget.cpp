/*
 File: TooltipWidget.cpp
 Created on: 26/11/2016
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
#include <WeatherWidget.h>

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QToolTip>
#include <QPainter>
#include <QDateTime>

// C++
#include <time.h>

//--------------------------------------------------------------------
WeatherWidget::WeatherWidget(const ForecastData& data, const Configuration &config)
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
  m_description->setText(toTitleCase(data.description));
  m_temperature->setText(tr("Temperature: %1 %2").arg(convertKelvinTo(data.temp, config.units)).arg(config.units == Temperature::CELSIUS ? "ºC" : "ºF"));
  m_icon->setPixmap(weatherPixmap(data).scaled(QSize{64,64}));
  m_cloudiness->setText(tr("Cloudiness: %1%").arg(data.cloudiness));
  m_humidity->setText(tr("Humidity: %1%").arg(data.humidity));
  m_pressure->setText(tr("Pressure: %1 hPa").arg(data.pressure));
  if(data.rain == 0)
  {
    m_rain->hide();
  }
  else
  {
    m_rain->setText(tr("Rain acc. %1 mm").arg(data.rain));
  }

  if(data.snow == 0)
  {
    m_snow->hide();
  }
  else
  {
    m_snow->setText(tr("Snow acc. %1 mm").arg(data.snow));
  }
}

//--------------------------------------------------------------------
void WeatherWidget::paintEvent(QPaintEvent* event)
{
  setFixedSize(minimumSize());

  QPainter painter(this);
  painter.drawRect(0, 0, width()-1, height()-1);
  painter.end();

  QWidget::paintEvent(event);
}
