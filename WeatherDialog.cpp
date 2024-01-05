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
#include <UVWidget.h>
#include <Utils.h>

// Qt
#include <QTime>
#include <QIcon>
#include <charts/qchart.h>
#include <charts/qchartview.h>
#include <charts/qabstractseries.h>
#include <charts/linechart/qlineseries.h>
#include <charts/splinechart/qsplineseries.h>
#include <charts/axis/datetimeaxis/qdatetimeaxis.h>
#include <charts/axis/valueaxis/qvalueaxis.h>
#include <charts/axis/categoryaxis/qcategoryaxis.h>
#include <charts/barchart/vertical/bar/qbarseries.h>
#include <charts/areachart/qareaseries.h>
#include <charts/barchart/qbarset.h>
#include <charts/legend/qlegend.h>
#include <charts/legend/qlegendmarker.h>
#include <QEasingCurve>
#include <QWebView>
#include <QWebFrame>
#include <QMessageBox>

// C++
#include <cmath>
#include <algorithm>
#include <set>

using namespace QtCharts;

//--------------------------------------------------------------------
WeatherDialog::WeatherDialog(QWidget* parent, Qt::WindowFlags flags)
: QDialog           (parent, flags)
, m_forecast        {nullptr}
, m_config          {nullptr}
, m_weatherTooltip  {nullptr}
, m_pollutionTooltip{nullptr}
, m_uvTooltip       {nullptr}
, m_webpage         {nullptr}
{
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setupUi(this);

  m_tabWidget->setContentsMargins(0, 0, 0, 0);

  auto weatherTabContents = new QWidget();
  auto weatherTabLayout = new QVBoxLayout();
  weatherTabLayout->setMargin(0);
  weatherTabLayout->setSpacing(0);
  weatherTabContents->setLayout(weatherTabLayout);

  m_weatherChart = new QChartView;
  m_weatherChart->setRenderHint(QPainter::Antialiasing);
  m_weatherChart->setBackgroundBrush(QBrush{Qt::white});
  m_weatherChart->setRubberBand(QChartView::HorizontalRubberBand);
  m_weatherChart->setToolTip(tr("Weather forecast for the next days."));
  m_weatherChart->setContentsMargins(0, 0, 0, 0);

  m_weatherError = new ErrorWidget(tr("Error requesting weather data."));
  m_weatherError->setVisible(false);

  weatherTabLayout->addWidget(m_weatherChart);
  weatherTabLayout->addWidget(m_weatherError);

  m_tabWidget->addTab(weatherTabContents, QIcon(), tr("Forecast"));

  auto pollutionTabContents = new QWidget();
  auto pollutionTabLayout = new QVBoxLayout();
  pollutionTabLayout->setMargin(0);
  pollutionTabLayout->setSpacing(0);
  pollutionTabContents->setLayout(pollutionTabLayout);

  m_pollutionChart = new QChartView;
  m_pollutionChart->setRenderHint(QPainter::Antialiasing);
  m_pollutionChart->setBackgroundBrush(QBrush{Qt::white});
  m_pollutionChart->setRubberBand(QChartView::HorizontalRubberBand);
  m_pollutionChart->setToolTip(tr("Pollution forecast for the next days."));
  m_pollutionChart->setContentsMargins(0, 0, 0, 0);

  m_pollutionError = new ErrorWidget(tr("Error requesting air quality data."));
  m_pollutionError->setVisible(false);

  pollutionTabLayout->addWidget(m_pollutionChart);
  pollutionTabLayout->addWidget(m_pollutionError);

  m_tabWidget->addTab(pollutionTabContents, QIcon(), tr("Pollution"));

  auto uvTabContents = new QWidget();
  auto uvTabLayout = new QVBoxLayout();
  uvTabLayout->setMargin(0);
  uvTabLayout->setSpacing(0);
  uvTabContents->setLayout(uvTabLayout);

  m_uvChart = new QChartView;
  m_uvChart->setRenderHint(QPainter::Antialiasing);
  m_uvChart->setBackgroundBrush(QBrush{Qt::white});
  m_uvChart->setRubberBand(QChartView::HorizontalRubberBand);
  m_uvChart->setToolTip(tr("Ultraviolet radiation forecast for the next days."));
  m_uvChart->setContentsMargins(0, 0, 0, 0);

  m_uvError = new ErrorWidget(tr("Error requesting ultraviolet radiation data."));
  m_uvError->setVisible(false);

  uvTabLayout->addWidget(m_uvChart);
  uvTabLayout->addWidget(m_uvError);

  m_tabWidget->addTab(uvTabContents, QIcon(), tr("UV"));

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

  // translation
  const auto illuStr = tr("Illumination");
  const auto currStr = tr("Current weather");
  const auto noneStr = tr("None");
  const auto unknStr = tr("Unknown");
  const auto tempStr = tr("Temperature");
  const auto precStr = tr("Accumulation");

  // conversions
  QString accuStr, pressStr, windUnits, tempUnits;
  std::function<double(double)> tempFunc = [](double d){ return d; };
  std::function<double(double)> precipitationFunc = [](double d){ return d; };
  double pressureValue = current.pressure;
  double windValue = current.wind_speed;

  switch(config.units)
  {
    case Units::IMPERIAL:
      accuStr = tr("inches");
      pressStr = tr("PSI");
      windUnits = tr("miles/hour");
      tempUnits = "ºF";
      break;
    default:
    case Units::METRIC:
      accuStr = tr("mm");
      pressStr = tr("hPa");
      windUnits = tr("meter/sec");
      tempUnits = "ºC";
      break;
    case Units::CUSTOM:
      switch(config.tempUnits)
      {
        case TemperatureUnits::FAHRENHEIT:
          tempFunc = convertCelsiusToFahrenheit;
          tempUnits = "ºF";
          break;
        default:
        case TemperatureUnits::CELSIUS:
          tempUnits = "ºC";
          break;
      }
      switch(config.pressureUnits)
      {
        case PressureUnits::INHG:
          pressureValue = converthPaToinHg(current.pressure);
          pressStr = tr("inHg");
          break;
        case PressureUnits::MMGH:
          pressureValue = converthPaTommHg(current.pressure);
          pressStr = tr("mmHg");
          break;
        case PressureUnits::PSI:
          pressureValue = converthPaToPSI(current.pressure);
          pressStr = tr("PSI");
          break;
        default:
        case PressureUnits::HPA:
          pressStr = tr("hPa");
          break;
      }
      switch(config.precUnits)
      {
        case PrecipitationUnits::INCH:
          accuStr = tr("inches");
          precipitationFunc = convertMmToInches;
          break;
        default:
        case PrecipitationUnits::MM:
          accuStr = tr("mm");
          break;
      }
      switch(config.windUnits)
      {
        case WindUnits::FEETSEC:
          windUnits = tr("feet/s");
          windValue = convertMetersSecondToFeetSecond(current.wind_speed);
          break;
        case WindUnits::KMHR:
          windUnits = tr("km/h");
          windValue = convertMetersSecondToKilometersHour(current.wind_speed);
          break;
        case WindUnits::MILHR:
          windUnits = tr("miles/h");
          windValue = convertMetersSecondToMilesHour(current.wind_speed);
          break;
        default:
        case WindUnits::METSEC:
          windUnits = tr("meter/sec");
          break;
      }
      break;
  }

  if(config.useGeolocation)
  {
    m_location->setText(QString("%1, %2 - %3").arg(config.city).arg(config.country).arg(toTitleCase(dtTime.toString("dddd dd/MM, hh:mm"))));
  }
  else
  {
    m_location->setText(QString("%1, %2 - %3").arg(current.name).arg(current.country).arg(toTitleCase(dtTime.toString("dddd dd/MM, hh:mm"))));
  }
  m_moon->setPixmap(moonPixmap(current, config.iconTheme, config.iconThemeColor).scaled(QSize{64,64}, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  m_description->setText(toTitleCase(current.description));
  m_icon->setPixmap(weatherPixmap(current, config.iconTheme, config.iconThemeColor).scaled(QSize{236,236}, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  m_temp->setText(QString("%1 %2").arg(tempFunc(current.temp)).arg(tempUnits));
  m_temp_max->setText(QString("%1 %2").arg(tempFunc(current.temp_max)).arg(tempUnits));
  m_temp_min->setText(QString("%1 %2").arg(tempFunc(current.temp_min)).arg(tempUnits));
  m_cloudiness->setText(QString("%1%").arg(current.cloudiness));
  m_humidity->setText(QString("%1%").arg(current.humidity));
  m_wind_speed->setText(QString("%1 %2").arg(windValue).arg(windUnits));
  m_wind_dir->setText(QString("%1º (%2)").arg(static_cast<int>(current.wind_dir) % 360).arg(windDegreesToName(current.wind_dir)));
  m_pressure->setText(QString("%1 %2").arg(pressureValue).arg(pressStr));

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
    m_rain->setText(QString("%1 %2").arg(precipitationFunc(current.rain)).arg(accuStr));
  }

  if(current.snow == 0)
  {
    m_snow->setText(noneStr);
  }
  else
  {
    m_snow->setText(QString("%1 %2").arg(precipitationFunc(current.snow)).arg(accuStr));
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
  if(m_forecast->empty())
  {
    m_weatherChart->hide();
    m_weatherError->show();
    m_tabWidget->setTabIcon(1, QIcon(":/TrayWeather/network_error.svg"));
  }
  else
  {
    m_weatherChart->show();
    m_weatherError->hide();
    m_tabWidget->setTabIcon(1, QIcon());

    auto axisX = new QDateTimeAxis();
    axisX->setLabelsAngle(45);
    axisX->setFormat("dd (hh)");
    axisX->setTitleText(tr("Day (Hour)"));

    auto axisYTemp = new QValueAxis();
    axisYTemp->setTickCount(6);
    axisYTemp->setLabelFormat("%i");
    axisYTemp->setTitleText(tr("%1 in %2").arg(tempStr).arg(tempUnits));

    auto axisYPrec = new QValueAxis();
    axisYPrec->setTickCount(6);
    axisYPrec->setLabelFormat("%.2f");
    axisYPrec->setTitleText(tr("%1 in %2").arg(precStr).arg(accuStr));

    const auto theme = m_config->lightTheme ? QtCharts::QChart::ChartTheme::ChartThemeQt : QtCharts::QChart::ChartTheme::ChartThemeDark;

    auto forecastChart = new QChart();
    forecastChart->legend()->setVisible(true);
    forecastChart->legend()->setAlignment(Qt::AlignBottom);
    forecastChart->legend()->setToolTip(tr("Click to hide or show the forecast."));
    forecastChart->setAnimationDuration(400);
    forecastChart->setAnimationEasingCurve(QEasingCurve(QEasingCurve::InOutQuad));
    forecastChart->setAnimationOptions(QChart::AllAnimations);
    forecastChart->addAxis(axisX, Qt::AlignBottom);
    forecastChart->addAxis(axisYTemp, Qt::AlignLeft);
    forecastChart->addAxis(axisYPrec, Qt::AlignRight);
    forecastChart->setBackgroundVisible(false);
    forecastChart->setTheme(theme);

    const auto linesColor = m_config->lightTheme ? Qt::darkGray : Qt::lightGray;

    axisX->setGridLineVisible(true);
    axisX->setGridLineColor(linesColor);
    axisX->setMinorGridLineVisible(true);
    axisX->setMinorGridLineColor(linesColor);
    axisYTemp->setGridLineVisible(true);
    axisYTemp->setGridLineColor(linesColor);
    axisYTemp->setMinorGridLineVisible(true);
    axisYTemp->setMinorGridLineColor(linesColor);
    axisYPrec->setGridLineVisible(true);
    axisYPrec->setGridLineColor(linesColor);
    axisYPrec->setMinorGridLineVisible(true);
    axisYPrec->setMinorGridLineColor(linesColor);

    auto titleFont = axisX->titleFont();
    titleFont.setHintingPreference(QFont::HintingPreference::PreferFullHinting);
    titleFont.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, 1);
    titleFont.setStyleStrategy(QFont::StyleStrategy::PreferQuality);
    titleFont.setBold(true);

    auto labelsFont = axisX->labelsFont();
    labelsFont.setHintingPreference(QFont::HintingPreference::PreferFullHinting);
    labelsFont.setStyleStrategy(QFont::StyleStrategy::PreferQuality);
    labelsFont.setBold(false);

    forecastChart->setFont(titleFont);
    axisX->setTitleFont(titleFont);
    axisX->setLabelsFont(labelsFont);
    axisYTemp->setTitleFont(titleFont);
    axisYTemp->setLabelsFont(labelsFont);
    axisYPrec->setTitleFont(titleFont);
    axisYPrec->setLabelsFont(labelsFont);

    QPen pen;
    pen.setWidth(2);
    pen.setColor(QColor{90,90,235});

    auto initSerie = [&](const Representation value, const QColor &color, const QString &name)
    {
      QAbstractSeries *series = nullptr;

      switch(value)
      {
        case Representation::SPLINE:
          {
            QPen pen;
            pen.setWidth(2);
            pen.setColor(color);

            auto line = new QtCharts::QSplineSeries(forecastChart);
            line->setPointsVisible(true);
            line->setPen(pen);
            line->setName(name);
            line->setUseOpenGL(true);

            connect(line, SIGNAL(hovered(const QPointF &, bool)),
                    this, SLOT(onChartHover(const QPointF &, bool)));

            series = line;
          }
          break;
        case Representation::BARS:
          {
            auto barset = new QtCharts::QBarSet("");
            barset->setColor(color);
            barset->setLabel(name);

            auto line = new QtCharts::QBarSeries(forecastChart);
            line->setName(name);
            line->append(barset);
            line->setBarWidth(line->barWidth()*1.5);
            line->setUseOpenGL(true);
            line->setOpacity(0.8);

            connect(line, SIGNAL(hovered(bool, int, QBarSet *)),
                    this, SLOT(onChartHover(bool, int)));

            series = line;
          }
          break;
        default:
        case Representation::NONE:
          break;
      }

      return series;
    };

    QAbstractSeries *fakeLine = initSerie(Representation::SPLINE, Qt::black, "fake");
    QAbstractSeries *tempLine = initSerie(m_config->tempRepr, m_config->tempReprColor, tempStr);
    QAbstractSeries *rainLine = initSerie(m_config->rainRepr, m_config->rainReprColor, tr("Rain accumulation"));
    QAbstractSeries *snowLine = initSerie(m_config->snowRepr, m_config->snowReprColor, tr("Snow accumulation"));

    double rainMin = 100, rainMax = -100;
    double snowMin = 100, snowMax = -100;

    auto appendValue = [](const long long x, const double y, QAbstractSeries *ptr)
    {
      if(ptr)
      {
        switch(ptr->type())
        {
          case QAbstractSeries::SeriesTypeBar:
            qobject_cast<QBarSeries*>(ptr)->barSets().first()->append(y);
            break;
          case QAbstractSeries::SeriesTypeSpline:
            qobject_cast<QSplineSeries*>(ptr)->append(x, y);
            break;
          default:
            break;
        }
      }
    };

    for(int i = 0; i < data.size(); ++i)
    {
      const auto &entry = data.at(i);

      unixTimeStampToDate(t, entry.dt);
      dtTime = QDateTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};

      const auto msecs = dtTime.toMSecsSinceEpoch();

      appendValue(msecs, 0, fakeLine);

      auto value = tempFunc(entry.temp);
      appendValue(msecs, value, tempLine);

      value = precipitationFunc(entry.rain);
      appendValue(msecs, value, rainLine);

      rainMin = std::min(rainMin, value);
      rainMax = std::max(rainMax, value);

      value = precipitationFunc(entry.snow);
      appendValue(msecs, value, snowLine);

      snowMin = std::min(snowMin, value);
      snowMax = std::max(snowMax, value);
    }

    auto plotAreaGradient = sunriseSunsetGradient(QDateTime::fromMSecsSinceEpoch(data.first().dt * 1000), 
                                                  QDateTime::fromMSecsSinceEpoch(data.last().dt * 1000));
    forecastChart->setPlotAreaBackgroundBrush(plotAreaGradient);
    forecastChart->setPlotAreaBackgroundVisible(true);

    axisYTemp->setProperty("axisType", "temp");

    // Bar series need to be added first so they don't hide the line series.
    auto addLineSeriesToChart = [&forecastChart, &axisX](QAbstractSeries *ptr)
    {
      forecastChart->addSeries(ptr);
      forecastChart->setAxisX(axisX,ptr);
    };

    QList<QAbstractSeries*> toAddLater;
    for(QAbstractSeries *ptr: {fakeLine, tempLine, rainLine, snowLine})
    {
      if(!ptr) continue;

      if(qobject_cast<QSplineSeries*>(ptr))
      {
        toAddLater << ptr;
      }
      else
      {
        forecastChart->addSeries(ptr);
      }
    }

    std::for_each(toAddLater.cbegin(), toAddLater.cend(), addLineSeriesToChart);

    if(tempLine) forecastChart->setAxisY(axisYTemp, tempLine);
    if(rainLine) forecastChart->setAxisY(axisYPrec, rainLine);
    if(snowLine) forecastChart->setAxisY(axisYPrec, snowLine);

    fakeLine->setVisible(false);
    if(rainLine) rainLine->setVisible(rainMin != 0 || rainMax != 0);
    if(snowLine) snowLine->setVisible(snowMin != 0 || snowMax != 0);

    axisYPrec->setVisible((rainLine && rainLine->isVisible()) || (snowLine && snowLine->isVisible()));

    updateAxesRanges(forecastChart);

    for(auto marker: forecastChart->legend()->markers())
    {
      connect(marker, SIGNAL(clicked()), this, SLOT(onLegendMarkerClicked()));
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

      font = axisYPrec->titleFont();
      font.setPointSize(font.pointSize()*scale);
      axisYPrec->setTitleFont(font);

      font = axisYTemp->titleFont();
      font.setPointSize(font.pointSize()*scale);
      axisYTemp->setTitleFont(font);

      forecastChart->adjustSize();
    }

    auto oldChart = m_weatherChart->chart();
    m_weatherChart->setChart(forecastChart);
    m_weatherChart->setBackgroundBrush(m_config->lightTheme ? this->palette().base() : QColor("#232629"));

    connect(axisX, SIGNAL(rangeChanged(QDateTime, QDateTime)),
            this,  SLOT(onAreaChanged()));
    connect(axisX, SIGNAL(rangeChanged(QDateTime, QDateTime)),
            this,  SLOT(onForecastAreaChanged(QDateTime, QDateTime)));

    if(oldChart)
    {
      auto axis = qobject_cast<QDateTimeAxis *>(oldChart->axisX());
      if(axis)
      {
        disconnect(axis, SIGNAL(rangeChanged(QDateTime, QDateTime)),
                   this, SLOT(onAreaChanged()));
        disconnect(axisX, SIGNAL(rangeChanged(QDateTime, QDateTime)),
                this,  SLOT(onForecastAreaChanged(QDateTime, QDateTime)));

      }

      delete oldChart;
    }

    onResetButtonPressed();
  }

  if(mapsEnabled())
  {
    updateMapLayerValues();
    loadMaps();
  }
}

//--------------------------------------------------------------------
void WeatherDialog::onResetButtonPressed()
{
  bool applied = false;
  switch(m_tabWidget->currentIndex())
  {
    case 1:
      m_weatherChart->chart()->zoomReset();
      applied = true;
      break;
    case 2:
      m_pollutionChart->chart()->zoomReset();
      applied = true;
      break;
    case 3:
      m_uvChart->chart()->zoomReset();
      applied = true;
      break;
    default:
      break;
  }

  if(applied) m_reset->setEnabled(false);
}

//--------------------------------------------------------------------
void WeatherDialog::onTabChanged(int index)
{
  m_reset->setVisible(index >= 1 && index <= 3);
  onAreaChanged();
  m_config->lastTab = index;
}

//--------------------------------------------------------------------
void WeatherDialog::onChartHover(const QPointF& point, bool state)
{
  QtCharts::QLineSeries *currentLine = qobject_cast<QtCharts::QLineSeries *>(sender());
  if(!currentLine) return;

  QtCharts::QChartView *currentChart = nullptr;
  switch(m_tabWidget->currentIndex())
  {
    case 1: currentChart = m_weatherChart; break;
    case 2: currentChart = m_pollutionChart; break;
    case 3: currentChart = m_uvChart; break;
    default: break;
  }
  if(!currentChart) return;

  if(currentChart == m_weatherChart && m_weatherTooltip)
  {
    m_weatherTooltip->hide();
    m_weatherTooltip = nullptr;
  }
  else
  {
    if(currentChart == m_pollutionChart && m_pollutionTooltip)
    {
      m_pollutionTooltip->hide();
      m_pollutionTooltip = nullptr;
    }
    else
    {
      if(currentChart == m_uvChart && m_uvTooltip)
      {
        m_uvTooltip->hide();
        m_uvTooltip = nullptr;
      }
    }
  }

  if(state)
  {
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
        QWidget* tooltipWidget = nullptr;

        if(currentChart == m_weatherChart)
        {
          m_weatherTooltip = std::make_shared<WeatherWidget>(m_forecast->at(closestPoint), *m_config);
          tooltipWidget = m_weatherTooltip.get();
        }
        else if(currentChart == m_pollutionChart)
        {
          m_pollutionTooltip = std::make_shared<PollutionWidget>(m_pollution->at(closestPoint));
          tooltipWidget = m_pollutionTooltip.get();
        }
        else if(currentChart == m_uvChart)
        {
          m_uvTooltip = std::make_shared<UVWidget>(m_uv->at(closestPoint));
          tooltipWidget = m_uvTooltip.get();
        }

        if(tooltipWidget)
        {
          auto pos = currentChart->mapToGlobal(currentChart->chart()->mapToPosition(currentLine->at(closestPoint), currentLine).toPoint());
          pos = QPoint{pos.x()-tooltipWidget->width()/2, pos.y() - tooltipWidget->height() - 5};
          tooltipWidget->move(pos);
          tooltipWidget->show();
        }
      }
    }
  }
}

//--------------------------------------------------------------------
void WeatherDialog::onLoadFinished(bool value)
{
  if(mapsEnabled()) m_tabWidget->setTabText(4, tr("Maps"));

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
  return m_tabWidget->count() == 5;
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
    m_tabWidget->setTabText(4, QString("%1 (%2%)").arg(mapsStr).arg(progress, 2, 10, QChar('0')));
  }
}

//--------------------------------------------------------------------
void WeatherDialog::onAreaChanged()
{
  const auto currentIndex = m_tabWidget->currentIndex();

  switch(currentIndex)
  {
    case 1:
      m_reset->setEnabled(!m_weatherError->isVisible() && m_weatherChart->chart() && m_weatherChart->chart()->isZoomed());
      break;
    case 2:
      m_reset->setEnabled(!m_pollutionError->isVisible() && m_pollutionChart->chart() && m_pollutionChart->chart()->isZoomed());
      break;
    case 3:
      m_reset->setEnabled(!m_uvError->isVisible() && m_uvChart->chart() && m_uvChart->chart()->isZoomed());
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

  if(m_pollution->isEmpty())
  {
    m_pollutionChart->hide();
    m_pollutionError->show();
    m_tabWidget->setTabIcon(2, QIcon(":/TrayWeather/network_error.svg"));
  }
  else
  {
    m_pollutionChart->show();
    m_pollutionError->hide();
    m_tabWidget->setTabIcon(2, QIcon());

    auto axisX = new QDateTimeAxis();
    axisX->setLabelsAngle(45);
    axisX->setFormat("dd (hh)");
    axisX->setTitleText(tr("Day (Hour)"));

    auto axisY = new QValueAxis();
    axisY->setLabelFormat("%i");
    axisY->setTitleText(tr("Concentration in %1").arg(CONCENTRATION_UNITS));

    const auto theme = m_config->lightTheme ? QtCharts::QChart::ChartTheme::ChartThemeQt : QtCharts::QChart::ChartTheme::ChartThemeDark;

    auto forecastChart = new QChart();
    forecastChart->legend()->setVisible(true);
    forecastChart->legend()->setAlignment(Qt::AlignBottom);
    forecastChart->legend()->setToolTip(tr("Click to hide or show the forecast."));
    forecastChart->setAnimationDuration(400);
    forecastChart->setAnimationEasingCurve(QEasingCurve(QEasingCurve::InOutQuad));
    forecastChart->setAnimationOptions(QChart::AllAnimations);
    forecastChart->addAxis(axisX, Qt::AlignBottom);
    forecastChart->addAxis(axisY, Qt::AlignLeft);
    forecastChart->setBackgroundVisible(false);
    forecastChart->setTheme(theme);

    const auto linesColor = Qt::darkGray;

    axisX->setGridLineVisible(true);
    axisX->setGridLineColor(linesColor);
    axisX->setMinorGridLineVisible(true);
    axisX->setMinorGridLineColor(linesColor);
    axisY->setGridLineVisible(true);
    axisY->setGridLineColor(linesColor);
    axisY->setMinorGridLineVisible(true);
    axisY->setMinorGridLineColor(linesColor);

    auto titleFont = axisX->titleFont();
    titleFont.setHintingPreference(QFont::HintingPreference::PreferFullHinting);
    titleFont.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, 1);
    titleFont.setStyleStrategy(QFont::StyleStrategy::PreferQuality);
    titleFont.setBold(true);

    auto labelsFont = axisX->labelsFont();
    labelsFont.setHintingPreference(QFont::HintingPreference::PreferFullHinting);
    labelsFont.setStyleStrategy(QFont::StyleStrategy::PreferQuality);
    labelsFont.setBold(false);

    forecastChart->setFont(titleFont);
    axisX->setTitleFont(titleFont);
    axisX->setLabelsFont(labelsFont);
    axisY->setTitleFont(titleFont);
    axisY->setLabelsFont(labelsFont);

    QPen pens[8];

    QSplineSeries *pollutionLine[8];

    for(int i = 0; i < 8; ++i)
    {
      pens[i].setWidth(2);
      pens[i].setColor(CONCENTRATION_COLORS[i]);
      pollutionLine[i] = new QSplineSeries(forecastChart);
      pollutionLine[i]->setName(CONCENTRATION_NAMES.at(i));
      pollutionLine[i]->setUseOpenGL(true);
      pollutionLine[i]->setPointsVisible(true);
      pollutionLine[i]->setPen(pens[i]);
    }

    QLinearGradient plotAreaGradient;
    plotAreaGradient.setStart(QPointF(0, 0));
    plotAreaGradient.setFinalStop(QPointF(1, 0));
    plotAreaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);

    struct tm t;
    for(int i = 0; i < data.size(); ++i)
    {
      const auto &entry = data.at(i);

      unixTimeStampToDate(t, entry.dt);
      auto dtTime = QDateTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
      const auto msec = dtTime.toMSecsSinceEpoch();

      pollutionLine[0]->append(msec, entry.co);
      pollutionLine[1]->append(msec, entry.no);
      pollutionLine[2]->append(msec, entry.no2);
      pollutionLine[3]->append(msec, entry.o3);
      pollutionLine[4]->append(msec, entry.so2);
      pollutionLine[5]->append(msec, entry.pm2_5);
      pollutionLine[6]->append(msec, entry.pm10);
      pollutionLine[7]->append(msec, entry.nh3);

      plotAreaGradient.setColorAt(static_cast<double>(i)/(data.size()-1), pollutionColor(entry.aqi));
    }

    forecastChart->setPlotAreaBackgroundBrush(plotAreaGradient);
    forecastChart->setPlotAreaBackgroundVisible(true);

    for(int i = 0; i < 8; ++i)
    {
      forecastChart->addSeries(pollutionLine[i]);
      forecastChart->setAxisX(axisX, pollutionLine[i]);
      forecastChart->setAxisY(axisY, pollutionLine[i]);

      connect(pollutionLine[i], SIGNAL(hovered(const QPointF &, bool)),
              this,             SLOT(onChartHover(const QPointF &, bool)));
    }

    updateAxesRanges(forecastChart);

    for(auto marker: forecastChart->legend()->markers())
    {
      connect(marker, SIGNAL(clicked()), this, SLOT(onLegendMarkerClicked()));
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

      font = axisY->titleFont();
      font.setPointSize(font.pointSize()*scale);
      axisY->setTitleFont(font);

      forecastChart->adjustSize();
    }

    auto oldChart = m_pollutionChart->chart();
    m_pollutionChart->setChart(forecastChart);
    m_pollutionChart->setBackgroundBrush(m_config->lightTheme ? this->palette().base() : QColor("#232629"));

    connect(axisX, SIGNAL(rangeChanged(QDateTime, QDateTime)),
            this,  SLOT(onAreaChanged()));
    connect(axisX, SIGNAL(rangeChanged(QDateTime, QDateTime)),
            this,  SLOT(onPollutionAreaChanged(QDateTime, QDateTime)));

    if(oldChart)
    {
      auto axis = qobject_cast<QDateTimeAxis *>(oldChart->axisX());
      if(axis)
      {
        disconnect(axis, SIGNAL(rangeChanged(QDateTime, QDateTime)),
                   this, SLOT(onAreaChanged()));
        disconnect(axis, SIGNAL(rangeChanged(QDateTime, QDateTime)),
                   this, SLOT(onPollutionAreaChanged(QDateTime, QDateTime)));
      }

      delete oldChart;
    }

    onResetButtonPressed();
  }
}

//--------------------------------------------------------------------
void WeatherDialog::setUVData(const UV &data)
{
  m_uv = &data;

  QString indexStr = tr("Unknown");
  if(!data.isEmpty())
  {
    const auto index = static_cast<int>(std::nearbyint(data.first().idx));
    switch(index)
    {
      case 0:
      case 1:
      case 2:
        indexStr = tr("Low");
        break;
      case 3:
      case 4:
      case 5:
        indexStr = tr("Moderate");
        break;
      case 6:
      case 7:
        indexStr = tr("High");
        break;
      case 8:
      case 9:
      case 10:
        indexStr = tr("Very high");
        break;
      default:
        indexStr = tr("Extreme");
        break;
    }
  }
  m_uvi->setText(indexStr);

  if(m_uv->isEmpty())
  {
    m_uvChart->hide();
    m_uvError->show();
    m_tabWidget->setTabIcon(3, QIcon(":/TrayWeather/network_error.svg"));
  }
  else
  {
    m_uvChart->show();
    m_uvError->hide();
    m_tabWidget->setTabIcon(3, QIcon());

    auto axisX = new QDateTimeAxis();
    axisX->setLabelsAngle(45);
    axisX->setFormat("dd (hh)");
    axisX->setTitleText(tr("Day (Hour)"));

    auto axisY = new QValueAxis();
    axisY->setLabelFormat("%i");
    axisY->setTitleText(tr("Ultraviolet radiation index"));

    const auto theme = m_config->lightTheme ? QtCharts::QChart::ChartTheme::ChartThemeQt : QtCharts::QChart::ChartTheme::ChartThemeDark;

    auto uvChart = new QChart();
    uvChart->legend()->setVisible(true);
    uvChart->legend()->setAlignment(Qt::AlignBottom);
    uvChart->legend()->setToolTip(tr("Click to hide or show the forecast."));
    uvChart->setAnimationDuration(400);
    uvChart->setAnimationEasingCurve(QEasingCurve(QEasingCurve::InOutQuad));
    uvChart->setAnimationOptions(QChart::AllAnimations);
    uvChart->addAxis(axisX, Qt::AlignBottom);
    uvChart->addAxis(axisY, Qt::AlignLeft);
    uvChart->setBackgroundVisible(false);
    uvChart->setTheme(theme);

    const auto linesColor = m_config->lightTheme ? Qt::darkGray : Qt::lightGray;

    axisX->setGridLineVisible(true);
    axisX->setGridLineColor(linesColor);
    axisX->setMinorGridLineVisible(true);
    axisX->setMinorGridLineColor(linesColor);
    axisY->setGridLineVisible(true);
    axisY->setGridLineColor(linesColor);
    axisY->setMinorGridLineVisible(true);
    axisY->setMinorGridLineColor(linesColor);

    auto titleFont = axisX->titleFont();
    titleFont.setHintingPreference(QFont::HintingPreference::PreferFullHinting);
    titleFont.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, 1);
    titleFont.setStyleStrategy(QFont::StyleStrategy::PreferQuality);
    titleFont.setBold(true);

    auto labelsFont = axisX->labelsFont();
    labelsFont.setHintingPreference(QFont::HintingPreference::PreferFullHinting);
    labelsFont.setStyleStrategy(QFont::StyleStrategy::PreferQuality);
    labelsFont.setBold(false);

    uvChart->setFont(titleFont);
    axisX->setTitleFont(titleFont);
    axisX->setLabelsFont(labelsFont);
    axisY->setTitleFont(titleFont);
    axisY->setLabelsFont(labelsFont);

    QPen pen;
    pen.setWidth(2);
    pen.setColor(QColor{90,90,235});

    auto uvLine = new QSplineSeries(uvChart);
    uvLine->setName(tr("UV Index"));
    uvLine->setUseOpenGL(true);
    uvLine->setPointsVisible(true);
    uvLine->setPen(pen);

    QLinearGradient plotAreaGradient;
    plotAreaGradient.setStart(QPointF(0, 0));
    plotAreaGradient.setFinalStop(QPointF(1, 0));
    plotAreaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);

    struct tm t;
    for(int i = 0; i < data.size(); ++i)
    {
      const auto &entry = data.at(i);

      unixTimeStampToDate(t, entry.dt);
      auto dtTime = QDateTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
      const auto msec = dtTime.toMSecsSinceEpoch();

      uvLine->append(msec, entry.idx);

      auto color = uvColor(entry.idx);
      if(static_cast<int>(std::nearbyint(entry.idx)) == 0) color = m_config->lightTheme ? this->palette().base().color() : QColor("#232629");
      color.setAlpha(210);
      plotAreaGradient.setColorAt(static_cast<double>(i)/(data.size()-1), color);
    }

    uvChart->setPlotAreaBackgroundBrush(plotAreaGradient);
    uvChart->setPlotAreaBackgroundVisible(true);

    uvChart->addSeries(uvLine);
    uvChart->setAxisX(axisX, uvLine);
    uvChart->setAxisY(axisY, uvLine);

    connect(uvLine, SIGNAL(hovered(const QPointF &, bool)),
            this,   SLOT(onChartHover(const QPointF &, bool)));

    updateAxesRanges(uvChart);

    const auto scale = dpiScale();
    if(scale != 1.)
    {
      auto font = uvChart->legend()->font();
      font.setPointSize(font.pointSize()*scale);
      uvChart->legend()->setFont(font);

      font = axisX->titleFont();
      font.setPointSize(font.pointSize()*scale);
      axisX->setTitleFont(font);

      font = axisY->titleFont();
      font.setPointSize(font.pointSize()*scale);
      axisY->setTitleFont(font);

      uvChart->adjustSize();
    }

    auto oldChart = m_uvChart->chart();
    m_uvChart->setChart(uvChart);
    m_uvChart->setBackgroundBrush(m_config->lightTheme ? this->palette().base() : QColor("#232629"));

    connect(axisX, SIGNAL(rangeChanged(QDateTime, QDateTime)),
            this,  SLOT(onAreaChanged()));
    connect(axisX, SIGNAL(rangeChanged(QDateTime, QDateTime)),
            this,  SLOT(onUVAreaChanged(QDateTime, QDateTime)));

    if(oldChart)
    {
      auto axis = qobject_cast<QDateTimeAxis *>(oldChart->axisX());
      if(axis)
      {
        disconnect(axis, SIGNAL(rangeChanged(QDateTime, QDateTime)),
                   this, SLOT(onAreaChanged()));
        disconnect(axis, SIGNAL(rangeChanged(QDateTime, QDateTime)),
                   this, SLOT(onUVAreaChanged(QDateTime, QDateTime)));
      }

      delete oldChart;
    }

    onResetButtonPressed();
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
void WeatherDialog::onPollutionAreaChanged(QDateTime begin, QDateTime end)
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
void WeatherDialog::onUVAreaChanged(QDateTime begin, QDateTime end)
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
  for(int i = 0; i < m_uv->size(); ++i)
  {
    const auto &entry = m_uv->at(i);

    unixTimeStampToDate(t, entry.dt);
    const auto dtTime = QDateTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
    const auto msec = dtTime.toMSecsSinceEpoch();

    auto color = uvColor(entry.idx);
    if(static_cast<int>(std::nearbyint(entry.idx)) == 0) color = m_config->lightTheme ? this->palette().base().color() : QColor("#232629");
    color.setAlpha(210);

    if(msec < begin.toMSecsSinceEpoch())
    {
      plotAreaGradient.setColorAt(0., color);
      continue;
    }

    if(msec > end.toMSecsSinceEpoch())
    {
      plotAreaGradient.setColorAt(1., color);
      break;
    }

    plotAreaGradient.setColorAt(interpolateDt(msec), color);
  }

  auto chart = m_uvChart->chart();
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
    	QString result = "mapnik"; // default OpenStreetMaps;

    	if(value.toString().compare("Google Maps", Qt::CaseInsensitive) == 0)
    		result = "googlemap";
    	else if(value.toString().compare("Google Satellite", Qt::CaseInsensitive) == 0)
    		result = "googlesat";

      m_config->lastStreetLayer = result;
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

    const QStringList tabNames{ tr("Current Weather"), tr("Forecast"), tr("Pollution"), tr("UV"), tr("Maps") };
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
  if(!m_webpage) return;

  m_mapsButton->setText(tr("Hide Maps"));
  m_mapsButton->setToolTip(tr("Hide weather maps tab."));

  QFile webfile(":/TrayWeather/webpage.html");
  if(webfile.open(QFile::ReadOnly))
  {
    QString webpage{webfile.readAll()};

    auto nullF = [](double d){return d;};
    QString isMetric, isImperial, degrees, windUnit, rainUnit, rainGrades, windGrades, tempGrades;
    switch(m_config->units)
    {
      case Units::IMPERIAL:
        isMetric   = "false";
        isImperial = "true";
        degrees    = "ºF";
        windUnit   = tr("miles/h");
        rainUnit   = tr("inches");
        rainGrades = generateMapGrades(RAIN_MAP_LAYER_GRADES_MM, convertMmToInches);
        windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, convertMetersSecondToMilesHour);
        tempGrades = generateMapGrades(TEMP_MAP_LAYER_GRADES_CELSIUS, convertCelsiusToFahrenheit);
        break;
      case Units::METRIC:
        isMetric   = "true";
        isImperial = "false";
        degrees    = "ºC";
        windUnit   = tr("met/sec");
        rainUnit   = tr("mm");
        rainGrades = generateMapGrades(RAIN_MAP_LAYER_GRADES_MM, nullF);
        windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, nullF);
        tempGrades = generateMapGrades(TEMP_MAP_LAYER_GRADES_CELSIUS, nullF);
        break;
      case Units::CUSTOM:
        isMetric   = (m_config->windUnits == WindUnits::METSEC  || m_config->windUnits == WindUnits::KMHR) ? "true":"false";
        isImperial = (m_config->windUnits == WindUnits::FEETSEC || m_config->windUnits == WindUnits::MILHR) ? "true":"false";
        degrees    = m_config->tempUnits == TemperatureUnits::CELSIUS ? "ºC" : "ºF";
        switch(m_config->precUnits)
        {
          case PrecipitationUnits::INCH:
            rainUnit   = tr("inches");
            rainGrades = generateMapGrades(RAIN_MAP_LAYER_GRADES_MM, convertMmToInches);
            break;
          default:
          case PrecipitationUnits::MM:
            rainUnit   = tr("mm");
            rainGrades = generateMapGrades(RAIN_MAP_LAYER_GRADES_MM, nullF);
            break;
        }
        switch(m_config->windUnits)
        {
          case WindUnits::FEETSEC:
            windUnit = tr("feet/sec");
            windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, convertMetersSecondToFeetSecond);
            break;
          case WindUnits::KMHR:
            windUnit = tr("km/h");
            windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, convertMetersSecondToKilometersHour);
            break;
          case WindUnits::MILHR:
            windUnit = tr("miles/h");
            windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, convertMetersSecondToMilesHour);
            break;
          case WindUnits::METSEC:
            windUnit = tr("met/sec");
            windGrades = generateMapGrades(WIND_MAP_LAYER_GRADES_METSEC, nullF);
            break;
        }
        switch(m_config->tempUnits)
        {
          case TemperatureUnits::FAHRENHEIT:
            tempGrades = generateMapGrades(TEMP_MAP_LAYER_GRADES_CELSIUS, convertCelsiusToFahrenheit);
            break;
          default:
          case TemperatureUnits::CELSIUS:
            tempGrades = generateMapGrades(TEMP_MAP_LAYER_GRADES_CELSIUS, nullF);
            break;
        }
        break;
      default:
        return;
        break;
    }

    webpage.replace("%%metric%%", isMetric);
    webpage.replace("%%imperial%%", isImperial);
    webpage.replace("%%degrees%%", degrees);
    webpage.replace("%%windUnit%%", windUnit);
    webpage.replace("%%rainUnit%%", rainUnit);
    webpage.replace("%%rainGrades%%", rainGrades);
    webpage.replace("%%windGrades%%", windGrades);
    webpage.replace("%%tempGrades%%", tempGrades);

    webpage.replace("%%tempStr%%", tr("Temperature"), Qt::CaseSensitive);
    webpage.replace("%%rainStr%%", tr("Rain"), Qt::CaseSensitive);
    webpage.replace("%%windStr%%", tr("Wind"), Qt::CaseSensitive);
    webpage.replace("%%cloudStr%%", tr("Clouds"), Qt::CaseSensitive);

    // config
    webpage.replace("%%lat%%", QString::number(m_config->latitude), Qt::CaseSensitive);
    webpage.replace("%%lon%%", QString::number(m_config->longitude), Qt::CaseSensitive);
    webpage.replace("{api_key}", m_config->owm_apikey, Qt::CaseSensitive);
    webpage.replace("%%layermap%%", m_config->lastLayer, Qt::CaseSensitive);
    webpage.replace("%%streetmap%%", m_config->lastStreetLayer, Qt::CaseSensitive);

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
void WeatherDialog::onChartHover(bool state, int index)
{
  auto series = qobject_cast<QBarSeries*>(sender());
  if(!series) return;

  if(m_weatherChart && m_weatherTooltip)
  {
    m_weatherTooltip->hide();
    m_weatherTooltip = nullptr;
  }

  if(state)
  {
    auto bars = series->barSets().first();
    const auto position = m_weatherChart->chart()->mapToPosition(QPointF{static_cast<double>(index), bars->at(index)}, series);

    m_weatherTooltip = std::make_shared<WeatherWidget>(m_forecast->at(index), *m_config);
    auto pos = m_weatherChart->mapToGlobal(position.toPoint());
    pos = QPoint{pos.x()-m_weatherTooltip->width()/2, pos.y() - m_weatherTooltip->height() - 5};
    m_weatherTooltip->move(pos);
    m_weatherTooltip->show();
  }
}

//--------------------------------------------------------------------
void WeatherDialog::onLegendMarkerClicked()
{
  auto marker = qobject_cast<QLegendMarker *>(sender());
  if(marker)
  {
    auto chart = marker->series()->chart();

    if(marker->series()->isVisible())
    {
      auto isVisible = [](const QAbstractSeries *s){ return s->isVisible(); };
      const auto series = chart->series();
      const auto visibleCount = std::count_if(series.cbegin(), series.cend(), isVisible);

      if(visibleCount == 1) return; // at least one should be visible.

      marker->series()->setVisible(false);

      auto brush = marker->brush();
      auto color = brush.color();
      color.setAlphaF(0.5);
      brush.setColor(color);
      marker->setBrush(brush);
      brush = marker->labelBrush();
      color = brush.color();
      color.setAlphaF(0.5);
      brush.setColor(color);
      marker->setLabelBrush(brush);
      marker->setVisible(true);
    }
    else
    {
      marker->series()->setVisible(true);

      auto brush = marker->brush();
      auto color = brush.color();
      color.setAlphaF(1);
      brush.setColor(color);
      marker->setBrush(brush);
      brush = marker->labelBrush();
      color = brush.color();
      color.setAlphaF(1);
      brush.setColor(color);
      marker->setLabelBrush(brush);
      marker->setVisible(true);
    }

    updateAxesRanges(chart);
  }
}

//--------------------------------------------------------------------
void WeatherDialog::removeMaps()
{
  m_mapsButton->setText(tr("Show Maps"));
  m_mapsButton->setToolTip(tr("Show weather maps tab."));

  m_tabWidget->removeTab(4);

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

//--------------------------------------------------------------------
void WeatherDialog::updateAxesRanges(QtCharts::QChart *chart)
{
  auto series = chart->series();
  auto axes = chart->axes(Qt::Vertical);

  for(auto axis: axes)
  {
    int timesUsed = 0;
    double minValue = 1000;
    double maxValue = -1000;

    for(auto serie: series)
    {
      if(serie->isVisible() && serie->attachedAxes().contains(axis))
      {
        ++timesUsed;

        auto lineSerie = qobject_cast<QLineSeries *>(serie);
        if(lineSerie)
        {
          for(int i = 0; i < lineSerie->count(); ++i)
          {
            const auto value = lineSerie->points().at(i).y();
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
          }
        }
        else
        {
          auto barSerie = qobject_cast<QBarSeries *>(serie);


          for(int i = 0; i < barSerie->barSets().first()->count(); ++i)
          {
            const auto value = barSerie->barSets().first()->at(i);
            minValue = std::min(minValue, value);
            maxValue = std::max(maxValue, value);
          }
        }
      }
    }

    if(timesUsed > 0)
    {
      maxValue *= 1.1;
      if(axis->property("axisType").toString().compare("temp", Qt::CaseInsensitive) == 0)
      {
        minValue -= 1;
      }

      axis->setRange(minValue, maxValue);
    }

    axis->setVisible(timesUsed > 0);
  }
}

//--------------------------------------------------------------------
QLinearGradient WeatherDialog::sunriseSunsetGradient(QDateTime begin, QDateTime end)
{
  const auto startT = begin.toMSecsSinceEpoch();
  const auto endT = end.toMSecsSinceEpoch();

  QLinearGradient plotAreaGradient;
  plotAreaGradient.setStart(QPointF(0, 0));
  plotAreaGradient.setFinalStop(QPointF(1, 0));
  plotAreaGradient.setCoordinateMode(QGradient::ObjectBoundingMode);

  QGradientStops stops;

  auto interpolateDt = [&startT, &endT](const long long int dt)
  {
    return static_cast<double>(dt-startT)/(endT-startT);
  };

  auto setColorAt = [&stops, startT, endT, &interpolateDt](const long long entry, const QColor &color)
  {
    if(entry > endT || entry < startT) return;
    stops << QGradientStop(interpolateDt(entry), color);
  };

  std::set<unsigned long long> sunrises, sunsets;
  constexpr unsigned long long minuteMs = 15 * 60 * 1000;

  for(int i = 0; i < m_forecast->size(); ++i)
  {
    const auto &entry = m_forecast->at(i);
    const auto [sunrise, sunset] = computeSunriseSunset(entry, m_config->longitude, m_config->latitude);

    sunrises.emplace(sunrise * 1000);
    sunsets.emplace(sunset * 1000);
  }

  for(const unsigned long long sunrise: sunrises)
  {
    setColorAt((sunrise-minuteMs), Qt::lightGray);
    setColorAt(sunrise, Qt::white);
  }

  for(const unsigned long long sunset: sunsets)
  {
    setColorAt(sunset-minuteMs, Qt::white);
    setColorAt(sunset, Qt::lightGray);
  }

  auto sortStops = [](const QGradientStop &a, const QGradientStop &b){ return a.first < b.first; };
  std::sort(stops.begin(), stops.end(), sortStops);
  for(const auto &s: stops) plotAreaGradient.setColorAt(s.first, s.second);

  return plotAreaGradient;
}

//--------------------------------------------------------------------
void WeatherDialog::onForecastAreaChanged(QDateTime begin, QDateTime end)
{
  auto plotAreaGradient = sunriseSunsetGradient(begin, end);

  auto chart = m_weatherChart->chart();
  chart->setPlotAreaBackgroundBrush(plotAreaGradient);
  chart->setPlotAreaBackgroundVisible(true);
  chart->update();
}
