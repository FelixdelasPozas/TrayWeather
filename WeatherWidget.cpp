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

  m_icon->setPixmap(weatherPixmap(data).scaled(QSize{64,64}));

  // for translation
  const auto tempStr = tr("Temperature");
  const auto clouStr = tr("Cloudiness");
  const auto humiStr = tr("Humidity");
  const auto presStr = tr("Pressure");
  const auto rainStr = tr("Rain acc");
  const auto snowStr = tr("Snow acc");
  const auto degUnits  = config.units == Units::METRIC ? "ºC" : "ºF";
  const auto presUnits = config.units == Units::METRIC ? tr("hPa"):tr("psi");
  const auto accUnits  = config.units == Units::METRIC ? tr("mm") : tr("inch");

  m_dateTime->setText(toTitleCase(dtTime.toString("dddd dd/MM, hh:mm")));
  m_description->setText(toTitleCase(data.description));

  m_temperature->setText(QString("%1: %2 %3").arg(tempStr).arg(data.temp).arg(degUnits));
  m_cloudiness->setText(QString("%1: %2%").arg(clouStr).arg(data.cloudiness));
  m_humidity->setText(QString("%1: %2%").arg(humiStr).arg(data.humidity));

  const auto pValue = config.units == Units::METRIC ? data.pressure : converthPaToPSI(data.pressure);
  m_pressure->setText(QString("%1: %2 %3").arg(presStr).arg(pValue).arg(presUnits));

  if(data.rain == 0)
  {
    m_rain->hide();
  }
  else
  {
    const double value = config.units == Units::METRIC ? data.rain : convertMmToInches(data.rain);
    m_rain->setText(QString("%1: %2 %3").arg(rainStr).arg(value).arg(accUnits));
  }

  if(data.snow == 0)
  {
    m_snow->hide();
  }
  else
  {
    const double value = config.units == Units::METRIC ? data.snow : convertMmToInches(data.snow);
    m_snow->setText(QString("%1: %2 %3").arg(snowStr).arg(value).arg(accUnits));
  }

  adjustSize();
  setFixedSize(size());
}

//--------------------------------------------------------------------
void WeatherWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.drawRect(0, 0, width()-1, height()-1);
  painter.end();

  QWidget::paintEvent(event);
}

//--------------------------------------------------------------------
void WeatherWidget::changeEvent(QEvent *e)
{
  if(e && e->type() == QEvent::LanguageChange)
  {
    retranslateUi(this);
  }

  QWidget::changeEvent(e);
}
