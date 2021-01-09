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
#include <WeatherWidget.h>
#include <PollutionWidget.h>
#include <Utils.h>

// Qt
#include <QTime>
#include <QIcon>
#include <charts/qchart.h>
#include <charts/qchartview.h>
#include <charts/splinechart/qsplineseries.h>
#include <charts/axis/datetimeaxis/qdatetimeaxis.h>
#include <charts/axis/valueaxis/qvalueaxis.h>
#include <charts/barchart/vertical/bar/qbarseries.h>
#include <charts/areachart/qareaseries.h>
#include <charts/barchart/qbarset.h>
#include <QEasingCurve>
#include <QWebView>
#include <QMessageBox>

using namespace QtCharts;

//--------------------------------------------------------------------
WeatherDialog::WeatherDialog(QWidget* parent, Qt::WindowFlags flags)
: QDialog           {parent, flags}
, m_temperatureLine {nullptr}
, m_forecast        {nullptr}
, m_config          {nullptr}
, m_weatherTooltip  {nullptr}
, m_pollutionTooltip{nullptr}
, m_webpage         {nullptr}
{
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setupUi(this);

  m_tabWidget->setContentsMargins(0, 0, 0, 0);

  m_weatherChart = new QChartView;
  m_weatherChart->setRenderHint(QPainter::Antialiasing);
  m_weatherChart->setBackgroundBrush(QBrush{Qt::white});
  m_weatherChart->setRubberBand(QChartView::HorizontalRubberBand);
  m_weatherChart->setToolTip(tr("Weather forecast for the next days."));
  m_weatherChart->setContentsMargins(0, 0, 0, 0);

  m_tabWidget->addTab(m_weatherChart, QIcon(), "Forecast");

  m_pollutionChart = new QChartView;
  m_pollutionChart->setRenderHint(QPainter::Antialiasing);
  m_pollutionChart->setBackgroundBrush(QBrush{Qt::white});
  m_pollutionChart->setRubberBand(QChartView::HorizontalRubberBand);
  m_pollutionChart->setToolTip(tr("Pollution forecast for the next days."));
  m_pollutionChart->setContentsMargins(0, 0, 0, 0);

  m_tabWidget->addTab(m_pollutionChart, QIcon(), "Pollution");

  connect(m_reset, SIGNAL(clicked()),
          this,    SLOT(onResetButtonPressed()));

  connect(m_mapsButton, SIGNAL(clicked()),
          this,         SLOT(onMapsButtonPressed()));

  connect(m_tabWidget, SIGNAL(currentChanged(int)),
          this,        SLOT(onTabChanged(int)));

  m_reset->setVisible(false);
}

//--------------------------------------------------------------------
void WeatherDialog::setWeatherData(const ForecastData &current, const Forecast &data, Configuration &config)
{
  m_forecast = &data;
  m_config   = &config;

  // Current weather tab
  struct tm t;
  unixTimeStampToDate(t, current.dt);
  QDateTime dtTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
  auto temperatureUnits = (config.units == Temperature::CELSIUS ? tr("ºC") : tr("Fº"));

  if(config.useGeolocation)
  {
    m_location->setText(tr("%1, %2 - %3").arg(config.city).arg(config.country).arg(toTitleCase(dtTime.toString("dddd dd/MM, hh:mm"))));
  }
  else
  {
    m_location->setText(tr("%1, %2 - %3").arg(current.name).arg(current.country).arg(toTitleCase(dtTime.toString("dddd dd/MM, hh:mm"))));
  }
  m_moon->setPixmap(moonPixmap(current).scaled(QSize{64,64}));
  m_description->setText(toTitleCase(current.description));
  m_icon->setPixmap(weatherPixmap(current).scaled(QSize{236,236}));
  m_temp->setText(tr("%1 %2").arg(convertKelvinTo(current.temp, config.units)).arg(temperatureUnits));
  m_temp_max->setText(tr("%1 %2").arg(convertKelvinTo(current.temp_max, config.units)).arg(temperatureUnits));
  m_temp_min->setText(tr("%1 %2").arg(convertKelvinTo(current.temp_min, config.units)).arg(temperatureUnits));
  m_cloudiness->setText(tr("%1%").arg(current.cloudiness));
  m_humidity->setText(tr("%1%").arg(current.humidity));
  m_pressure->setText(tr("%1 hPa").arg(current.pressure));
  m_wind_speed->setText(tr("%1 meter/sec").arg(current.wind_speed));
  m_wind_dir->setText(tr("%1 º (%2)").arg(static_cast<int>(current.wind_dir) % 360).arg(windDegreesToName(current.wind_dir)));

  m_moon->setToolTip(moonTooltip(current.dt));
  m_icon->setToolTip(tr("Current weather: %1").arg(current.description));

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

  if(current.sunrise != 0)
  {
    unixTimeStampToDate(t, current.sunrise);
    QDateTime dtTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
    m_sunrise->setText(dtTime.toString("hh:mm"));
  }
  else
  {
    m_sunrise->setText("Unknown");
  }

  if(current.sunset != 0)
  {
    unixTimeStampToDate(t, current.sunset);
    QDateTime dtTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
    m_sunset->setText(dtTime.toString("hh:mm"));
  }
  else
  {
    m_sunset->setText("Unknown");
  }

  // Forecast tab
  auto axisX = new QDateTimeAxis();
  axisX->setTickCount(13);
  axisX->setLabelsAngle(45);
  axisX->setFormat("dd (hh)");
  axisX->setTitleText("Day (Hour)");

  auto axisYTemp = new QValueAxis();
  axisYTemp->setLabelFormat("%i");
  axisYTemp->setTitleText(tr("Temperature in %1").arg(temperatureUnits));

  auto axisYRain = new QValueAxis();
  axisYRain->setLabelFormat("%.2f");
  axisYRain->setTitleText("Rain accumulation in mm");

  auto forecastChart = new QChart();
  forecastChart->legend()->setVisible(true);
  forecastChart->legend()->setAlignment(Qt::AlignBottom);
  forecastChart->setAnimationDuration(400);
  forecastChart->setAnimationEasingCurve(QEasingCurve(QEasingCurve::InOutQuad));
  forecastChart->setAnimationOptions(QChart::AllAnimations);
  forecastChart->addAxis(axisX, Qt::AlignBottom);
  forecastChart->addAxis(axisYTemp, Qt::AlignLeft);
  forecastChart->addAxis(axisYRain, Qt::AlignRight);

  QPen pen;
  pen.setWidth(2);
  pen.setColor(QColor{90,90,235});

  m_temperatureLine = new QSplineSeries(forecastChart);
  m_temperatureLine->setName(tr("Temperature"));
  m_temperatureLine->setUseOpenGL(true);
  m_temperatureLine->setPointsVisible(true);
  m_temperatureLine->setPen(pen);

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
    dtTime = QDateTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};

    auto temperature = convertKelvinTo(entry.temp, config.units);
    m_temperatureLine->append(dtTime.toMSecsSinceEpoch(), temperature);

    tempMin = std::min(tempMin, temperature);
    tempMax = std::max(tempMax, temperature);

    rainMin = std::min(rainMin, entry.rain);
    rainMax = std::max(rainMax, entry.rain);

    bars->append(entry.rain);
  }

  axisYTemp->setRange(tempMin-1, tempMax+1);
  axisYRain->setRange(rainMin, rainMax*1.25);

  forecastChart->addSeries(rainBars);
  forecastChart->addSeries(m_temperatureLine);

  forecastChart->setAxisX(axisX, m_temperatureLine);
  forecastChart->setAxisY(axisYTemp, m_temperatureLine);
  forecastChart->setAxisY(axisYRain, rainBars);

  connect(m_temperatureLine, SIGNAL(hovered(const QPointF &, bool)),
          this,              SLOT(onChartHover(const QPointF &, bool)));

  if(rainMin == 0 && rainMax == 0)
  {
    axisYRain->setVisible(false);
    rainBars->clear();
  }

  auto oldChart = m_weatherChart->chart();
  m_weatherChart->setChart(forecastChart);
  m_weatherChart->chart()->zoomReset();

  m_reset->setEnabled(false);

  connect(axisX, SIGNAL(rangeChanged(QDateTime, QDateTime)),
          this,  SLOT(onAreaChanged()));

  if(oldChart)
  {
    auto axis = qobject_cast<QDateTimeAxis *>(oldChart->axisX());
    if(axis)
    {
      disconnect(axis, SIGNAL(rangeChanged(QDateTime, QDateTime)),
                 this, SLOT(onAreaChanged()));
    }

    delete oldChart;
  }

  // Maps tab
  if(config.mapsEnabled)
  {
    if(!mapsEnabled())
    {
      onMapsButtonPressed();
    }
    else
    {
      m_webpage->reload();
    }
  }
  else
  {
    if(mapsEnabled()) onMapsButtonPressed();
  }

  onResetButtonPressed();
}

//--------------------------------------------------------------------
void WeatherDialog::onResetButtonPressed()
{
  if(m_tabWidget->currentIndex() == 1)
  {
    m_weatherChart->chart()->zoomReset();
  }
  else
  {
    m_pollutionChart->chart()->zoomReset();
  }
}

//--------------------------------------------------------------------
void WeatherDialog::onTabChanged(int index)
{
  m_reset->setVisible(index == 1 || index == 2);
  onAreaChanged();
}

//--------------------------------------------------------------------
void WeatherDialog::onChartHover(const QPointF& point, bool state)
{
  QtCharts::QLineSeries *currentLine = qobject_cast<QtCharts::QLineSeries *>(sender());
  if(!currentLine) return;

  QtCharts::QChartView *currentChart = (m_tabWidget->currentIndex() == 1) ? m_weatherChart : m_pollutionChart;

  auto closeWidgets = [this, &currentChart]()
  {
    if(currentChart == m_weatherChart && m_weatherTooltip)
    {
      m_weatherTooltip->hide();
      m_weatherTooltip = nullptr;
    }
    else
    {
      if(m_pollutionTooltip)
      {
        m_pollutionTooltip->hide();
        m_pollutionTooltip = nullptr;
      }
    }
  };

  if(state)
  {
    closeWidgets();

    int closestPoint{-1};
    QPointF distance{100, 100};
    auto iPoint = currentChart->chart()->mapToPosition(point, currentLine);

    if(currentLine && currentLine->count() > 0)
    {
      for(int i = 0; i < currentLine->count(); ++i)
      {
        auto iPos = currentChart->chart()->mapToPosition(currentLine->at(i), currentLine);
        auto dist = iPoint - iPos;
        if(dist.manhattanLength() < distance.manhattanLength())
        {
          distance = dist;
          closestPoint = i;
        }
      }

      if(distance.manhattanLength() < 30)
      {
        if(currentChart == m_weatherChart)
        {
          m_weatherTooltip = std::make_shared<WeatherWidget>(m_forecast->at(closestPoint), *m_config);
          auto pos = currentChart->mapToGlobal(currentChart->chart()->mapToPosition(currentLine->at(closestPoint), currentLine).toPoint());
          pos = QPoint{pos.x()-m_weatherTooltip->width()/2, pos.y() - m_weatherTooltip->height() - 5};
          m_weatherTooltip->move(pos);
          m_weatherTooltip->show();
        }
        else
        {
          m_pollutionTooltip = std::make_shared<PollutionWidget>(m_pollution->at(closestPoint));
          auto pos = currentChart->mapToGlobal(currentChart->chart()->mapToPosition(currentLine->at(closestPoint), currentLine).toPoint());
          pos = QPoint{pos.x()-m_pollutionTooltip->width()/2, pos.y() - m_pollutionTooltip->height() - 5};
          m_pollutionTooltip->move(pos);
          m_pollutionTooltip->show();
        }
      }
    }
  }
  else
  {
    closeWidgets();
  }
}

//--------------------------------------------------------------------
void WeatherDialog::onLoadFinished(bool value)
{
  m_tabWidget->setTabText(3, tr("Maps"));

  if(isVisible() && !value)
  {
    QMessageBox msg{ this };
    msg.setWindowTitle(tr("TrayWeather Maps"));
    msg.setWindowIcon(QIcon{":/TrayWeather/application.ico"});
    msg.setText(tr("The weather maps couldn't be loaded."));
    msg.exec();
  }
}

//--------------------------------------------------------------------
bool WeatherDialog::mapsEnabled() const
{
  return m_tabWidget->count() == 4;
}

//--------------------------------------------------------------------
void WeatherDialog::onMapsButtonPressed()
{
  auto enabled = mapsEnabled();
  m_config->mapsEnabled = !enabled;

  if(enabled)
  {
    m_mapsButton->setText(tr("Show Maps"));
    m_mapsButton->setToolTip(tr("Show weather maps tab."));

    m_tabWidget->removeTab(3);
    delete m_webpage;
    m_webpage = nullptr;
  }
  else
  {
    m_mapsButton->setText(tr("Hide Maps"));
    m_mapsButton->setToolTip(tr("Hide weather maps tab."));

    m_webpage = new QWebView;
    m_webpage->setRenderHint(QPainter::HighQualityAntialiasing, true);
    m_webpage->setContextMenuPolicy(Qt::NoContextMenu);
    m_webpage->setAcceptDrops(false);
    m_webpage->settings()->setAttribute(QWebSettings::Accelerated2dCanvasEnabled, true);
    m_webpage->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    m_webpage->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, false);
    m_webpage->settings()->setAttribute(QWebSettings::AcceleratedCompositingEnabled, true);
    m_webpage->settings()->setAttribute(QWebSettings::JavascriptCanCloseWindows, false);
    m_webpage->setToolTip("Weather Maps.");

    connect(m_webpage, SIGNAL(loadFinished(bool)),
            this,      SLOT(onLoadFinished(bool)));

    connect(m_webpage, SIGNAL(loadProgress(int)),
            this,      SLOT(onLoadProgress(int)));

    m_tabWidget->addTab(m_webpage, QIcon(), "Maps");

    QFile webfile(":/TrayWeather/webpage.html");
    webfile.open(QFile::ReadOnly);
    QString webpage{webfile.readAll()};

    webpage.replace("%%lat%%", QString::number(m_config->latitude));
    webpage.replace("%%lon%%", QString::number(m_config->longitude));
    webpage.replace("{api_key}", m_config->owm_apikey);

    m_webpage->setHtml(webpage);
  }
}

//--------------------------------------------------------------------
void WeatherDialog::onLoadProgress(int progress)
{
  if(mapsEnabled())
  {
    m_tabWidget->setTabText(3, QObject::tr("Maps (%1%)").arg(progress, 2, 10, QChar('0')));
  }
}

//--------------------------------------------------------------------
void WeatherDialog::onAreaChanged()
{
  const auto currentIndex = m_tabWidget->currentIndex();

  switch(currentIndex)
  {
    case 1:
      m_reset->setEnabled(m_weatherChart->chart() && m_weatherChart->chart()->isZoomed());
      break;
    case 2:
      m_reset->setEnabled(m_pollutionChart->chart() && m_pollutionChart->chart()->isZoomed());
      break;
    default:
      m_reset->setEnabled(false);
      break;
  }
}

//--------------------------------------------------------------------
void WeatherDialog::setPollutionData(const Pollution &data)
{
  m_pollution = &data;

  auto axisX = new QDateTimeAxis();
  axisX->setTickCount(13);
  axisX->setLabelsAngle(45);
  axisX->setFormat("dd (hh)");
  axisX->setTitleText("Day (Hour)");

  auto axisY = new QValueAxis();
  axisY->setLabelFormat("%i");
  axisY->setTitleText(tr("Concentration in %1").arg(CONCENTRATION_UNITS));
  axisY->setGridLineVisible(true);

  auto forecastChart = new QChart();
  forecastChart->legend()->setVisible(true);
  forecastChart->legend()->setAlignment(Qt::AlignBottom);
  forecastChart->setAnimationDuration(400);
  forecastChart->setAnimationEasingCurve(QEasingCurve(QEasingCurve::InOutQuad));
  forecastChart->setAnimationOptions(QChart::AllAnimations);
  forecastChart->addAxis(axisX, Qt::AlignBottom);
  forecastChart->addAxis(axisY, Qt::AlignLeft);

  QPen pens[8];
  for(int i = 0; i < 8; ++i)
  {
    pens[i].setWidth(2);
    pens[i].setColor(CONCENTRATION_COLORS[i]);
  }

  for(int i = 0; i < 8; ++i)
  {
    m_pollutionLine[i] = new QSplineSeries(forecastChart);
    m_pollutionLine[i]->setName(tr("%1").arg(CONCENTRATION_NAMES.at(i)));
    m_pollutionLine[i]->setUseOpenGL(true);
    m_pollutionLine[i]->setPointsVisible(true);
    m_pollutionLine[i]->setPen(pens[i]);
  }

  double min = 1000, max = 0;
  struct tm t;
  for(int i = 0; i < data.size(); ++i)
  {
    const auto &entry = data.at(i);

    unixTimeStampToDate(t, entry.dt);
    auto dtTime = QDateTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
    const auto msec = dtTime.toMSecsSinceEpoch();

    QList<double> values;

    m_pollutionLine[0]->append(msec, entry.co);    values << entry.co;
    m_pollutionLine[1]->append(msec, entry.no);    values << entry.no;
    m_pollutionLine[2]->append(msec, entry.no2);   values << entry.no2;
    m_pollutionLine[3]->append(msec, entry.o3);    values << entry.o3;
    m_pollutionLine[4]->append(msec, entry.so2);   values << entry.so2;
    m_pollutionLine[5]->append(msec, entry.pm2_5); values << entry.pm2_5;
    m_pollutionLine[6]->append(msec, entry.pm10);  values << entry.pm10;
    m_pollutionLine[7]->append(msec, entry.nh3);   values << entry.nh3;

    std::sort(values.begin(), values.end());
    min = std::min(min, values.first());
    max = std::max(max, values.last());
  }

  axisY->setRange(min, max*1.1);

  for(int i = 0; i < 8; ++i)
  {
    forecastChart->addSeries(m_pollutionLine[i]);
    forecastChart->setAxisX(axisX, m_pollutionLine[i]);
    forecastChart->setAxisY(axisY, m_pollutionLine[i]);

    connect(m_pollutionLine[i], SIGNAL(hovered(const QPointF &, bool)),
            this,              SLOT(onChartHover(const QPointF &, bool)));
  }

  auto oldChart = m_pollutionChart->chart();
  m_pollutionChart->setChart(forecastChart);
  m_pollutionChart->chart()->zoomReset();

  m_reset->setEnabled(false);

  connect(axisX, SIGNAL(rangeChanged(QDateTime, QDateTime)),
          this,  SLOT(onAreaChanged()));

  if(oldChart)
  {
    auto axis = qobject_cast<QDateTimeAxis *>(oldChart->axisX());
    if(axis)
    {
      disconnect(axis, SIGNAL(rangeChanged(QDateTime, QDateTime)),
                 this, SLOT(onAreaChanged()));
    }

    delete oldChart;
  }
}
