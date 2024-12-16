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
class WeatherProvider;
struct ProviderCapabilities;

// Qt
#include "ui_WeatherDialog.h"
#include "ui_ErrorWidget.h"
#include <QDialog>
#include <qwebview.h>
#include <QWidget>

// C++
#include <memory>

namespace QtCharts
{
  class QChart;
  class QChartView;
  class QLineSeries;
}

class QWebView;
class WeatherWidget;
class PollutionWidget;
class UVWidget;

/** \class ErrorWidget
 * \brief Widget to show an error message in a forecast tab.
 *
 */
class ErrorWidget
: public QWidget
, private Ui::ErrorWidget
{
    Q_OBJECT
  public:
    explicit ErrorWidget(const QString &text)
    { setupUi(this); m_text->setText(text); }

    virtual ~ErrorWidget()
    {};
};

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
     * \param[in] provider Weather data provider. 
     * \param[in] parent pointer of the widget parent of this one.
     * \param[in] flags window flags.
     *
     */
    WeatherDialog(std::shared_ptr<WeatherProvider> provider, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    /** \brief WeatherDialog class virtual destructor.
     *
     */
    virtual ~WeatherDialog();

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

    /** \brief Sets the UV forecast data.
     * \param[in] data UV forecast data.
     *
     */
    void setUVData(const UV &data);

    /** \brief Returns true if the maps tab is visible and false otherwise.
     *
     */
    bool mapsEnabled() const;

    /** \brief Updates the weather provider object. 
     * \param[in] provider Weather data provider. 
     * 
     */
    void setWeatherProvider(std::shared_ptr<WeatherProvider> provider);

  signals:
    void mapsEnabled(bool);

  protected:
    virtual void showEvent(QShowEvent *e) override;
    virtual void changeEvent(QEvent *e) override;

  private slots:
    /** \brief Shows weather data when the user hovers on spline series.
     * \param[in] point hover point.
     * \param[in] state true when user has hovered over the series and false when hover has moved away from the series.
     *
     */
    void onChartHover(const QPointF &point, bool state);

    /** \brief Shows weather data when the user hovers on a bar series.
     * \param[in] state true when user has hovered over the series and false when hover has moved away from the series.
     * \param[in] index Barset value index.
     *
     */
    void onChartHover(bool state, int index);

    /** \brief Hides/show the series when the legend marker is clicked.
     *
     */
    void onLegendMarkerClicked();

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

    /** \brief Updates the background of the pollution chart on zoom.
     * \param[in] begin Begin point in X axis.
     * \param[in] end End point in X axis.
     *
     */
    void onPollutionAreaChanged(QDateTime begin, QDateTime end);

    /** \brief Updates the background of the forecast chart on zoom.
     * \param[in] begin Begin point in X axis.
     * \param[in] end End point in X axis.
     *
     */
    void onForecastAreaChanged(QDateTime begin, QDateTime end);

    /** \brief Updates the background of the uv chart on zoom.
     * \param[in] begin Begin point in X axis.
     * \param[in] end End point in X axis.
     *
     */
    void onUVAreaChanged(QDateTime begin, QDateTime end);

  private:
    /** \brief Returns the color of the given aqi value.
     * \param[in] aqiValue aqi value in [1,5].
     *
     */
    QColor pollutionColor(const int aqiValue);

    /** \brief Updates the values of map layers in the configuration.
     *
     */
    void updateMapLayerValues();

    /** \brief Loads and translated the web page of the maps.
     *
     */
    void loadMaps();

    /** \brief Removes the maps tab.
     *
     */
    void removeMaps();

    /** \brief Updates the visible series ranges.
     * \param[in] chart QChart object to update axes ranges.
     *
     */
    void updateAxesRanges(QtCharts::QChart *chart);

    /** \brief Computes and returns the gradient for the given dates.
     * \param[in] begin Begin point in X axis.
     * \param[in] end End point in X axis.
     * 
     */
    QLinearGradient sunriseSunsetGradient(QDateTime begin, QDateTime end);

    /** \brief Updates the UI depeding on the capabilities of the current weather provider.
     * \param[in] capabilities Weather provider capabilities.
     *
     */
    void updateUI(const ProviderCapabilities &capabilities);

    std::shared_ptr<WeatherProvider> m_provider;         /** Weather data provider.          */
    ErrorWidget                     *m_weatherError;     /** Weather forecast error widget.  */
    ErrorWidget                     *m_pollutionError;   /** Pollution forecast error widget */
    ErrorWidget                     *m_uvError;          /** UV forecast error widget        */
    QtCharts::QChartView            *m_weatherChart;     /** weather forecast chart view.    */
    QtCharts::QChartView            *m_pollutionChart;   /** pollution forecast chart view.  */
    QtCharts::QChartView            *m_uvChart;          /** UV forecast chart view.         */    
    const Forecast                  *m_forecast;         /** forecast data.                  */
    const Pollution                 *m_pollution;        /** pollution data.                 */
    const UV                        *m_uv;               /** UV data.                        */
    Configuration                   *m_config;           /** configuration data for tooltip. */
    std::shared_ptr<WeatherWidget>   m_weatherTooltip;   /** weather char tooltip widget.    */
    std::shared_ptr<PollutionWidget> m_pollutionTooltip; /** pollution chart tooltip widget. */
    std::shared_ptr<UVWidget>        m_uvTooltip;        /** UV chart tooltip widget.        */
    QWebView                        *m_webpage;          /** maps webpage.                   */
};

#endif // WEATHERDIALOG_H_
