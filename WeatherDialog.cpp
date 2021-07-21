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
#include <QWebFrame>
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

  m_tabWidget->addTab(m_weatherChart, QIcon(), tr("Forecast"));

  m_pollutionChart = new QChartView;
  m_pollutionChart->setRenderHint(QPainter::Antialiasing);
  m_pollutionChart->setBackgroundBrush(QBrush{Qt::white});
  m_pollutionChart->setRubberBand(QChartView::HorizontalRubberBand);
  m_pollutionChart->setToolTip(tr("Pollution forecast for the next days."));
  m_pollutionChart->setContentsMargins(0, 0, 0, 0);

  m_tabWidget->addTab(m_pollutionChart, QIcon(), tr("Pollution"));

  connect(m_reset, SIGNAL(clicked()),
          this,    SLOT(onResetButtonPressed()));

  connect(m_mapsButton, SIGNAL(clicked()),
          this,         SLOT(onMapsButtonPressed()));

  connect(m_tabWidget, SIGNAL(currentChanged(int)),
          this,        SLOT(onTabChanged(int)));

  m_reset->setVisible(false);
}

//--------------------------------------------------------------------
WeatherDialog::~WeatherDialog()
{
  updateMapLayerValues();
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
  auto temperatureUnits = (config.units == Temperature::CELSIUS ? "ºC" : "Fº");

  // translation
  const auto windUnits = tr("meter/sec");
  const auto illuStr   = tr("Illumination");
  const auto currStr   = tr("Current weather");
  const auto noneStr   = tr("None");
  const auto unknStr   = tr("Unknown");
  const auto tempStr   = tr("Temperature");
  const auto rainStr   = tr("Rain accumulation");

  if(config.useGeolocation)
  {
    m_location->setText(QString("%1, %2 - %3").arg(config.city).arg(config.country).arg(toTitleCase(dtTime.toString("dddd dd/MM, hh:mm"))));
  }
  else
  {
    m_location->setText(QString("%1, %2 - %3").arg(current.name).arg(current.country).arg(toTitleCase(dtTime.toString("dddd dd/MM, hh:mm"))));
  }
  m_moon->setPixmap(moonPixmap(current).scaled(QSize{64,64}));
  m_description->setText(toTitleCase(current.description));
  m_icon->setPixmap(weatherPixmap(current).scaled(QSize{236,236}));
  m_temp->setText(QString("%1 %2").arg(convertKelvinTo(current.temp, config.units)).arg(temperatureUnits));
  m_temp_max->setText(QString("%1 %2").arg(convertKelvinTo(current.temp_max, config.units)).arg(temperatureUnits));
  m_temp_min->setText(QString("%1 %2").arg(convertKelvinTo(current.temp_min, config.units)).arg(temperatureUnits));
  m_cloudiness->setText(QString("%1%").arg(current.cloudiness));
  m_humidity->setText(QString("%1%").arg(current.humidity));
  m_pressure->setText(QString("%1 hPa").arg(current.pressure));
  m_wind_speed->setText(QString("%1 %2").arg(current.wind_speed).arg(windUnits));
  m_wind_dir->setText(QString("%1 º (%2)").arg(static_cast<int>(current.wind_dir) % 360).arg(windDegreesToName(current.wind_dir)));

  double illuminationPercent = 0;
  const auto moonPhase = moonPhaseText(current.dt, illuminationPercent);
  m_moon_phase->setText(moonPhase);
  m_illumination->setText(QString("%1% %2").arg(static_cast<int>(illuminationPercent * 100)).arg(illuStr));

  m_moon->setToolTip(moonTooltip(current.dt));
  m_icon->setToolTip(QString("%1: %2").arg(currStr).arg(current.description));

  if(current.rain == 0)
  {
    m_rain->setText(noneStr);
  }
  else
  {
    m_rain->setText(QString("%1 mm").arg(current.rain));
  }

  if(current.snow == 0)
  {
    m_snow->setText(noneStr);
  }
  else
  {
    m_snow->setText(QString("%1 mm").arg(current.snow));
  }

  if(current.sunrise != 0)
  {
    unixTimeStampToDate(t, current.sunrise);
    QDateTime dtTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
    m_sunrise->setText(dtTime.toString("hh:mm"));
  }
  else
  {
    m_sunrise->setText(unknStr);
  }

  if(current.sunset != 0)
  {
    unixTimeStampToDate(t, current.sunset);
    QDateTime dtTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
    m_sunset->setText(dtTime.toString("hh:mm"));
  }
  else
  {
    m_sunset->setText(unknStr);
  }

  // Forecast tab
  auto axisX = new QDateTimeAxis();
  axisX->setTickCount(13);
  axisX->setLabelsAngle(45);
  axisX->setFormat("dd (hh)");
  axisX->setTitleText(tr("Day (Hour)"));

  auto axisYTemp = new QValueAxis();
  axisYTemp->setLabelFormat("%i");
  axisYTemp->setTitleText(tr("%1 in %2").arg(tempStr).arg(temperatureUnits));

  auto axisYRain = new QValueAxis();
  axisYRain->setLabelFormat("%.2f");
  axisYRain->setTitleText(tr("%1 in mm").arg(rainStr));

  auto forecastChart = new QChart();
  forecastChart->legend()->setVisible(true);
  forecastChart->legend()->setAlignment(Qt::AlignBottom);
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
  m_temperatureLine->setName(tempStr);
  m_temperatureLine->setUseOpenGL(true);
  m_temperatureLine->setPointsVisible(true);
  m_temperatureLine->setPen(pen);

  auto bars = new QBarSet(rainStr);
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

  const auto scale = dpiScale();
  if(scale != 1.)
  {
    auto font = forecastChart->legend()->font();
    font.setPointSize(font.pointSize()*scale);
    forecastChart->legend()->setFont(font);

    font = axisX->titleFont();
    font.setPointSize(font.pointSize()*scale);
    axisX->setTitleFont(font);

    font = axisYRain->labelsFont();
    font.setPointSize(font.pointSize()*scale);
    axisYRain->setLabelsFont(font);

    font = axisYRain->titleFont();
    font.setPointSize(font.pointSize()*scale);
    axisYRain->setTitleFont(font);

    font = axisYTemp->labelsFont();
    font.setPointSize(font.pointSize()*scale);
    axisYTemp->setLabelsFont(font);

    font = axisYTemp->titleFont();
    font.setPointSize(font.pointSize()*scale);
    axisYTemp->setTitleFont(font);

    forecastChart->adjustSize();
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

  // Maps tab handled on showEvent to avoid main widget resize problem.
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
  m_config->lastTab = index;
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
  if(m_tabWidget->count() == 4) m_tabWidget->setTabText(3, tr("Maps"));

  if(m_webpage) m_webpage->setProperty("finished", value);

  if(isVisible() && !value)
  {
    QMessageBox msg{ this };
    msg.setWindowTitle(tr("TrayWeather Maps"));
    msg.setWindowIcon(QIcon{":/TrayWeather/application.ico"});
    msg.setText(tr("The weather maps couldn't be loaded."));
    msg.exec();

    removeMaps();
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
  emit mapsEnabled(m_config->mapsEnabled);

  if(enabled)
  {
    removeMaps();
  }
  else
  {
    m_mapsButton->setText(tr("Hide Maps"));
    m_mapsButton->setToolTip(tr("Hide weather maps tab."));

    m_webpage = new QWebView;
    m_webpage->setProperty("finished", false);
    m_webpage->setRenderHint(QPainter::HighQualityAntialiasing, true);
    m_webpage->setContextMenuPolicy(Qt::NoContextMenu);
    m_webpage->setAcceptDrops(false);
    m_webpage->settings()->setAttribute(QWebSettings::Accelerated2dCanvasEnabled, true);
    m_webpage->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    m_webpage->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, false);
    m_webpage->settings()->setAttribute(QWebSettings::AcceleratedCompositingEnabled, true);
    m_webpage->settings()->setAttribute(QWebSettings::JavascriptCanCloseWindows, false);
    m_webpage->setToolTip(tr("Weather Maps."));

    connect(m_webpage, SIGNAL(loadFinished(bool)),
            this,      SLOT(onLoadFinished(bool)));

    connect(m_webpage, SIGNAL(loadProgress(int)),
            this,      SLOT(onLoadProgress(int)));

    m_tabWidget->addTab(m_webpage, QIcon(), tr("Maps"));

    loadMaps();
  }
}

//--------------------------------------------------------------------
void WeatherDialog::onLoadProgress(int progress)
{
  if(mapsEnabled())
  {
    const auto mapsStr = tr("Maps");
    m_tabWidget->setTabText(3, QString("%1 (%2%)").arg(mapsStr).arg(progress, 2, 10, QChar('0')));
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

  QString qualityStr = tr("Unknown");

  if(!m_pollution->empty())
  {
    switch(m_pollution->first().aqi)
    {
      case 1:
        qualityStr = tr("Good");
        break;
      case 2:
        qualityStr = tr("Fair");
        break;
      case 3:
        qualityStr = tr("Moderate");
        break;
      case 4:
        qualityStr = tr("Poor");
        break;
      default:
        qualityStr = tr("Very poor");
        break;
    }
  }

  m_air_quality->setText(qualityStr);

  auto axisX = new QDateTimeAxis();
  axisX->setTickCount(13);
  axisX->setLabelsAngle(45);
  axisX->setFormat("dd (hh)");
  axisX->setTitleText(tr("Day (Hour)"));
  axisX->setGridLineColor(Qt::gray);

  auto axisY = new QValueAxis();
  axisY->setLabelFormat("%i");
  axisY->setTitleText(tr("Concentration in %1").arg(CONCENTRATION_UNITS));
  axisY->setGridLineVisible(true);
  axisY->setGridLineColor(Qt::gray);

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
    m_pollutionLine[i] = new QSplineSeries(forecastChart);
    m_pollutionLine[i]->setName(CONCENTRATION_NAMES.at(i));
    m_pollutionLine[i]->setUseOpenGL(true);
    m_pollutionLine[i]->setPointsVisible(true);
    m_pollutionLine[i]->setPen(pens[i]);
  }

  QLinearGradient plotAreaGradient;
  plotAreaGradient.setStart(QPointF(0, 0));
  plotAreaGradient.setFinalStop(QPointF(1, 0));
  plotAreaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);

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

    plotAreaGradient.setColorAt(static_cast<double>(i)/(data.size()-1), pollutionColor(entry.aqi));

    std::sort(values.begin(), values.end());
    min = std::min(min, values.first());
    max = std::max(max, values.last());
  }

  forecastChart->setPlotAreaBackgroundBrush(plotAreaGradient);
  forecastChart->setPlotAreaBackgroundVisible(true);

  axisY->setRange(min, max*1.1);

  for(int i = 0; i < 8; ++i)
  {
    forecastChart->addSeries(m_pollutionLine[i]);
    forecastChart->setAxisX(axisX, m_pollutionLine[i]);
    forecastChart->setAxisY(axisY, m_pollutionLine[i]);

    connect(m_pollutionLine[i], SIGNAL(hovered(const QPointF &, bool)),
            this,              SLOT(onChartHover(const QPointF &, bool)));
  }

  const auto scale = dpiScale();
  if(scale != 1.)
  {
    auto font = forecastChart->legend()->font();
    font.setPointSize(font.pointSize()*scale);
    forecastChart->legend()->setFont(font);

    font = axisX->titleFont();
    font.setPointSize(font.pointSize()*scale);
    axisX->setTitleFont(font);

    font = axisY->labelsFont();
    font.setPointSize(font.pointSize()*scale);
    axisY->setLabelsFont(font);

    font = axisY->titleFont();
    font.setPointSize(font.pointSize()*scale);
    axisY->setTitleFont(font);

    forecastChart->adjustSize();
  }

  auto oldChart = m_pollutionChart->chart();
  m_pollutionChart->setChart(forecastChart);
  m_pollutionChart->chart()->zoomReset();

  m_reset->setEnabled(false);

  connect(axisX, SIGNAL(rangeChanged(QDateTime, QDateTime)),
          this,  SLOT(onAreaChanged()));
  connect(axisX, SIGNAL(rangeChanged(QDateTime, QDateTime)),
          this,  SLOT(onAreaChanged(QDateTime, QDateTime)));

  if(oldChart)
  {
    auto axis = qobject_cast<QDateTimeAxis *>(oldChart->axisX());
    if(axis)
    {
      disconnect(axis, SIGNAL(rangeChanged(QDateTime, QDateTime)),
                 this, SLOT(onAreaChanged()));
      disconnect(axis, SIGNAL(rangeChanged(QDateTime, QDateTime)),
                 this, SLOT(onAreaChanged(QDateTime, QDateTime)));
    }

    delete oldChart;
  }
}

//--------------------------------------------------------------------
void WeatherDialog::showEvent(QShowEvent *e)
{
  QDialog::showEvent(e);

  scaleDialog(this);

  const auto wSize = size();
  if(wSize.width() < 717 || wSize.height() < 515) setFixedSize(717, 515);

  if(m_config->mapsEnabled)
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
}

//--------------------------------------------------------------------
void WeatherDialog::onAreaChanged(QDateTime begin, QDateTime end)
{
  QLinearGradient plotAreaGradient;
  plotAreaGradient.setStart(QPointF(0, 0));
  plotAreaGradient.setFinalStop(QPointF(1, 0));
  plotAreaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);

  auto interpolateDt = [&begin, &end](const long long int dt)
  {
    return static_cast<double>(dt-begin.toMSecsSinceEpoch())/(end.toMSecsSinceEpoch()-begin.toMSecsSinceEpoch());
  };

  struct tm t;
  for(int i = 0; i < m_pollution->size(); ++i)
  {
    const auto &entry = m_pollution->at(i);

    unixTimeStampToDate(t, entry.dt);
    const auto dtTime = QDateTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
    const auto msec = dtTime.toMSecsSinceEpoch();

    if(msec < begin.toMSecsSinceEpoch())
    {
      plotAreaGradient.setColorAt(0., pollutionColor(entry.aqi));
      continue;
    }

    if(msec > end.toMSecsSinceEpoch())
    {
      plotAreaGradient.setColorAt(1., pollutionColor(entry.aqi));
      break;
    }

    plotAreaGradient.setColorAt(interpolateDt(msec), pollutionColor(entry.aqi));
  }

  auto chart = m_pollutionChart->chart();
  chart->setPlotAreaBackgroundBrush(plotAreaGradient);
  chart->setPlotAreaBackgroundVisible(true);
  chart->update();
}

//--------------------------------------------------------------------
QColor WeatherDialog::pollutionColor(const int aqiValue)
{
  QColor gradientColor;
  switch(aqiValue)
  {
    case 1:
      gradientColor = QColor::fromRgb(180, 230, 180);
      break;
    case 2:
      gradientColor = QColor::fromRgb(180, 230, 230);
      break;
    case 3:
      gradientColor = QColor::fromRgb(180, 180, 230);
      break;
    case 4:
      gradientColor = QColor::fromRgb(230, 180, 230);
      break;
    default:
      gradientColor = QColor::fromRgb(230, 180, 180);
      break;
  }

  return gradientColor;
}

//--------------------------------------------------------------------
void WeatherDialog::updateMapLayerValues()
{
  if(m_webpage != nullptr && m_webpage->property("finished").toBool())
  {
    auto value = m_webpage->page()->mainFrame()->evaluateJavaScript("customGetLayer();");
    if(!value.isNull())
    {
      auto result = value.toString();

      QStringList translations, original;
      translations << tr("Temperature") << tr("Rain") << tr("Wind") << tr("Clouds");
      original     << "temperature"     << "rain"     << "wind"     << "clouds";

      const auto it = std::find(translations.cbegin(), translations.cend(), result);
      int dist = 0;
      if(it != translations.cend()) dist = std::distance(translations.cbegin(), it);
      m_config->lastLayer = original.at(dist);
    }

    value = m_webpage->page()->mainFrame()->evaluateJavaScript("customGetStreet();");
    if(!value.isNull())
    {
      m_config->lastStreetLayer = value.toString().compare("OpenStreetMap", Qt::CaseInsensitive) == 0 ? "mapnik":"mapnikbw";
    }
  }
}

//--------------------------------------------------------------------
void WeatherDialog::changeEvent(QEvent *e)
{
  if(e && e->type() == QEvent::LanguageChange)
  {
    retranslateUi(this);

    if(mapsEnabled())
    {
      loadMaps();
    }

    const QStringList tabNames{ tr("Current Weather"), tr("Forecast"), tr("Pollution"), tr("Maps") };
    for(int i = 0; i < m_tabWidget->count(); ++i)
    {
      m_tabWidget->setTabText(i, tabNames.at(i));
    }
  }

  QDialog::changeEvent(e);
}

//--------------------------------------------------------------------
void WeatherDialog::loadMaps()
{
  QFile webfile(":/TrayWeather/webpage.html");
  if(webfile.open(QFile::ReadOnly))
  {
    QString webpage{webfile.readAll()};

    // translation
    QStringList translations;
    translations << tr("Temperature") << tr("Rain") << tr("Wind") << tr("Clouds");

    webpage.replace("%%tempStr%%", tr("Temperature"), Qt::CaseSensitive);
    webpage.replace("%%rainStr%%", tr("Rain"), Qt::CaseSensitive);
    webpage.replace("%%windStr%%", tr("Wind"), Qt::CaseSensitive);
    webpage.replace("%%cloudStr%%", tr("Clouds"), Qt::CaseSensitive);

    // config
    webpage.replace("%%lat%%", QString::number(m_config->latitude), Qt::CaseSensitive);
    webpage.replace("%%lon%%", QString::number(m_config->longitude), Qt::CaseSensitive);
    webpage.replace("{api_key}", m_config->owm_apikey, Qt::CaseSensitive);
    webpage.replace("%%streetmap%%", m_config->lastStreetLayer, Qt::CaseSensitive);
    webpage.replace("%%layermap%%", m_config->lastLayer, Qt::CaseSensitive);

    m_webpage->setHtml(webpage);
    webfile.close();
  }
  else
  {
    const auto message = tr("Unable to load weather webpage");
    m_webpage->setHtml(QString("<p style=\"color:red\"><h1>%1</h1></p>").arg(message));
  }
}

//--------------------------------------------------------------------
void WeatherDialog::removeMaps()
{
  m_mapsButton->setText(tr("Show Maps"));
  m_mapsButton->setToolTip(tr("Show weather maps tab."));

  m_tabWidget->removeTab(3);

  if(m_webpage)
  {
    if(m_webpage->property("finished").toBool())
    {
      updateMapLayerValues();
    }

    disconnect(m_webpage, SIGNAL(loadFinished(bool)),
               this,      SLOT(onLoadFinished(bool)));

    disconnect(m_webpage, SIGNAL(loadProgress(int)),
               this,      SLOT(onLoadProgress(int)));

    m_webpage->deleteLater();
    m_webpage = nullptr;
  }
}
