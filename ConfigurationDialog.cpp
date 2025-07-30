/*
 File: ConfigurationDialog.cpp
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

// Project
#include <ConfigurationDialog.h>
#include <LocationFinderDialog.h>
#include <Utils.h>
#include <Provider.h>
#include <Providers/OpenMeteo.h>

// Qt
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QIcon>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QColorDialog>
#include <QPainter>
#include <QSettings>
#include <QDir>
#include <QMouseEvent>
#include <QFontDialog>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QPlainTextEdit>

// C++
#include <chrono>
#include <cmath>

const char SELECTED[] = "Selected";
TemperatureUnits lastTemperatureUnits; // Last temperature unit selected.

extern QList<ProviderData> WEATHER_PROVIDERS;

//--------------------------------------------------------------------
ConfigurationDialog::ConfigurationDialog(const Configuration &configuration, QWidget* parent, Qt::WindowFlags flags)
: QDialog       {parent}
, m_netManager  {std::make_shared<NetworkAccessManager>(this)}
, m_provider    {nullptr}
, m_testedAPIKey{false}
, m_temp        {28}
, m_validFont   {true}
, m_config      {configuration}
{
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

  setupUi(this);
  connectStaticSignals();

  m_tooltipList->setItemDelegate(new RichTextItemDelegate());
  m_tooltipValueCombo->setItemDelegate(new RichTextItemDelegate());

  // generate checkered pattern background for icon preview.
  m_pixmap = QPixmap{384,384};
  const QList<QColor> colors = { Qt::white, Qt::lightGray };
  int color = 0;
  QPainter painter(&m_pixmap);
  for(int x = 0; x < 384; x +=16)
  {
    ++color;
    for(int y = 0; y < 384; y +=16)
    {
      painter.setBrush(QBrush{colors.at(++color % 2)});
      painter.setPen(colors.at(color % 2));
      painter.drawRect(x, y, 16, 16);
    }
  }
  painter.end();

  for(int i = 0; i < WEATHER_PROVIDERS.size(); ++i)
    m_providerComboBox->addItem(QIcon(WEATHER_PROVIDERS.at(i).icon), WEATHER_PROVIDERS.at(i).id);

  lastTemperatureUnits = configuration.units == Units::METRIC ? TemperatureUnits::CELSIUS : (configuration.units == Units::IMPERIAL ? TemperatureUnits::FAHRENHEIT : configuration.tempUnits);
  setConfiguration(configuration);
  setCurrentTemperature(m_temp);

  // Just for debugging purposes
  if(NetworkAccessManager::LOG_REQUESTS)
  {
    auto tabWidget = new QWidget(this);
    auto layout = new QVBoxLayout();

    auto textWidget = new QPlainTextEdit(tabWidget);
    textWidget->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
    textWidget->setReadOnly(true);
    layout->addWidget(textWidget);

    auto button = new QPushButton("Update Log", tabWidget);
    layout->addWidget(button);

    tabWidget->setLayout(layout);
    m_tabWidget->addTab(tabWidget, "Network Log");

    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(refreshNetworkLog()));
    connect(button, SIGNAL(pressed()), this, SLOT(refreshNetworkLog()));
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::replyFinished(QNetworkReply* reply)
{
  QString details, message;

  const auto url = reply->url();

  if(url.toString().contains("edns.ip", Qt::CaseInsensitive))
  {
    if(!m_DNSIP.isEmpty() && url.toString().contains(m_DNSIP, Qt::CaseSensitive))
    {
      if(reply->error() == QNetworkReply::NetworkError::NoError)
      {
        const auto data = reply->readAll();

        m_DNSIP = data.split(' ').at(1);
        m_DNSIP = m_DNSIP.remove('\n');
        QTimer::singleShot(2500, this, SLOT(requestGeolocation()));
      }
      else
      {
        message = tr("Invalid reply from Geo-Locator server.\nCouldn't get location information.\nIf you have a firewall change the configuration to allow this program to access the network.");
        details = reply->errorString();

        QMessageBox msgbox(this);
        msgbox.setWindowTitle(tr("Network Error"));
        msgbox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
        msgbox.setDetailedText(details);
        msgbox.setText(message);
        msgbox.setBaseSize(400, 400);
        msgbox.exec();
      }
    }

    reply->deleteLater();
    return;
  }

  if(url.toString().contains("ipify", Qt::CaseInsensitive))
  {
    const auto data = reply->readAll();
    m_ip->setText(data);

    reply->deleteLater();
    return;
  }

  if(url.toString().startsWith("http://ip-api.com/csv", Qt::CaseSensitive))
  {
    m_geoRequest->setEnabled(true);

    if(reply->error() == QNetworkReply::NetworkError::NoError)
    {
      message = tr("Invalid reply from Geo-Locator server.\nCouldn't get location information.\nIf you have a firewall change the configuration to allow this program to access the network.");

      const auto type = reply->header(QNetworkRequest::ContentTypeHeader);
      if(type.toString().startsWith("text/plain", Qt::CaseInsensitive))
      {
        const auto data = QString::fromUtf8(reply->readAll());
        const auto values = parseCSV(data);

        if((values.first().compare("success", Qt::CaseInsensitive) == 0) && (values.size() == 14))
        {
          m_country->setText(values.at(1));
          m_region->setText(values.at(4));
          m_city->setText(values.at(5));
          m_latitude->setText(values.at(7));
          m_longitude->setText(values.at(8));
          m_ip->setText(values.at(13));

          m_ipapiLabel->setStyleSheet("QLabel { color : green; }");
          m_ipapiLabel->setText(tr("Success"));

          m_latitudeSpin->setValue(values.at(7).toDouble());
          m_longitudeSpin->setValue(values.at(8).toDouble()); 

          reply->deleteLater();

          return;
        }
        else
        {
          m_ipapiLabel->setStyleSheet("QLabel { color : red; }");
          m_ipapiLabel->setText(tr("Failure"));

          details = tr("Error parsing location data. Failure or invalid number of fields.");
        }
      }
      else
      {
        m_ipapiLabel->setStyleSheet("QLabel { color : red; }");
        m_ipapiLabel->setText(tr("Failure"));

        details = tr("Data request failure. Invalid data format.");
      }
    }
    else
    {
      m_ipapiLabel->setStyleSheet("QLabel { color : red; }");
      m_ipapiLabel->setText(tr("Failure"));

      message = tr("Invalid reply from Geo-Locator server.");
      details = reply->errorString();
    }
  }

  if(!message.isEmpty() || !details.isEmpty())
  {
    QMessageBox msgbox(this);
    msgbox.setWindowTitle(tr("Network Error"));
    msgbox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    msgbox.setDetailedText(details);
    msgbox.setText(message);
    msgbox.setBaseSize(400, 400);
    msgbox.exec();

    return;
  }

  if(m_provider)
  {
    m_provider->processReply(reply);
  }

  reply->deleteLater();
}

//--------------------------------------------------------------------
void ConfigurationDialog::getConfiguration(Configuration &configuration) const
{
  auto currentProviderId = [&]()
  {
    if(m_provider)
    {
      if(m_provider->capabilities().requiresKey)
      {
        if(m_testedAPIKey)
          return m_provider->id();
        else
          return QString();
      }

      return m_provider->id();
    }
    
    return QString();
  };

  configuration.city            = m_city->text();
  configuration.country         = m_country->text();
  configuration.ip              = m_ip->text();
  configuration.providerId      = currentProviderId();
  configuration.region          = m_region->text();
  configuration.updateTime      = m_updateTime->value();
  configuration.units           = static_cast<Units>(m_unitsComboBox->currentIndex());
  configuration.useDNS          = m_useDNS->isChecked();
  configuration.useGeolocation  = m_useGeolocation->isChecked();
  configuration.roamingEnabled  = m_roamingCheck->isChecked();
  configuration.lightTheme      = m_theme->currentIndex() == 0;
  configuration.iconType        = static_cast<unsigned int>(m_trayIconType->currentIndex());
  configuration.iconTheme       = static_cast<unsigned int>(m_trayIconTheme->currentIndex());
  configuration.iconThemeColor  = QColor(m_iconThemeColor->property("iconColor").toString());
  configuration.trayTextColor   = QColor(m_trayTempColor->property("iconColor").toString());
  configuration.trayTextMode    = m_fixed->isChecked();
  configuration.trayTextBorder  = m_border->isChecked();
  configuration.trayBorderWidth = m_borderWidth->value();
  configuration.trayTextDegree  = m_drawDegree->isChecked();
  configuration.trayTextFont    = m_validFont ? m_font.toString() : m_fontButton->property("initial_font").toString();
  configuration.trayFontSpacing = m_spacingSpinBox->value();
  configuration.trayBorderAuto  = m_borderColor->isChecked();
  configuration.trayBorderColor = m_borderColorButton->property("iconColor").toString();
  configuration.trayBackAuto    = m_backgroundColor->isChecked();
  configuration.trayBackColor   = m_backgroundColorButton->property("iconColor").toString();
  configuration.stretchTempIcon = m_stretch->isChecked();
  configuration.minimumColor    = QColor(m_minColor->property("iconColor").toString());
  configuration.maximumColor    = QColor(m_maxColor->property("iconColor").toString());
  configuration.minimumValue    = m_minSpinBox->value();
  configuration.maximumValue    = m_maxSpinBox->value();
  configuration.update          = static_cast<Update>(m_updatesCombo->currentIndex());
  configuration.autostart       = m_autostart->isChecked();
  configuration.language        = m_languageCombo->itemData(m_languageCombo->currentIndex(), Qt::UserRole).toString();
  configuration.tempUnits       = static_cast<TemperatureUnits>(m_tempCombo->currentIndex());
  configuration.pressureUnits   = static_cast<PressureUnits>(m_pressionCombo->currentIndex());
  configuration.precUnits       = static_cast<PrecipitationUnits>(m_precipitationCombo->currentIndex());
  configuration.windUnits       = static_cast<WindUnits>(m_windCombo->currentIndex());
  configuration.showAlerts      = m_showAlerts->isChecked();
  configuration.keepAlertIcon   = m_keepAlertsIcon->isChecked();
  configuration.swapTrayIcons   = m_swapIcons->isChecked();
  configuration.trayIconSize    = m_iconSize->value();
  configuration.tempRepr        = static_cast<Representation>(m_tempGraph->currentIndex());
  configuration.rainRepr        = static_cast<Representation>(m_rainGraph->currentIndex());
  configuration.snowRepr        = static_cast<Representation>(m_snowGraph->currentIndex());
  configuration.tempReprColor   = QColor(m_tempGraphColor->property("iconColor").toString());
  configuration.rainReprColor   = QColor(m_rainGraphColor->property("iconColor").toString());
  configuration.snowReprColor   = QColor(m_snowGraphColor->property("iconColor").toString());
  configuration.tempMapOpacity  = static_cast<float>(m_tempLayerSlider->value() / 100.);
  configuration.cloudMapOpacity = static_cast<float>(m_cloudLayerSlider->value() / 100.);
  configuration.rainMapOpacity  = static_cast<float>(m_rainLayerSlider->value() / 100.);
  configuration.windMapOpacity  = static_cast<float>(m_windLayerSlider->value() / 100.);
  configuration.barWidth        = m_barWidthSpinbox->value();

  configuration.tooltipFields.clear();
  for(int row = 0; row < m_tooltipList->count(); ++row)
  {
    const auto item = m_tooltipList->item(row);
    configuration.tooltipFields << static_cast<TooltipText>(item->data(Qt::UserRole).toInt());
  }

  if(m_useManual->isChecked())
  {
    configuration.latitude  = std::min(90.0, std::max(-90.0, m_latitudeSpin->value()));
    configuration.longitude = std::min(180.0, std::max(-180.0, m_longitudeSpin->value()));
  }
  else
  {
    configuration.latitude  = !m_latitude->text().isEmpty() ? m_latitude->text().toDouble() : -90.0;
    configuration.longitude = !m_longitude->text().isEmpty() ? m_longitude->text().toDouble() : -180.0;
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::requestGeolocation()
{
  if(!m_useGeolocation->isChecked()) return;

  // protect from abuse
  static auto lastRequest = std::chrono::steady_clock::now();
  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<double> interval = now - lastRequest;
  const auto duration = interval.count();

  if(duration > 0 && duration < 2.5) return;
  lastRequest = now;

  if(m_useDNS->isChecked() && m_DNSIP.isEmpty())
  {
    requestDNSIPGeolocation();
    return;
  }

  if(m_DNSIP.length() == 32)
  {
    // it hasn't received a reply for DNS IP.
    return;
  }

  // CSV is easier to parse later.
  auto ipAddress = QString("http://ip-api.com/csv/%1").arg(m_DNSIP);
  m_netManager->get(QNetworkRequest{QUrl{ipAddress}});

  m_DNSIP.clear();

  m_geoRequest->setEnabled(false);
  m_ipapiLabel->setStyleSheet("QLabel { color : blue; }");
  m_ipapiLabel->setText(tr("Requesting..."));
}

//--------------------------------------------------------------------
void ConfigurationDialog::requestDNSIPGeolocation()
{
  if(!m_useGeolocation->isChecked()) return;

  // protect from abuse
  static auto lastRequest = std::chrono::steady_clock::now();
  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<double> interval = now - lastRequest;
  const auto duration = interval.count();

  if(duration > 0 && duration < 2.5) return;
  lastRequest = now;

  // CSV is easier to parse later.
  m_DNSIP = randomString();
  auto requestAddress = QString("http://%1.edns.ip-api.com/csv").arg(m_DNSIP);
  m_netManager->get(QNetworkRequest{QUrl{requestAddress}});

  m_geoRequest->setEnabled(false);
  m_ipapiLabel->setStyleSheet("QLabel { color : blue; }");
  m_ipapiLabel->setText(tr("Requesting..."));
}

//--------------------------------------------------------------------
void ConfigurationDialog::connectStaticSignals()
{
  // Clickable labels clicked to checkbox setChecked.
  m_borderLabel->connectToCheckBox(m_border);
  m_stretchLabel->connectToCheckBox(m_stretch);
  m_drawDegreeLabel->connectToCheckBox(m_drawDegree);
  m_backgroundColorLabel->connectToCheckBox(m_backgroundColor);
  m_borderColorLabel->connectToCheckBox(m_borderColor);

  // Checkbox values to buttons setEnable
  QObject::connect(m_backgroundColor, &QCheckBox::toggled, [this](bool value){ m_backgroundColorButton->setEnabled(!value); });
  QObject::connect(m_borderColor, &QCheckBox::toggled, [this](bool value){ m_borderColorButton->setEnabled(!value); });
}

//--------------------------------------------------------------------
void ConfigurationDialog::requestAPIKeyTest() 
{
  const double latitude = m_latitudeSpin->value();
  const double longitude = m_longitudeSpin->value(); 

  if(m_apikey->text().isEmpty())
  {
    QMessageBox msgbox(this);
    msgbox.setWindowTitle(tr("API Key Error"));
    msgbox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    msgbox.setText(tr("API key missing!"));
    msgbox.exec();
    return;
  }

  if((latitude > 90.0) || (latitude < -90.0) || (longitude > 180.0) || (longitude < -180.0))
  {
    QMessageBox msgbox(this);
    msgbox.setWindowTitle(tr("Location Error"));
    msgbox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    msgbox.setText(tr("You must set a valid location before testing the API key."));
    msgbox.exec();
    return;
  }

  m_config.latitude = latitude;
  m_config.longitude = longitude;

  if(m_provider && m_provider->capabilities().requiresKey)
  {
    m_provider->setApiKey(m_apikey->text());
    m_provider->testApiKey(m_netManager);
  }

  m_apiTest->setEnabled(false);
  m_testLabel->setStyleSheet("QLabel { color : blue; }");
  m_testLabel->setText(tr("Testing API Key..."));
}

//--------------------------------------------------------------------
void ConfigurationDialog::onDNSRequestStateChanged(int state)
{
  if(!m_useGeolocation->isChecked()) return;

  if(m_useDNS->isChecked())
  {
    requestDNSIPGeolocation();
  }
  else
  {
    m_DNSIP.clear();
    requestGeolocation();
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::connectSignals()
{
  connect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
          this,               SLOT(replyFinished(QNetworkReply*)));

  connect(m_apiTest, SIGNAL(pressed()),
          this,      SLOT(requestAPIKeyTest()));

  connect(m_geoRequest, SIGNAL(pressed()),
          this,         SLOT(requestGeolocation()));

  connect(m_useDNS, SIGNAL(stateChanged(int)),
          this,     SLOT(onDNSRequestStateChanged(int)));

  connect(m_useGeolocation, SIGNAL(toggled(bool)),
          this,             SLOT(onLocationRadioChanged()));

  connect(m_useManual, SIGNAL(toggled(bool)),
          this,        SLOT(onLocationRadioChanged()));

  connect(m_longitudeSpin, SIGNAL(editingFinished()),
          this,            SLOT(onCoordinatesChanged()));

  connect(m_latitudeSpin, SIGNAL(editingFinished()),
          this,           SLOT(onCoordinatesChanged()));

  connect(m_theme, SIGNAL(currentIndexChanged(int)),
         this,     SLOT(onThemeIndexChanged(int)));

  connect(m_minSpinBox, SIGNAL(valueChanged(int)),
          this,         SLOT(onTemperatureValueChanged(int)));

  connect(m_maxSpinBox, SIGNAL(valueChanged(int)),
          this,         SLOT(onTemperatureValueChanged(int)));

  connect(m_autostart, SIGNAL(stateChanged(int)),
          this,        SLOT(onAutostartValueChanged(int)));

  connect(m_languageCombo, SIGNAL(currentIndexChanged(int)),
          this,            SLOT(onLanguageChanged(int)));

  connect(m_unitsComboBox, SIGNAL(currentIndexChanged(int)),
          this,            SLOT(onUnitsValueChanged(int)));

  connect(m_tooltipAdd, SIGNAL(clicked()),
          this,         SLOT(onTooltipTextAdded()));

  connect(m_tooltipDelete, SIGNAL(clicked()),
          this,            SLOT(onTooltipTextDeleted()));

  connect(m_tooltipDown, SIGNAL(clicked()),
          this,          SLOT(onTooltipTextMoved()));

  connect(m_tooltipUp, SIGNAL(clicked()),
          this,        SLOT(onTooltipTextMoved()));

  connect(m_tooltipList, SIGNAL(currentRowChanged(int)),
          this,          SLOT(onTooltipFieldsRowChanged(int)));

  connect(m_iconSummary, SIGNAL(pressed()),
          this,          SLOT(onIconPreviewPressed()));

  connect(m_trayIconTheme, SIGNAL(currentIndexChanged(int)),
          this,            SLOT(onIconThemeIndexChanged(int)));

  for(auto &w: {m_trayTempColor, m_minColor, m_maxColor, m_iconThemeColor,
                m_tempGraphColor, m_rainGraphColor, m_snowGraphColor, 
                m_backgroundColorButton, m_borderColorButton })
  {
    connect(w, SIGNAL(clicked()), this, SLOT(onColorButtonClicked()));
  }

  for(auto &w: {m_tempCombo, m_pressionCombo, m_windCombo, m_precipitationCombo})
  {
    connect(w, SIGNAL(currentIndexChanged(int)), this, SLOT(onUnitComboChanged(int)));
  }

  connect(m_fontButton, SIGNAL(clicked()),
          this,         SLOT(onFontSelectorPressed()));

  connect(m_fontPreview, SIGNAL(clicked()),
          this,          SLOT(onFontPreviewPressed()));

  connect(m_border, SIGNAL(stateChanged(int)),
          this,     SLOT(updateTemperatureIcon()));

  connect(m_borderColor, SIGNAL(stateChanged(int)),
          this,          SLOT(updateTemperatureIcon()));

  connect(m_backgroundColor, SIGNAL(stateChanged(int)),
          this,              SLOT(updateIconBackgroundColor()));

  connect(m_border, SIGNAL(stateChanged(int)),
          this,     SLOT(onBorderStateChanged()));

  connect(m_drawDegree, SIGNAL(stateChanged(int)),
          this,         SLOT(updateTemperatureIcon()));

  connect(m_borderWidth, SIGNAL(valueChanged(int)),
          this,          SLOT(updateTemperatureIcon()));

  connect(m_fixed, SIGNAL(toggled(bool)),
          this,    SLOT(updateTemperatureIcon()));

  connect(m_stretch, SIGNAL(stateChanged(int)),
          this,      SLOT(updateTemperatureIcon()));

  connect(m_trayIconType, SIGNAL(currentIndexChanged(int)),
          this,           SLOT(onIconTypeChanged(int)));

  connect(m_iconSize, SIGNAL(valueChanged(int)),
          this,       SLOT(updateTemperatureIcon()));

  connect(m_geoFind, SIGNAL(clicked()), 
          this,      SLOT(onSearchButtonClicked()));

  connect(m_providerComboBox, SIGNAL(currentIndexChanged(int)), 
          this,      SLOT(onProviderChanged(int)));

  connect(m_spacingSpinBox, SIGNAL(valueChanged(int)),
          this,            SLOT(updateTemperatureIcon()));

  connect(m_barWidthSlider, SIGNAL(valueChanged(int)), 
          this, SLOT(onBarWidthSliderChanged(int)));

  connect(m_barWidthSpinbox, SIGNAL(valueChanged(double)), 
          this, SLOT(onBarWidthSpinboxChanged(double)));

  connectProviderSignals();
}

//--------------------------------------------------------------------
void ConfigurationDialog::connectProviderSignals()
{
  if(m_provider)
  {
    connect(m_provider.get(), SIGNAL(apiKeyValid(bool)), 
            this,             SLOT(apiKeyValid(bool)));
    connect(m_provider.get(), SIGNAL(errorMessage(const QString &)), 
            this,             SLOT(providerErrorMessage(const QString &)));
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::disconnectProviderSignals()
{
  if(m_provider)
  {
    disconnect(m_provider.get(), SIGNAL(apiKeyValid(bool)), 
               this,             SLOT(apiKeyValid(bool)));
    disconnect(m_provider.get(), SIGNAL(errorMessage(const QString &)), 
               this,             SLOT(providerErrorMessage(const QString &)));
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::onLocationRadioChanged()
{
  auto manualEnabled = m_useManual->isChecked();

  m_longitudeLabel->setEnabled(manualEnabled);
  m_longitudeSpin->setEnabled(manualEnabled);
  m_latitudeLabel->setEnabled(manualEnabled);
  m_latitudeSpin->setEnabled(manualEnabled);

  m_ipapiLabel->clear();
  m_ipapiLabel->setEnabled(!manualEnabled);
  m_geoRequest->setEnabled(!manualEnabled);
  m_useDNS->setEnabled(!manualEnabled);
  m_geoBox->setEnabled(!manualEnabled);

  if(!manualEnabled) requestGeolocation();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onCoordinatesChanged()
{
  auto longitude = std::min(180.0, std::max(-180.0, m_longitudeSpin->value()));
  auto latitude  = std::min(90.0, std::max(-90.0, m_latitudeSpin->value()));

  m_longitudeSpin->setValue(longitude);
  m_latitudeSpin->setValue(latitude);
}

//--------------------------------------------------------------------
void ConfigurationDialog::onThemeIndexChanged(int index)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  // We need Qt to resize the dialog according to its contents...
  const auto currentIndex = m_tabWidget->currentIndex();
  m_tabWidget->setCurrentIndex(0);

  setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
  setMinimumSize(0,0);

  QString sheet;

  if(index != 0)
  {
    QFile file(":qdarkstyle/style.qss");
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts(&file);
    sheet = ts.readAll();
  }

  qApp->setStyleSheet(sheet);

  adjustSize();
  fixVisuals();
  setFixedSize(size() + QSize{8,8});

  updateRange();

  // ...and then return to the one tab the user is in.
  m_tabWidget->setCurrentIndex(currentIndex);

  QApplication::restoreOverrideCursor();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onColorButtonClicked()
{
  auto button = qobject_cast<QToolButton *>(sender());
  auto color  = QColor(button->property("iconColor").toString());

  QColorDialog dialog;
  dialog.setCurrentColor(color);
  dialog.setOption(QColorDialog::ColorDialogOption::ShowAlphaChannel, true);
  dialog.setWindowIcon(QIcon(":/TrayWeather/application.svg"));

  if(dialog.exec() != QColorDialog::Accepted) return;

  color = dialog.selectedColor();

  QPixmap icon(QSize(64,64));
  icon.fill(color);
  button->setIcon(QIcon(icon));
  button->setProperty("iconColor", color.name(QColor::HexArgb));

  if(button == m_minColor || button == m_maxColor)
  {
    updateRange();
    updateTemperatureIcon();
    return;
  }

  if(button == m_iconThemeColor)
  {
    onIconThemeIndexChanged(m_trayIconTheme->currentIndex());
    return;
  }

  if(button == m_backgroundColorButton)
  {
    onIconThemeIndexChanged(m_trayIconTheme->currentIndex());
  }

  updateTemperatureIcon();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onTemperatureValueChanged(int value)
{
  auto spinBox = qobject_cast<QSpinBox *>(sender());
  if(spinBox == m_minSpinBox)
  {
    m_minSpinBox->setMaximum(m_maxSpinBox->value()-1);
    m_maxSpinBox->setMinimum(value + 1);
  }
  else
  {
    m_maxSpinBox->setMinimum(m_minSpinBox->value()+1);
    m_minSpinBox->setMaximum(value - 1);
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::updateRange()
{
  auto minColor = QColor(m_minColor->property("iconColor").toString());
  auto maxColor = QColor(m_maxColor->property("iconColor").toString());

  double inc = 1./100;
  double rStep = (maxColor.red()   - minColor.red())   *inc;
  double gStep = (maxColor.green() - minColor.green()) *inc;
  double bStep = (maxColor.blue()  - minColor.blue())  *inc;
  double aStep = (maxColor.alpha() - minColor.alpha()) *inc;

  QImage range(QSize{100,30}, QImage::Format_ARGB32_Premultiplied);
  range.fill(qRgba(0, 0, 0, 0));
  QPainter painter(&range);
  for(int i = 0; i < 100; ++i)
  {
    const auto newColor = QColor::fromRgb(minColor.red() + (i * rStep), minColor.green() + (i * gStep), minColor.blue() + (i * bStep), minColor.alpha() + (i * aStep));
    painter.setPen(newColor);
    painter.drawLine(i, 5, i, 25);
  }
  painter.end();

  m_range->setPixmap(QPixmap::fromImage(range));
}

//--------------------------------------------------------------------
void ConfigurationDialog::showEvent(QShowEvent *e)
{
  this->m_tabWidget->setElideMode(Qt::ElideNone);
  this->m_tabWidget->setCurrentIndex(0);

  QDialog::showEvent(e);
  scaleDialog(this);
  fixVisuals();

  if(m_provider && m_provider->capabilities().requiresKey && !m_provider->apikey().isEmpty())
     requestAPIKeyTest();
}

//--------------------------------------------------------------------
void ConfigurationDialog::changeEvent(QEvent *e)
{
  if(e && e->type() == QEvent::LanguageChange)
  {
    Configuration backup;
    getConfiguration(backup);

    retranslateUi(this);

    fixVisuals();

    disconnectSignals();
    m_languageCombo->blockSignals(true);
    setConfiguration(backup);
    m_languageCombo->blockSignals(false);

    QString sheet;

    if(!backup.lightTheme)
    {
      QFile file(":qdarkstyle/style.qss");
      file.open(QFile::ReadOnly | QFile::Text);
      QTextStream ts(&file);
      sheet = ts.readAll();
    }

    qApp->setStyleSheet(sheet);
  }

  QDialog::changeEvent(e);
}

//--------------------------------------------------------------------
void ConfigurationDialog::onAutostartValueChanged(int value)
{
  setAutorunsKey(m_config, value);
}

//--------------------------------------------------------------------
void ConfigurationDialog::setConfiguration(const Configuration &configuration)
{
  m_autostart->setChecked(configuration.autostart);
  setAutorunsKey(configuration, configuration.autostart);

  m_trayIconTheme->clear();
  for(int i = 0; i < ICON_THEMES.size(); ++i)
  {
    m_trayIconTheme->insertItem(i, ICON_THEMES.at(i).name);
  }

  QPixmap icon(QSize(64,64));
  icon.fill(configuration.iconThemeColor);
  m_iconThemeColor->setIcon(QIcon(icon));
  m_iconThemeColor->setProperty("iconColor", configuration.iconThemeColor.name(QColor::HexArgb));

  onIconThemeIndexChanged(static_cast<int>(configuration.iconTheme));

  connectSignals();

  const auto providerIndex = configuration.providerId.isEmpty() ? WeatherProviderFactory::indexOf(OPENMETEO_PROVIDER) : WeatherProviderFactory::indexOf(configuration.providerId);
  onProviderChanged(providerIndex);

  m_useManual->setChecked(!configuration.useGeolocation);
  m_useGeolocation->setChecked(configuration.useGeolocation);
  m_city->setText(configuration.city);
  m_country->setText(configuration.country);
  m_ip->setText(configuration.ip);
  m_region->setText(configuration.region);
  m_updateTime->setValue(configuration.updateTime);
  m_useDNS->setChecked(configuration.useDNS);
  m_roamingCheck->setChecked(configuration.roamingEnabled);
  m_apikey->setText(m_provider ? m_provider->apikey() : QString());
  m_theme->setCurrentIndex(configuration.lightTheme ? 0 : 1);
  m_trayIconTheme->setCurrentIndex(static_cast<int>(configuration.iconTheme));
  m_iconSize->setValue(configuration.trayIconSize);
  m_barWidthSlider->setValue(configuration.barWidth);
  m_barWidthSpinbox->setValue(configuration.barWidth);

  m_font.fromString(configuration.trayTextFont);
  m_spacingSlider->setValue(configuration.trayFontSpacing);
  m_spacingSpinBox->setValue(configuration.trayFontSpacing);
  m_fontButton->setText(m_font.toString().split(",").first());
  if(m_validFont) m_fontButton->setProperty("initial_font", m_font.toString());

  m_trayIconType->setCurrentIndex(static_cast<int>(configuration.iconType));
  m_updatesCombo->setCurrentIndex(static_cast<int>(configuration.update));

  m_unitsComboBox->setCurrentIndex(static_cast<int>(configuration.units));
  m_tempCombo->setCurrentIndex(static_cast<int>(configuration.tempUnits));
  m_minSpinBox->setValue(configuration.minimumValue);
  m_maxSpinBox->setValue(configuration.maximumValue);
  m_pressionCombo->setCurrentIndex(static_cast<int>(configuration.pressureUnits));
  m_precipitationCombo->setCurrentIndex(static_cast<int>(configuration.precUnits));
  m_windCombo->setCurrentIndex(static_cast<int>(configuration.windUnits));

  m_fixed->setChecked(configuration.trayTextMode);
  m_variable->setChecked(!configuration.trayTextMode);
  m_border->setChecked(configuration.trayTextBorder);
  m_borderColor->setChecked(configuration.trayBorderAuto);
  m_borderWidth->setValue(configuration.trayBorderWidth);
  m_stretch->setChecked(configuration.stretchTempIcon);
  m_drawDegree->setChecked(configuration.trayTextDegree);
  m_backgroundColor->setChecked(configuration.trayBackAuto);

  m_backgroundColorButton->setEnabled(!configuration.trayBackAuto);
  m_borderColorButton->setEnabled(!configuration.trayBorderAuto && m_borderColor->isEnabled());

  m_minSpinBox->setMaximum(configuration.maximumValue-1);
  m_maxSpinBox->setMinimum(configuration.minimumValue+1);

  m_tempGraph->setCurrentIndex(static_cast<int>(configuration.tempRepr));
  m_rainGraph->setCurrentIndex(static_cast<int>(configuration.rainRepr));
  m_snowGraph->setCurrentIndex(static_cast<int>(configuration.snowRepr));

  m_tooltipList->clear();
  m_tooltipList->setAlternatingRowColors(true);

  m_showAlerts->setChecked(configuration.showAlerts);
  m_keepAlertsIcon->setChecked(configuration.keepAlertIcon);

  onIconTypeChanged(configuration.iconType);
  m_swapIcons->setChecked(configuration.swapTrayIcons);

  for(int i = 0; i < configuration.tooltipFields.size(); ++i)
  {
    const auto field = configuration.tooltipFields.at(i);
    const auto idx   = static_cast<int>(field);
    const auto fieldText = QApplication::translate("QObject", TooltipTextFields.at(idx).toStdString().c_str());

    auto item = new QListWidgetItem();
    item->setData(Qt::UserRole, idx);
    item->setData(Qt::DisplayRole, fieldText);
    m_tooltipList->addItem(item);
  }

  m_tooltipValueCombo->clear();

  for(int i = 0; i < static_cast<int>(TooltipText::MAX); ++i)
  {
    auto field = static_cast<TooltipText>(i);
    if(configuration.tooltipFields.contains(field)) continue;

    const auto fieldText = QApplication::translate("QObject", TooltipTextFields.at(i).toStdString().c_str());
    m_tooltipValueCombo->addItem(QIcon(), fieldText, i);
  }

  m_tooltipList->setCurrentRow(0);
  m_tooltipValueCombo->setCurrentIndex(0);

  icon.fill(configuration.trayTextColor);
  m_trayTempColor->setIcon(QIcon(icon));
  m_trayTempColor->setProperty("iconColor", configuration.trayTextColor.name(QColor::HexArgb));

  icon.fill(configuration.minimumColor);
  m_minColor->setIcon(QIcon(icon));
  m_minColor->setProperty("iconColor", configuration.minimumColor.name(QColor::HexArgb));

  icon.fill(configuration.maximumColor);
  m_maxColor->setIcon(QIcon(icon));
  m_maxColor->setProperty("iconColor", configuration.maximumColor.name(QColor::HexArgb));

  icon.fill(configuration.tempReprColor);
  m_tempGraphColor->setIcon(QIcon(icon));
  m_tempGraphColor->setProperty("iconColor", configuration.tempReprColor.name(QColor::HexArgb));

  icon.fill(configuration.rainReprColor);
  m_rainGraphColor->setIcon(QIcon(icon));
  m_rainGraphColor->setProperty("iconColor", configuration.rainReprColor.name(QColor::HexArgb));

  icon.fill(configuration.snowReprColor);
  m_snowGraphColor->setIcon(QIcon(icon));
  m_snowGraphColor->setProperty("iconColor", configuration.snowReprColor.name(QColor::HexArgb));

  icon.fill(configuration.trayBorderColor);
  m_borderColorButton->setIcon(QIcon(icon));
  m_borderColorButton->setProperty("iconColor", configuration.trayBorderColor.name(QColor::HexArgb));

  icon.fill(configuration.trayBackColor);
  m_backgroundColorButton->setIcon(QIcon(icon));
  m_backgroundColorButton->setProperty("iconColor", configuration.trayBackColor.name(QColor::HexArgb));

  m_longitudeSpin->setValue(configuration.longitude);
  m_latitudeSpin->setValue(configuration.latitude);

  m_latitude->setText(QString::number(configuration.latitude));
  m_longitude->setText(QString::number(configuration.longitude));

  if(!configuration.isValid())
  {
    m_updateTime->setValue(15);
    m_unitsComboBox->setCurrentIndex(0);
    m_providerComboBox->setCurrentIndex(providerIndex);
  }
  else
  {
    const auto position = WeatherProviderFactory::indexOf(configuration.providerId);
    m_providerComboBox->setCurrentIndex(position == -1 ? 0 : position);
  }

  // this requests geolocation if checked.
  onLocationRadioChanged();
  onCoordinatesChanged();

  if (!configuration.isValid()) {
      if (m_provider && m_provider->capabilities().requiresKey) {
          m_apiTest->setEnabled(true);

          if (m_provider->apikey().isEmpty()) {
              m_testLabel->setStyleSheet("QLabel { color : red; }");
              m_testLabel->setText(tr("Invalid API Key!"));
          } else {
              m_testLabel->setStyleSheet("QLabel { color : red; }");
              m_testLabel->setText(tr("Untested API Key!"));
          }
      } else {
          m_testLabel->clear();
          m_testLabel->setEnabled(false);
          m_apiTest->setEnabled(false);
      }
  }

  m_tempCombo->setProperty(SELECTED, static_cast<int>(configuration.tempUnits));
  m_pressionCombo->setProperty(SELECTED, static_cast<int>(configuration.pressureUnits));
  m_windCombo->setProperty(SELECTED, static_cast<int>(configuration.windUnits));
  m_precipitationCombo->setProperty(SELECTED, static_cast<int>(configuration.precUnits));

  m_tempLayerSlider->setValue(static_cast<int>(configuration.tempMapOpacity * 100));
  m_cloudLayerSlider->setValue(static_cast<int>(configuration.cloudMapOpacity * 100));
  m_rainLayerSlider->setValue(static_cast<int>(configuration.rainMapOpacity * 100));
  m_windLayerSlider->setValue(static_cast<int>(configuration.windMapOpacity * 100));

  setFixedSize(size());
  updateRange();
  updateLanguageCombo(configuration.language);
}

//--------------------------------------------------------------------
void ConfigurationDialog::disconnectSignals()
{
  disconnect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
             this,               SLOT(replyFinished(QNetworkReply*)));

  disconnect(m_apiTest, SIGNAL(pressed()),
             this,      SLOT(requestAPIKeyTest()));

  disconnect(m_geoRequest, SIGNAL(pressed()),
             this,         SLOT(requestGeolocation()));

  disconnect(m_useDNS, SIGNAL(stateChanged(int)),
             this,     SLOT(onDNSRequestStateChanged(int)));

  disconnect(m_useGeolocation, SIGNAL(toggled(bool)),
             this,             SLOT(onLocationRadioChanged()));

  disconnect(m_useManual, SIGNAL(toggled(bool)),
             this,        SLOT(onLocationRadioChanged()));

  disconnect(m_longitudeSpin, SIGNAL(editingFinished()),
             this,            SLOT(onCoordinatesChanged()));

  disconnect(m_latitudeSpin, SIGNAL(editingFinished()),
             this,           SLOT(onCoordinatesChanged()));

  disconnect(m_theme, SIGNAL(currentIndexChanged(int)),
             this,     SLOT(onThemeIndexChanged(int)));

  disconnect(m_minSpinBox, SIGNAL(valueChanged(int)),
             this,         SLOT(onTemperatureValueChanged(int)));

  disconnect(m_maxSpinBox, SIGNAL(valueChanged(int)),
             this,         SLOT(onTemperatureValueChanged(int)));

  disconnect(m_autostart, SIGNAL(stateChanged(int)),
             this,        SLOT(onAutostartValueChanged(int)));

  disconnect(m_languageCombo, SIGNAL(currentIndexChanged(int)),
             this,            SLOT(onLanguageChanged(int)));

  disconnect(m_unitsComboBox, SIGNAL(currentIndexChanged(int)),
             this,            SLOT(onUnitsValueChanged(int)));

  disconnect(m_tooltipAdd, SIGNAL(clicked()),
             this,         SLOT(onTooltipTextAdded()));

  disconnect(m_tooltipDelete, SIGNAL(clicked()),
             this,            SLOT(onTooltipTextDeleted()));

  disconnect(m_tooltipDown, SIGNAL(clicked()),
             this,          SLOT(onTooltipTextMoved()));

  disconnect(m_tooltipUp, SIGNAL(clicked()),
             this,        SLOT(onTooltipTextMoved()));

  disconnect(m_tooltipList, SIGNAL(currentRowChanged(int)),
             this,          SLOT(onTooltipFieldsRowChanged(int)));

  disconnect(m_iconSummary, SIGNAL(pressed()),
             this,          SLOT(onIconPreviewPressed()));

  disconnect(m_trayIconTheme, SIGNAL(currentIndexChanged(int)),
             this,            SLOT(onIconThemeIndexChanged(int)));

  for(auto &w: {m_trayTempColor, m_minColor, m_maxColor, m_iconThemeColor,
                m_tempGraphColor, m_rainGraphColor, m_snowGraphColor})
  {
    disconnect(w, SIGNAL(clicked()), this, SLOT(onColorButtonClicked()));
  }

  for(auto &w: {m_tempCombo, m_pressionCombo, m_windCombo, m_precipitationCombo})
  {
    disconnect(w, SIGNAL(currentIndexChanged(int)), this, SLOT(onUnitComboChanged(int)));
  }

  disconnect(m_fontButton, SIGNAL(clicked()),
             this,         SLOT(onFontSelectorPressed()));

  disconnect(m_fontPreview, SIGNAL(clicked()),
             this,          SLOT(onFontPreviewPressed()));

  disconnect(m_border, SIGNAL(stateChanged(int)),
             this,     SLOT(updateTemperatureIcon()));

  disconnect(m_border, SIGNAL(stateChanged(int)),
             this,     SLOT(onBorderStateChanged()));

  disconnect(m_drawDegree, SIGNAL(stateChanged(int)),
             this,         SLOT(updateTemperatureIcon()));

  disconnect(m_borderWidth, SIGNAL(valueChanged(int)),
             this,          SLOT(updateTemperatureIcon()));

  disconnect(m_fixed, SIGNAL(toggled(bool)),
             this,    SLOT(updateTemperatureIcon()));

  disconnect(m_stretch, SIGNAL(stateChanged(int)),
             this,      SLOT(updateTemperatureIcon()));

  disconnect(m_trayIconType, SIGNAL(currentIndexChanged(int)),
             this,           SLOT(onIconTypeChanged(int)));

  disconnect(m_iconSize, SIGNAL(valueChanged(int)),
             this,       SLOT(updateTemperatureIcon()));

  disconnect(m_geoFind, SIGNAL(clicked()), 
             this,      SLOT(onSearchButtonClicked()));

  disconnect(m_spacingSpinBox, SIGNAL(valueChanged(int)),
             this,            SLOT(updateTemperatureIcon()));

  disconnect(m_barWidthSlider, SIGNAL(valueChanged(int)), 
             this, SLOT(onBarWidthSliderChanged(int)));

  disconnect(m_barWidthSpinbox, SIGNAL(valueChanged(double)), 
             this, SLOT(onBarWidthSpinboxChanged(double)));

  disconnectProviderSignals();             
}

//--------------------------------------------------------------------
void ConfigurationDialog::onTooltipTextAdded()
{
  const auto index = m_tooltipValueCombo->currentIndex();
  const auto tooltipIndex = m_tooltipValueCombo->itemData(index, Qt::UserRole).toInt();
  const auto text = m_tooltipValueCombo->itemData(index, Qt::DisplayRole).toString();
  m_tooltipValueCombo->removeItem(index);

  auto item = new QListWidgetItem(text);
  item->setData(Qt::UserRole, tooltipIndex);
  m_tooltipList->addItem(item);

  updateTooltipFieldsButtons();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onTooltipTextDeleted()
{
  const auto row = m_tooltipList->currentRow();
  const auto item = m_tooltipList->takeItem(row);
  const auto tooltipIndex = item->data(Qt::UserRole).toInt();
  const auto text = item->data(Qt::DisplayRole).toString();
  delete item;

  m_tooltipValueCombo->addItem(QIcon(), text, tooltipIndex);

  updateTooltipFieldsButtons();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onTooltipTextMoved()
{
  auto button = qobject_cast<QToolButton *>(sender());
  if(!button) return;

  auto row = m_tooltipList->currentRow();
  auto item = m_tooltipList->takeItem(row);
  const auto tooltipIdx = item->data(Qt::UserRole).toInt();
  const auto text = item->data(Qt::DisplayRole).toString();

  delete item;
  item = new QListWidgetItem(text);
  item->setData(Qt::UserRole, tooltipIdx);

  if(button == m_tooltipUp)
  {
    row -= 1;
  }
  else
  {
    row += 1;
  }

  m_tooltipList->insertItem(row, item);
  m_tooltipList->setCurrentRow(row);
}

//--------------------------------------------------------------------
void ConfigurationDialog::updateLanguageCombo(const QString &current)
{
  m_languageCombo->clear();

  int selected = 0;
  auto translations = TRANSLATIONS;
  std::sort(translations.begin(), translations.end());

  for(int i = 0; i < translations.size(); ++i)
  {
    const auto &lang = translations.at(i);
    m_languageCombo->addItem(QIcon(lang.icon), lang.name, lang.file);
    if(lang.file.compare(current) == 0) selected = i;
  }
  m_languageCombo->blockSignals(true);
  m_languageCombo->setCurrentIndex(selected);
  m_languageCombo->blockSignals(false);
}

//--------------------------------------------------------------------
void ConfigurationDialog::onLanguageChanged(int index)
{
  auto translations = TRANSLATIONS;
  std::sort(translations.begin(), translations.end());

  auto idx = std::max(std::min(index, TRANSLATIONS.size() - 1), 0);

  emit languageChanged(translations.at(idx).file);
}

//--------------------------------------------------------------------
void ConfigurationDialog::onUnitsValueChanged(int index)
{
  auto setCustomUnits = [this](int idx)
  {
    m_customBox->setEnabled(false);
    for(auto w: {m_tempCombo, m_pressionCombo, m_windCombo, m_precipitationCombo})
    {
      w->blockSignals(true);
      w->setCurrentIndex(idx);
      w->blockSignals(false);
    }
  };

  switch(index)
  {
    default:
      m_unitsComboBox->blockSignals(true);
      m_unitsComboBox->setCurrentIndex(0);
      m_unitsComboBox->blockSignals(false);
      /* fall through */
    case 0:
      setCustomUnits(0);
      break;
    case 1:
      setCustomUnits(1);
      break;
    case 2:
      m_customBox->setEnabled(true);
      for(auto &w: {m_tempCombo, m_pressionCombo, m_windCombo, m_precipitationCombo})
      {
        const auto value = w->property(SELECTED);
        if(value.isValid() && value.canConvert<int>())
        {
          w->setCurrentIndex(value.toInt());
        }
      }
      break;
  }

  onTemperatureUnitsChanged();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onUnitComboChanged(int index)
{
  auto w = qobject_cast<QComboBox*>(sender());
  if(w)
  {
    w->setProperty(SELECTED, index);

    if(w == m_tempCombo)
      onTemperatureUnitsChanged();
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::onTooltipFieldsRowChanged(int row)
{
  m_tooltipUp->setEnabled(row != 0);
  m_tooltipDown->setEnabled(row + 1 != m_tooltipList->count());
}

//--------------------------------------------------------------------
void ConfigurationDialog::updateTooltipFieldsButtons()
{
  auto enabled = (m_tooltipValueCombo->count() != 0);
  m_tooltipValueCombo->setEnabled(enabled);
  m_tooltipAdd->setEnabled(enabled);

  enabled = m_tooltipList->count() != 0;
  m_tooltipDelete->setEnabled(enabled);
}

//--------------------------------------------------------------------
void ConfigurationDialog::onIconPreviewPressed()
{
  const auto color = QColor(m_iconThemeColor->property("iconColor").toString());
  const auto invertedColor = QColor{color.red() ^ 0xFF, color.green() ^ 0xFF, color.blue() ^ 0xFF, 100};
  const auto iconTheme = m_trayIconTheme->currentIndex();
  QColor backgroundColor = ICON_THEMES.at(iconTheme).colored ? Qt::transparent : invertedColor;

  if(!m_backgroundColor->isChecked()) 
    backgroundColor = QColor(m_backgroundColorButton->property("iconColor").toString());

  const auto image = createIconsSummary(iconTheme, 32, color, backgroundColor).toImage();

  QPixmap background(m_pixmap.scaled(image.size()));
  QPainter painter(&background);
  painter.drawImage(QPoint{0,0}, image);
  painter.end();

  auto previewWidget = new PreviewWidget(background, this);

  const QPoint pos = m_iconSummary->mapToGlobal(QPoint{0,0});
  previewWidget->move(pos);
  previewWidget->show();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onFontPreviewPressed()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  const auto pixmap = generateTemperatureIconPixmap(m_font);
  if(pixmap.isNull())
  {
    m_validFont = false;
    QApplication::restoreOverrideCursor();

    QMessageBox msgbox(this);
    msgbox.setWindowTitle(tr("Font Selection"));
    msgbox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    msgbox.setIcon(QMessageBox::Critical);
    msgbox.setText(tr("The selected font '%1' is not valid because it cannot draw the needed characters.").arg(m_fontButton->text()));
    msgbox.setBaseSize(400, 400);
    msgbox.exec();
    return;
  }

  auto previewWidget = new PreviewWidget(pixmap, this);
  const QPoint pos = m_fontPreview->mapToGlobal(QPoint{0,0});
  previewWidget->move(pos);

  QApplication::restoreOverrideCursor();

  previewWidget->show();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onFontSelectorPressed()
{
  const QString validChars("0123456789-");

  auto font = m_font;
  font.setPointSize(20);

  QFontDialog dialog(font, this);
  dialog.setOption(QFontDialog::NonScalableFonts, false);
  dialog.setModal(true);
  dialog.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
  dialog.setWindowTitle(tr("Select font for temperature icon"));
  if(QDialog::Accepted != dialog.exec()) return;

  font = dialog.currentFont();
  const auto fontName = font.toString().split(',').first();

  auto showErrorMessage = [&fontName, this]()
  {
    QMessageBox msgbox(this);
    msgbox.setWindowTitle(tr("Font Selection"));
    msgbox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    msgbox.setIcon(QMessageBox::Critical);
    msgbox.setText(tr("The selected font '%1' is not valid because it cannot draw the needed characters.").arg(fontName));
    msgbox.setBaseSize(400, 400);
    msgbox.exec();
  };

  QFontMetrics metrics(font);
  auto it = std::find_if_not(validChars.constBegin(), validChars.constEnd(), [&metrics](const QChar c){ return metrics.inFont(c); });
  if(it != validChars.constEnd() || metrics.boundingRect("01234").width() == 0)
  {
    showErrorMessage();
    return;
  }

  const auto pixmap = generateTemperatureIconPixmap(font);
  if(pixmap.isNull())
  {
    showErrorMessage();
  }
  else
  {
    m_font = font;
    m_validFont = true;
    m_fontPreview->setIcon(QIcon(pixmap));
    m_fontButton->setText(m_font.toString().split(",").first());
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::onIconThemeIndexChanged(int idx)
{
  const auto iconColor = QColor(m_iconThemeColor->property("iconColor").toString());
  auto weatherPix = weatherPixmap("01d", idx, iconColor);

  if(!m_backgroundColor->isChecked())
  {
    const auto backgroundColor = QColor(m_backgroundColorButton->property("iconColor").toString());
    weatherPix = setIconBackground(backgroundColor, weatherPix);
  }

  m_iconSummary->setIcon(QIcon(weatherPix));
  m_iconThemeColor->setEnabled(!ICON_THEMES.at(idx).colored);
}

//--------------------------------------------------------------------
PreviewWidget::PreviewWidget(QPixmap image, QWidget *p)
: QWidget(p, Qt::WindowFlags())
{
  setWindowFlags(Qt::ToolTip|Qt::FramelessWindowHint);
  setLayout(new QVBoxLayout());
  layout()->setMargin(3);
  layout()->setContentsMargins(QMargins{5,5,5,5});

  auto label = new QLabel(this);
  label->setPixmap(image);

  layout()->addWidget(label);
}

//--------------------------------------------------------------------
void PreviewWidget::leaveEvent(QEvent *e)
{
  QWidget::leaveEvent(e);

  close();
  deleteLater();
}

//--------------------------------------------------------------------
void ConfigurationDialog::setCurrentTemperature(const int value)
{
  m_temp = value;

  updateTemperatureIcon();
}

//--------------------------------------------------------------------
void ConfigurationDialog::updateTemperatureIcon()
{
  const auto pixmap = generateTemperatureIconPixmap(m_font);

  m_fontPreview->setIcon(pixmap.isNull() ? QIcon(":/TrayWeather/alert.svg") : QIcon(pixmap));
  m_validFont = !pixmap.isNull();
}

//--------------------------------------------------------------------
void ConfigurationDialog::fixVisuals()
{
  // Fix Visuals layout.
  m_fixed->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
  m_fixed->setMinimumSize(0,0);
  m_variable->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
  m_variable->setMinimumSize(0,0);
  m_from->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
  m_from->setMinimumSize(0,0);

  m_fixed->updateGeometry();
  m_variable->updateGeometry();

  m_fixed->adjustSize();
  m_variable->adjustSize();

  const int visualsFix = std::max(m_fixed->width(), m_variable->width());
  m_fixed->setFixedWidth(visualsFix);
  m_variable->setFixedWidth(visualsFix);
  m_from->setFixedWidth(visualsFix);

  // give tabs a little more room
  m_tabWidget->tabBar()->setUsesScrollButtons(false);
  m_tabWidget->tabBar()->adjustSize();
  setFixedWidth(std::max(width(), m_tabWidget->tabBar()->sizeHint().width() + 30));

  // Re-translates dynamic strings.
  onProviderChanged(m_providerComboBox->currentIndex());
}

//--------------------------------------------------------------------
void ConfigurationDialog::onIconTypeChanged(int value)
{
  auto enableLayout = [](QLayout *lay, const bool enable)
  {
    for (int i = 0; i < lay->count(); ++i)
    {
      auto item = lay->itemAt(i);
      if (item && item->widget())
        item->widget()->setEnabled(enable);
    }
  };

  const auto swapEnabled = value > 2;
  m_swapIcons->setEnabled(swapEnabled);
  m_swapIconsLabel->setEnabled(swapEnabled);

  const auto iconsOptionsEnabled = (value == 0 || value > 1);
  m_iconThemeLabel->setEnabled(iconsOptionsEnabled);
  for(int i = 0; i < m_iconThemeLayout->count(); ++i)
  {
    auto item = m_iconThemeLayout->itemAt(i);
    if(item && item->widget()) item->widget()->setEnabled(iconsOptionsEnabled);
  }

  const auto tempOptionsEnabled = value > 0;
  for(auto lay: {m_tempLayout_1, m_tempLayout_2, m_tempLayout_3, m_tempLayout_4,
                 m_tempLayout_5, m_tempLayout_6, m_tempLayout_7, m_tempLayout_8, 
                 m_tempLayout_9, m_tempLayout10 })
  {
    enableLayout(lay, tempOptionsEnabled);
  }

  enableLayout(m_tempLayout11, tempOptionsEnabled && m_border->isChecked());
  m_borderColorButton->setEnabled(m_borderColor->isEnabled() && !m_borderColor->isChecked());

  m_textColorLabel->setEnabled(tempOptionsEnabled);
  m_textFontLabel->setEnabled(tempOptionsEnabled);
  m_tempIconSizeLabel->setEnabled(tempOptionsEnabled);
  m_borderColorButton->setEnabled(tempOptionsEnabled && !m_borderColor->isChecked() && m_borderColor->isEnabled());

  const auto enableColored = !ICON_THEMES.at(m_trayIconTheme->currentIndex()).colored;
  m_iconThemeColor->setEnabled(enableColored && iconsOptionsEnabled);
  onBorderStateChanged();
  onIconThemeIndexChanged(m_trayIconTheme->currentIndex());
}

//--------------------------------------------------------------------
void ConfigurationDialog::onSearchButtonClicked()
{
  if(m_provider && m_provider->capabilities().requiresKey && !m_testedAPIKey)
  {
    const QString message = tr("Location search requires a valid weather provider API key.");

    QMessageBox msgbox(this);
    msgbox.setWindowTitle(tr("Network Error"));
    msgbox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    msgbox.setText(message);
    msgbox.setBaseSize(400, 400);
    msgbox.exec();
    return;
  }

  LocationFinderDialog dialog(m_provider, m_netManager, this);
  const auto result = dialog.exec();

  if(result == QDialog::Accepted)
  {
    const auto information = dialog.selected();
    if(!information.isValid()) return;

    m_country->setText(information.country);
    m_region->setText(information.region);
    m_city->setText(information.location);
    m_latitude->setText(QString("%1").arg(information.latitude));
    m_longitude->setText(QString("%1").arg(information.longitude));

    auto requestIP = QString("https://api.ipify.org");
    m_netManager->get(QNetworkRequest{QUrl{requestIP}});

    m_useManual->setChecked(true);
    m_latitudeSpin->setValue(information.latitude);
    m_longitudeSpin->setValue(information.longitude);
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::onProviderChanged(int index)
{
  const QString GEOLOCATION_AVAILABLE = tr("Get the coordinates of a location.");
  const QString GEOLOCATION_UNAVAILABLE = tr("Current provider does not have Geo-Location capability.");
  const QString PROVIDER_KEY_TEXT = tr("<html><head/><body><p>To obtain weather forecast data from %1 for your location an " 
                                       "API Key must be obtained from the <a href=\"%2\"><span style=\"text-decoration:" 
                                       "underline; color:#0000ff;\">website</span></a>.</p></body></html>");
  const QString PROVIDER_NO_TEXT = tr("%1 doesn't require any configuration.");

  m_testLabel->clear();

  if(index < 0) index = 0;
  const auto id = WEATHER_PROVIDERS.at(index).id;

  if(!m_provider || m_provider->id().compare(id) != 0)
  {
    disconnectProviderSignals();

    m_config.providerId = id;

    m_provider = WeatherProviderFactory::createProvider(id, m_config);
    connectProviderSignals();
  }

  const auto size = 20; // m_forecastLabel->size().height();
  for(auto &widget: {m_weatherCheck, m_pollutionCheck, m_uvCheck, m_mapsCheck, m_locationCheck, m_alertsCheck })
    widget->setMaximumSize(QSize{size, size});

  const auto yesIcon = QIcon(":/TrayWeather/yes-check.svg").pixmap(QSize(size,size));
  const auto noIcon = QIcon(":/TrayWeather/no-cross.svg").pixmap(QSize(size, size));

  const auto capabilities = m_provider->capabilities();
  m_weatherCheck->setPixmap(capabilities.hasWeatherForecast ? yesIcon : noIcon );
  m_pollutionCheck->setPixmap(capabilities.hasPollutionForecast ? yesIcon : noIcon );
  m_uvCheck->setPixmap(capabilities.hasUVForecast ? yesIcon : noIcon );
  m_mapsCheck->setPixmap(capabilities.hasMaps ? yesIcon : noIcon );
  m_locationCheck->setPixmap(capabilities.hasGeoLocation ? yesIcon : noIcon );
  m_alertsCheck->setPixmap(capabilities.hasAlerts ? yesIcon : noIcon );

  m_geoFind->setEnabled(capabilities.hasGeoLocation);
  m_geoFind->setToolTip(capabilities.hasGeoLocation ? GEOLOCATION_AVAILABLE : GEOLOCATION_UNAVAILABLE );

  m_keyBox->setEnabled(capabilities.requiresKey);
  if(capabilities.requiresKey)
  {
    m_apikey->setText(m_provider->apikey());
    m_ipapiLabel->setEnabled(true);
    m_testLabel->setEnabled(true);
    m_providerConfigText->setTextFormat(Qt::RichText);
    m_providerConfigText->setText(PROVIDER_KEY_TEXT.arg(m_provider->name()).arg(m_provider->website()));
    m_providerConfigText->setOpenExternalLinks(true);
    m_apiTest->setEnabled(true);

    if(!m_provider->apikey().isEmpty())
      requestAPIKeyTest();
  }
  else
  {
    m_apikey->clear();
    m_testLabel->clear();
    m_testLabel->setEnabled(false);
    m_ipapiLabel->setEnabled(false);
    m_apiTest->setEnabled(false);
    m_ipapiLabel->clear();
    m_providerConfigText->setText(PROVIDER_NO_TEXT.arg(m_provider->name()));
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::apiKeyValid(const bool value)
{
  m_apiTest->setEnabled(true);

  if (value)
  {
    m_testedAPIKey = true;
    m_testLabel->setStyleSheet("QLabel { color : green; }");
    m_testLabel->setText(tr("The API Key is valid!"));
    m_geoFind->setEnabled(true);
  }
  else
  {
    m_testLabel->setStyleSheet("QLabel { color : red; }");
    m_testLabel->setText(tr("Invalid API Key!"));
    m_geoFind->setEnabled(false);
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::providerErrorMessage(const QString &msg)
{
  QApplication::restoreOverrideCursor();

  QMessageBox msgbox(this);
  msgbox.setWindowTitle(tr("Weather Provider Error"));
  msgbox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
  msgbox.setText(msg);
  msgbox.setBaseSize(400, 400);
  msgbox.exec();
}

//--------------------------------------------------------------------
QPixmap ConfigurationDialog::generateTemperatureIconPixmap(QFont &font)
{
  auto interpolate = [this](const int temp)
  {
    const auto minVal = m_minSpinBox->value();
    const auto maxVal = m_maxSpinBox->value();
    const auto minColor = QColor(m_minColor->property("iconColor").toString());
    const auto maxColor = QColor(m_maxColor->property("iconColor").toString());
    const auto value = std::min(maxVal, std::max(minVal, temp));

    const double inc = static_cast<double>(value-minVal)/(maxVal - minVal);
    const double rInc = (maxColor.red()   - minColor.red())   * inc;
    const double gInc = (maxColor.green() - minColor.green()) * inc;
    const double bInc = (maxColor.blue()  - minColor.blue())  * inc;

    return QColor::fromRgb(minColor.red() + rInc, minColor.green() + gInc, minColor.blue() + bInc, 255);
  };

  const auto roundedString = QString::number(m_temp) + (m_drawDegree->isChecked() ? QString::fromUtf8("\u00B0") : QString());

  QPixmap pixmap(384,384);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  font.setPixelSize(150);
  font.setLetterSpacing(QFont::AbsoluteSpacing, m_spacingSlider->value());
  painter.setFont(font);

  QColor color;
  if(m_fixed->isChecked())
  {
    color = QColor(m_trayTempColor->property("iconColor").toString());
  }
  else
  {
    color = interpolate(m_temp);
  }

  painter.setRenderHint(QPainter::RenderHint::TextAntialiasing, true);
  painter.setRenderHint(QPainter::RenderHint::HighQualityAntialiasing, true);
  painter.setPen(color);
  painter.drawText(pixmap.rect(), Qt::AlignCenter, roundedString);

  if(m_border->isChecked())
  {
    const auto invertedColor = QColor{color.red() ^ 0xFF, color.green() ^ 0xFF, color.blue() ^ 0xFF};
    const auto selectedColor = QColor(m_borderColorButton->property("iconColor").toString());

    //constructing temporal object only to get path for border.
    const auto pixmapBackup = pixmap;
    QGraphicsPixmapItem tempItem(pixmap);
    tempItem.setShapeMode(QGraphicsPixmapItem::MaskShape);
    const auto path = tempItem.shape();

    QPen pen(m_borderColor->isChecked() ? invertedColor : selectedColor, m_borderWidth->value(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.drawPath(path);
    painter.end();

    subtractPixmap(pixmap, pixmapBackup);

    // repaint the temperature, it was overwritten by path.
    painter.begin(&pixmap);
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, roundedString);
  }
  painter.end();

  const auto rect = computeDrawnRect(pixmap.toImage());
  if(!rect.isValid()) return QPixmap();

  const auto difference = (pixmap.rect().center() - rect.center())/2.;
  double ratioX = pixmap.width() * 1.0 / rect.width();
  double ratioY = pixmap.height() * 1.0 / rect.height();
  ratioX = std::min(ratioX, ratioY);
  ratioY = m_stretch->isChecked() ? ratioY : ratioX;

  const auto ratio = static_cast<double>(m_iconSize->value()) / 100.;
  if(ratio != 1 && ratio >= 0.5)
  {
    ratioX *= ratio;
    ratioY *= ratio;
  }
  
  QPixmap background(m_pixmap);
  painter.begin(&background);
  painter.setRenderHint(QPainter::RenderHint::HighQualityAntialiasing, true);
  if(!m_backgroundColor->isChecked()) painter.fillRect(background.rect(), QColor(m_backgroundColorButton->property("iconColor").toString()));  
  painter.translate(rect.center());
  painter.scale(ratioX, ratioY);
  painter.translate(-rect.center()+difference);
  painter.drawImage(QPoint{0,0}, pixmap.toImage());
  painter.end();

  blurPixmap(background,10);

  return background.scaled(160, 160, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
}

//--------------------------------------------------------------------
void ConfigurationDialog::onTemperatureUnitsChanged()
{
  const auto current = m_unitsComboBox->currentIndex() == 0 ? TemperatureUnits::CELSIUS : (m_unitsComboBox->currentIndex() == 1 ? TemperatureUnits::FAHRENHEIT : static_cast<TemperatureUnits>(m_tempCombo->currentIndex()));

  if (current == lastTemperatureUnits) return;
  lastTemperatureUnits = current;

  // have to reset the limits or it will mess the conversion.
  m_minSpinBox->setMinimum(-200);
  m_minSpinBox->setMaximum(200);
  m_maxSpinBox->setMinimum(-200);
  m_maxSpinBox->setMaximum(200);

  switch(current)
  {
    case TemperatureUnits::CELSIUS:
      m_maxSpinBox->setValue(convertFahrenheitToCelsius(m_maxSpinBox->value()));
      m_minSpinBox->setValue(convertFahrenheitToCelsius(m_minSpinBox->value()));
      m_temp = convertFahrenheitToCelsius(m_temp);
      m_minSpinBox->setMinimum(convertFahrenheitToCelsius(m_minSpinBox->minimum()));
      m_minSpinBox->setMaximum(convertFahrenheitToCelsius(m_minSpinBox->maximum()));
      m_maxSpinBox->setMinimum(convertFahrenheitToCelsius(m_maxSpinBox->minimum()));
      m_maxSpinBox->setMaximum(convertFahrenheitToCelsius(m_maxSpinBox->maximum()));
      break;
    case TemperatureUnits::FAHRENHEIT:
      m_maxSpinBox->setValue(convertCelsiusToFahrenheit(m_maxSpinBox->value()));
      m_minSpinBox->setValue(convertCelsiusToFahrenheit(m_minSpinBox->value()));        
      m_temp = convertCelsiusToFahrenheit(m_temp);
      m_minSpinBox->setMinimum(convertCelsiusToFahrenheit(m_minSpinBox->minimum()));
      m_minSpinBox->setMaximum(convertCelsiusToFahrenheit(m_minSpinBox->maximum()));
      m_maxSpinBox->setMinimum(convertCelsiusToFahrenheit(m_maxSpinBox->minimum()));
      m_maxSpinBox->setMaximum(convertCelsiusToFahrenheit(m_maxSpinBox->maximum()));
      break;
  }

  updateTemperatureIcon();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onBorderStateChanged()
{
  const auto enable = m_border->isEnabled() && m_border->isChecked();
  m_borderWidthLabel->setEnabled(enable);
  m_borderWidth->setEnabled(enable);
  m_borderSpinBox->setEnabled(enable);
  m_borderColor->setEnabled(enable);
  m_borderColorLabel->setEnabled(enable);
  m_borderColorButton->setEnabled(enable && !m_borderColor->isChecked());
}

//--------------------------------------------------------------------
void ConfigurationDialog::onBarWidthSliderChanged(int value)
{
  m_barWidthSpinbox->setUpdatesEnabled(false);
  m_barWidthSpinbox->setValue(value/10.);
  m_barWidthSpinbox->setUpdatesEnabled(true);
}

//--------------------------------------------------------------------
void ConfigurationDialog::onBarWidthSpinboxChanged(double value)
{
  m_barWidthSlider->setUpdatesEnabled(false);
  m_barWidthSlider->setValue(value*10);
  m_barWidthSlider->setUpdatesEnabled(true);
}

//--------------------------------------------------------------------
void ConfigurationDialog::updateIconBackgroundColor()
{
  updateTemperatureIcon();
  onIconThemeIndexChanged(m_trayIconTheme->currentIndex());
}

//--------------------------------------------------------------------
void ConfigurationDialog::refreshNetworkLog()
{
  if(m_tabWidget->currentIndex() == m_tabWidget->count() - 1)
  {
    auto widget = m_tabWidget->widget(m_tabWidget->count() - 1);
    auto textWidget = qobject_cast<QPlainTextEdit *>(widget->layout()->itemAt(0)->widget());
    if(textWidget)
    {
      textWidget->clear();
      textWidget->setPlainText(REQUESTS_BUFFER);
    }
  }
}
