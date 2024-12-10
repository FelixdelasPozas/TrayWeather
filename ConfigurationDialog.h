/*
 File: ConfigurationDialog.h
 Created on: 13/11/2016
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

#ifndef CONFIGURATIONDIALOG_H_
#define CONFIGURATIONDIALOG_H_

// Project
#include <Utils.h>
class WeatherProvider;

// Qt
#include "ui_ConfigurationDialog.h"
#include <QDialog>
#include <QNetworkAccessManager>
class QNetworkReply;
class QMouseEvent;

// C++
#include <memory>

/** \class PreviewWidget
 * \brief Implements a widget to show a pixmap and close when the mouse leaves it.
 *
 */
class PreviewWidget
: public QWidget
{
    Q_OBJECT
  public:
    /** \brief PreviewWidget class constructor.
     * \param[in] image Image to show.
     * \param[in] parent Raw pointer of the widget parent of this one.
     *
     *
     */
    explicit PreviewWidget(QPixmap image, QWidget* parent = nullptr);

    /** \brief PreviewWidget class virtual destructor.
     *
     */
    virtual ~PreviewWidget()
    {};

  protected:
    virtual void leaveEvent(QEvent *e) override;
};

/** \class ConfigurationDialog
 * \brief Implements the dialog to configure the settings.
 *
 */
class ConfigurationDialog
: public QDialog
, private Ui_ConfigurationDialog
{
    Q_OBJECT
  public:
    /** \brief LocationConfigDialog class constructor.
     * \param[in] configuration application configuration.
     * \param[in] provider Weather provider.
     * \param[in] parent pointer to the widget parent of this one.
     * \param[in] flags window flags.
     */
    ConfigurationDialog(const Configuration &configuration, std::shared_ptr<WeatherProvider> provider, 
                        QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    /** \brief LocationConfigDialog class virtual destructor.
     *
     */
    virtual ~ConfigurationDialog()
    {};

    /** \brief Returns the configuration values.
     * \param[out] configuration application configuration values.
     *
     */
    void getConfiguration(Configuration &configuration) const;

    /** \brief Sets the temperature value to use for icon preview.
     * \param[in] value Temperature value.
     *
     */
    void setCurrentTemperature(const int value);

  protected:
    virtual void showEvent(QShowEvent *e) override;
    virtual void changeEvent(QEvent *e) override;

  private slots:
    /** \brief Manages replies from the network requests.
     * \param[in] reply network reply object pointer.
     */
    void replyFinished(QNetworkReply *reply);

    /** \brief Request IP Geolocation.
     *
     */
    void requestGeolocation();

    /** \brief Request forecast data to test OpenWeatherMap API key validity.
     *
     */
    void requestOpenWeatherMapAPIKeyTest() const;

    /** \brief Disables/Enables geolocation from DNS IP instead of geolocation detected ip.
     * \param[in] state DNS Checkbox state.
     *
     */
    void onDNSRequestStateChanged(int state);

    /** \brief Helper method that enables/disables part of the UI depending on the state of the UI radio buttons.
     *
     */
    void onRadioChanged();

    /** \brief Helper method that updates the coordinates labels when one changes.
     *
     */
    void onCoordinatesChanged();

    /** \brief Updates the visual theme of the application when the user changes the theme value.
     * \param[in] index Theme combo box index.
     *
     */
    void onThemeIndexChanged(int index);

    /** \brief Opens a color dialog to let the user choose the color for the tray text.
     *
     */
    void onColorButtonClicked();

    /** \brief Helper that checks the values of temperatures spinboxes.
     * \param[in] value New value for modified spinbox.
     *
     */
    void onTemperatureValueChanged(int value);

    /** \brief Helper that creates/removes the autostart registry entry depending on
     * the value of the parameter.
     * \param[in] value True to create and false to remove if it exists.
     *
     */
    void onAutostartValueChanged(int value);

    /** \brief Updates the application translation when the user changes it.
     * \param[in] index Current index of language combo box.
     *
     */
    void onLanguageChanged(int index);

    /** \brief Updates the UI when the user changes the measurement units.
     * \param[in] index Current index of units combo.
     *
     */
    void onUnitsValueChanged(int index);

    /** \brief Updates the "Selected" property in the combo.
     * \param[in] index Current index of the combo.
     *
     */
    void onUnitComboChanged(int index);

    /** \brief Adds the selected tooltip field in the combo to the shown list.
     *
     */
    void onTooltipTextAdded();

    /** \brief Removes the selected tooltip field from the shown list to the combo.
     *
     */
    void onTooltipTextDeleted();

    /** \brief Moves the selected tooltip field up/down in the shown list.
     *
     */
    void onTooltipTextMoved();

    /** \brief Updates the UI when the user moves the tooltip fields.
     * \param[in] row Current selected row.
     */
    void onTooltipFieldsRowChanged(int row);

    /** \brief Show the icon theme summary.
     *
     */
    void onIconPreviewPressed();

    /** \brief Show the temperature icon preview.
     *
     */
    void onFontPreviewPressed();

    /** \brief Displays the font selector dialog and gets the result.
     *
     */
    void onFontSelectorPressed();

    /** \brief Updates the UI when the user changes the icon theme.
     * \param[in] idx Current index of icon theme combobox.
     *
     */
    void onIconThemeIndexChanged(int idx);

    /** \brief Updates the temperature icon according to current settings.
     *
     */
    void updateTemperatureIcon();

    /** \brief Updates UI when the icon type changes.
     *
     */
    void onIconTypeChanged(int);

    /** \brief Shows the location finder dialog and gets the results. 
     * 
     */
    void onSearchButtonClicked();

  signals:
    void languageChanged(const QString &);

  private:
    /** \brief Helper method to fill the configuration values and initilize the UI.
     * \param[in] configuration Configuration struct reference.
     *
     */
    void setConfiguration(const Configuration &configuration);

    /** \brief Request DNS IP
     *
     */
    void requestDNSIPGeolocation();

    /** \brief Helper method to connect UI signals to the slots.
     *
     */
    void connectSignals();

    /** \brief Helper method to disconnect UI signals to the slots.
     *
     */
    void disconnectSignals();

    /** \brief Helper method that draws the color interpolation of the range QLabel.
     *
     */
    void updateRange();

    /** \brief Fills the language combo box with the available translations.
     * \param[in] current Current language in configuration.
     *
     */
    void updateLanguageCombo(const QString &current);

    /** \brief Updates the UI when the user adds/removes fields from the tooltip.
     *
     */
    void updateTooltipFieldsButtons();

    /** \brief Fixes some layout problems that ocurr when changing language or app theme.
     *
     */
    void fixVisuals();

    /** \brief Generates and returns a pixmap for the preview and the font preview button.
     * \param[in] font QFont used to generate the pixmap.
     *
     */
    QPixmap generateTemperatureIconPixmap(QFont &font);

    std::shared_ptr<QNetworkAccessManager> m_netManager;   /** network manager.                                                                  */
    std::shared_ptr<WeatherProvider>       m_provider;     /** Weather provider.                                                                 */
    bool                                   m_testedAPIKey; /** true if the OpenWeatherMap API key has been tested and is valid, false otherwise. */
    QString                                m_DNSIP;        /** DNS server IP.                                                                    */
    QFont                                  m_font;         /** temperature tray icon font.                                                       */
    QPixmap                                m_pixmap;       /** checkered pixmap for font preview.                                                */
    int                                    m_temp;         /** temperature used to generate icon preview if not given.                           */
    bool                                   m_validFont;    /** true if current selected font can render the temperature, false otherwise.        */
};

#endif // CONFIGURATIONDIALOG_H_
