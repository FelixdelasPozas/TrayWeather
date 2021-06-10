/*
 File: WeatherDialog.h
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

#ifndef WEATHERDIALOG_H_
#define WEATHERDIALOG_H_

// Project
#include <Utils.h>

// Qt
#include "ui_WeatherDialog.h"
#include <QDialog>
#include <qwebview.h>
#include <QWidget>

// C++
#include <memory>

namespace QtCharts
{
  class QChartView;
  class QLineSeries;
}

class QWebView;
class WeatherWidget;
class PollutionWidget;

/** \class WeatherDialog
 * \brief Implements the dialog showing the current weather and the forecast.
 *
 */
class WeatherDialog
: public QDialog
, public Ui_WeatherDialog
{
    Q_OBJECT
  public:
    /** \brief WeatherDialog class constructor.
     * \param[in] parent pointer of the widget parent of this one.
     * \param[in] flags window flags.
     *
     */
    WeatherDialog(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    /** \brief WeatherDialog class virtual destructor.
     *
     */
    virtual ~WeatherDialog()
    {};

    /** \brief Sets the weather and forecast data.
     * \param[in] current current weather data.
     * \param[in] data forecast data.
     * \param[in] config application configuration.
     *
     */
    void setWeatherData(const ForecastData &current, const Forecast &data, Configuration &config);

    /** \brief Sets the pollution forecast data.
     * \param[in] data pollution forecast data.
     *
     */
    void setPollutionData(const Pollution &data);

    /** \brief Returns true if the maps tab is visible and false otherwise.
     *
     */
    bool mapsEnabled() const;

  signals:
    void mapsEnabled(bool);

  protected:
    virtual void showEvent(QShowEvent *e);

  private slots:
    /** \brief Shows weather data when the user hovers on the temperature line.
     * \param[in] point hover point.
     * \param[in] state true when user has hovered over the series and false when hover has moved away from the series.
     *
     */
    void onChartHover(const QPointF &point, bool state);

    /** \brief Resets the chart's zoom to the original one.
     *
     */
    void onResetButtonPressed();

    /** \brief Updates the GUI when the user changes the tab.
     * \param[in] index index of the current tab.
     *
     */
    void onTabChanged(int index);

    /** \brief Shows the maps tab once it has finished loading.
     * \param[in] value true on load success and false otherwise.
     *
     */
    void onLoadFinished(bool value);

    /** \brief Shows/hides the maps tab.
     *
     */
    void onMapsButtonPressed();

    /** \brief Helper method to update the maps tab title on load progress.
     * \param[in] progress progress value in [0-100].
     *
     */
    void onLoadProgress(int progress);

    /** \brief Updates the state of the reset chart zoom button.
     *
     */
    void onAreaChanged();

    /** \brief Updates the state of the reset chart zoom button.
     *
     */
    void onAreaChanged(QDateTime begin, QDateTime end);

  private:
    /** \brief Returns the color of the given aqi value.
     * \param[in] aqiValue aqi value in [1,5].
     *
     */
    QColor pollutionColor(const int aqiValue);


    QtCharts::QChartView            *m_weatherChart;     /** weather forecast chart view.          */
    QtCharts::QChartView            *m_pollutionChart;   /** pollution forecast chart view.        */
    QtCharts::QLineSeries           *m_temperatureLine;  /** temperature series line.              */
    QtCharts::QLineSeries           *m_pollutionLine[8]; /** pollution concentrations series line. */
    const Forecast                  *m_forecast;         /** forecast data for tooltip.            */
    const Pollution                 *m_pollution;        /** pollution data for tooltip.           */
    Configuration                   *m_config;           /** configuration data for tooltip.       */
    std::shared_ptr<WeatherWidget>   m_weatherTooltip;   /** weather char tooltip widget.          */
    std::shared_ptr<PollutionWidget> m_pollutionTooltip; /** pollution chart tooltip widget.       */
    QWebView                        *m_webpage;          /** maps webpage.                         */
};

#endif // WEATHERDIALOG_H_
