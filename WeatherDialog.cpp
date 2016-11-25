/*
 File: WeatherDialog.cpp
 Created on: 24/11/2016
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
#include <WeatherDialog.h>

// Qt
#include <QTime>
#include <QIcon>
#include <QtCharts/qchart.h>
#include <QtCharts/qchartview.h>
#include <QtCharts/qlineseries.h>
#include <QtCharts/qdatetimeaxis.h>
#include <QtCharts/qvalueaxis.h>
#include <QtCharts/qbarseries.h>
#include <QtCharts/qareaseries.h>
#include <QtCharts/qbarset.h>

using namespace QtCharts;

//--------------------------------------------------------------------
WeatherDialog::WeatherDialog(QWidget* parent, Qt::WindowFlags flags)
: QDialog{parent, flags}
{
  setupUi(this);

  m_tabWidget->removeTab(1);

  m_chartView = new QChartView{m_tabWidget};
  m_chartView->setRenderHint(QPainter::Antialiasing);
  m_chartView->setBackgroundBrush(QBrush{Qt::white});
  m_chartView->setRubberBand(QChartView::HorizontalRubberBand);

  m_tabWidget->addTab(m_chartView, QIcon(), "Weather Forecast");
  m_chartView->setToolTip(tr("Next days weather forecast"));

  connect(m_reset, SIGNAL(clicked()),
          this,    SLOT(onResetPressed()));

  connect(m_tabWidget, SIGNAL(currentChanged(int)),
          this,        SLOT(onTabChanged(int)));

  m_reset->setVisible(false);
}

//--------------------------------------------------------------------
void WeatherDialog::setData(const ForecastData &current, const Forecast &data, const Temperature units)
{
  struct tm t;
  unixTimeStampToDate(t, current.dt);
  QChar fillChar{'0'};
  auto temperatureUnits = (units == Temperature::CELSIUS ? tr("ºC") : tr("Fº"));

  m_updateTime->setText(tr("Weather station time: %1/%2/%3 at %4:%5 hours.").arg(t.tm_mday, 2, 10, fillChar)
                                                                            .arg(t.tm_mon + 1, 2, 10, fillChar)
                                                                            .arg(t.tm_year + 1900, 4, 10)
                                                                            .arg(t.tm_hour)
                                                                            .arg(t.tm_min, 2, 10, fillChar));

  m_moon->setPixmap(moonIcon(current).pixmap(QSize{64,64}, QIcon::Normal, QIcon::On));

  m_description->setText(toTitleCase(current.description));
  m_icon->setPixmap(weatherIcon(current).pixmap(QSize{236,236}, QIcon::Normal, QIcon::On));
  m_temp->setText(tr("%1 %2").arg(convertKelvinTo(current.temp, units)).arg(temperatureUnits));
  m_temp_max->setText(tr("%1 %2").arg(convertKelvinTo(current.temp_max, units)).arg(temperatureUnits));
  m_temp_min->setText(tr("%1 %2").arg(convertKelvinTo(current.temp_min, units)).arg(temperatureUnits));
  m_cloudiness->setText(tr("%1%").arg(current.cloudiness));
  m_humidity->setText(tr("%1%").arg(current.humidity));
  m_pressure->setText(tr("%1 hPa").arg(current.pressure));
  m_wind_speed->setText(tr("%1 meter/sec").arg(current.wind_speed));
  m_wind_dir->setText(tr("%1 º").arg(current.wind_dir));

  if(current.rain == 0)
  {
    m_rain->setText("None");
  }
  else
  {
    m_rain->setText(tr("%1 mm").arg(current.rain));
  }

  if(current.snow == 0)
  {
    m_snow->setText("None");
  }
  else
  {
    m_snow->setText(tr("%1 mm").arg(current.snow));
  }

  auto forecastChart = new QChart();
  forecastChart->legend()->setVisible(true);
  forecastChart->legend()->setAlignment(Qt::AlignBottom);

  auto axisX = new QDateTimeAxis;
  axisX->setTickCount(data.size()/3);
  axisX->setLabelsAngle(65);
  axisX->setFormat("dd (hh)");
  axisX->setTitleText("Day (Hour)");
  forecastChart->addAxis(axisX, Qt::AlignBottom);

  auto axisYTemp = new QValueAxis();
  axisYTemp->setLabelFormat("%i");
  axisYTemp->setTitleText(tr("Temperature in %1").arg(temperatureUnits));
  forecastChart->addAxis(axisYTemp, Qt::AlignLeft);

  auto axisYRain = new QValueAxis();
  axisYRain->setLabelFormat("%i");
  axisYRain->setTitleText("Rain accumulation in mm");
  forecastChart->addAxis(axisYRain, Qt::AlignRight);

  QPen pen;
  pen.setWidth(2);
  pen.setColor(QColor{90,90,235});

  auto temperatureLine = new QLineSeries(forecastChart);
  temperatureLine->setName(tr("Temperature"));
  temperatureLine->setPointsVisible(true);
  temperatureLine->setPen(pen);

  auto bars = new QBarSet("Rain accumulation");
  bars->setColor(QColor{100,235,100});

  auto rainBars = new QBarSeries(forecastChart);
  rainBars->setUseOpenGL(true);
  rainBars->setBarWidth(rainBars->barWidth()*2);
  rainBars->append(bars);

  double tempMin = 100, tempMax = -100;
  double rainMin = 100, rainMax = -100;
  for(auto &entry: data)
  {
    unixTimeStampToDate(t, entry.dt);
    QDateTime dtTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};

    auto temperature = convertKelvinTo(entry.temp, units);
    temperatureLine->append(dtTime.toMSecsSinceEpoch(), temperature);

    if(tempMin > temperature) tempMin = temperature;
    if(tempMax < temperature) tempMax = temperature;

    if(rainMin > entry.rain) rainMin = entry.rain;
    if(rainMax < entry.rain) rainMax = entry.rain;

    bars->append(entry.rain);
  }

  axisYTemp->setRange(tempMin-1, tempMax+1);
  axisYRain->setRange(rainMin, rainMax*2);

  forecastChart->addSeries(rainBars);
  forecastChart->addSeries(temperatureLine);
  forecastChart->setAxisX(axisX, temperatureLine);
  forecastChart->setAxisY(axisYTemp, temperatureLine);
  forecastChart->setAxisY(axisYRain, rainBars);

  auto oldChart = m_chartView->chart();
  m_chartView->setChart(forecastChart);
  if(oldChart) delete oldChart;

  onResetPressed();
}

//--------------------------------------------------------------------
void WeatherDialog::onResetPressed()
{
  m_chartView->chart()->zoomReset();
}

//--------------------------------------------------------------------
void WeatherDialog::onTabChanged(int index)
{
  m_reset->setVisible(index == 1);
}
