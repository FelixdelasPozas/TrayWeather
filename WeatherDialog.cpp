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
#include <TooltipWidget.h>

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
: QDialog          {parent, flags}
, m_temperatureLine{nullptr}
, m_forecast       {nullptr}
, m_config         {nullptr}
, m_tooltip        {nullptr}
, m_webpage        {nullptr}
{
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setupUi(this);

  auto tab = m_tabWidget->widget(1);
  m_tabWidget->removeTab(1);
  if(tab) delete tab;

  m_chartView = new QChartView;
  m_chartView->setRenderHint(QPainter::Antialiasing);
  m_chartView->setBackgroundBrush(QBrush{Qt::white});
  m_chartView->setRubberBand(QChartView::HorizontalRubberBand);
  m_chartView->setToolTip(tr("Weather forecast for the next days."));

  m_tabWidget->addTab(m_chartView, QIcon(), "Forecast");

  connect(m_reset, SIGNAL(clicked()),
          this,    SLOT(onResetButtonPressed()));

  connect(m_mapsButton, SIGNAL(clicked()),
          this,         SLOT(onMapsButtonPressed()));

  connect(m_tabWidget, SIGNAL(currentChanged(int)),
          this,        SLOT(onTabChanged(int)));

  m_reset->setVisible(false);
}

//--------------------------------------------------------------------
void WeatherDialog::setData(const ForecastData &current, const Forecast &data, Configuration &config)
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
  m_moon->setPixmap(moonIcon(current).pixmap(QSize{64,64}, QIcon::Normal, QIcon::On));
  m_description->setText(toTitleCase(current.description));
  m_icon->setPixmap(weatherIcon(current).pixmap(QSize{236,236}, QIcon::Normal, QIcon::On));
  m_temp->setText(tr("%1 %2").arg(convertKelvinTo(current.temp, config.units)).arg(temperatureUnits));
  m_temp_max->setText(tr("%1 %2").arg(convertKelvinTo(current.temp_max, config.units)).arg(temperatureUnits));
  m_temp_min->setText(tr("%1 %2").arg(convertKelvinTo(current.temp_min, config.units)).arg(temperatureUnits));
  m_cloudiness->setText(tr("%1%").arg(current.cloudiness));
  m_humidity->setText(tr("%1%").arg(current.humidity));
  m_pressure->setText(tr("%1 hPa").arg(current.pressure));
  m_wind_speed->setText(tr("%1 meter/sec").arg(current.wind_speed));
  m_wind_dir->setText(tr("%1 º (%2)").arg(static_cast<int>(current.wind_dir) % 360).arg(windDegreesToName(current.wind_dir)));

  m_moon->setToolTip(moonTooltip(current.dt));

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
  axisX->setTickCount(data.size()/3);
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

    if(tempMin > temperature) tempMin = temperature;
    if(tempMax < temperature) tempMax = temperature;

    if(rainMin > entry.rain) rainMin = entry.rain;
    if(rainMax < entry.rain) rainMax = entry.rain;

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

  auto oldChart = m_chartView->chart();
  m_chartView->setChart(forecastChart);
  m_chartView->chart()->zoomReset();

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
  m_chartView->chart()->zoomReset();
}

//--------------------------------------------------------------------
void WeatherDialog::onTabChanged(int index)
{
  m_reset->setVisible(index == 1);
}

//--------------------------------------------------------------------
void WeatherDialog::onChartHover(const QPointF& point, bool state)
{
  auto closeWidget = [](std::shared_ptr<TooltipWidget> w) { if(w && w.get()) { w->hide(); } w = nullptr; };

  if(state)
  {
    closeWidget(m_tooltip);

    int closestPoint{-1};
    QPointF distance{100, 100};
    auto iPoint = m_chartView->chart()->mapToPosition(point, m_temperatureLine);

    if(m_temperatureLine && m_temperatureLine->count() > 0)
    {
      for(int i = 0; i < m_temperatureLine->count(); ++i)
      {
        auto iPos = m_chartView->chart()->mapToPosition(m_temperatureLine->at(i), m_temperatureLine);
        auto dist = iPoint - iPos;
        if(dist.manhattanLength() < distance.manhattanLength())
        {
          distance = dist;
          closestPoint = i;
        }
      }

      if(distance.manhattanLength() < 30)
      {
        m_tooltip = std::make_shared<TooltipWidget>(m_forecast->at(closestPoint), *m_config);

        auto pos = m_chartView->mapToGlobal(m_chartView->chart()->mapToPosition(m_temperatureLine->at(closestPoint), m_temperatureLine).toPoint());
        pos = QPoint{pos.x()-m_tooltip->width()/2, pos.y() - m_tooltip->height() - 5};
        m_tooltip->move(pos);
        m_tooltip->show();
      }
    }
  }
  else
  {
    closeWidget(m_tooltip);
  }
}

//--------------------------------------------------------------------
void WeatherDialog::onLoadFinished(bool value)
{
  m_tabWidget->setTabText(2, tr("Maps"));

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
  return m_tabWidget->count() == 3;
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

    m_tabWidget->removeTab(2);
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
    m_tabWidget->setTabText(2, QObject::tr("Maps (%1%)").arg(progress, 2, 10, QChar('0')));
  }
}

//--------------------------------------------------------------------
void WeatherDialog::onAreaChanged()
{
  m_reset->setEnabled(m_chartView->chart() && m_chartView->chart()->isZoomed());
}
