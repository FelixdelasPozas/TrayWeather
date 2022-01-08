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
#include <Utils.h>

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

// C++
#include <chrono>

const char SELECTED[] = "Selected";

//--------------------------------------------------------------------
ConfigurationDialog::ConfigurationDialog(const Configuration &configuration, QWidget* parent, Qt::WindowFlags flags)
: QDialog       {parent}
, m_netManager  {std::make_shared<QNetworkAccessManager>(this)}
, m_testedAPIKey{false}
{
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

  setupUi(this);

  m_tooltipList->setItemDelegate(new RichTextItemDelegate());
  m_tooltipValueCombo->setItemDelegate(new RichTextItemDelegate());

  setConfiguration(configuration);
}

//--------------------------------------------------------------------
void ConfigurationDialog::replyFinished(QNetworkReply* reply)
{
  QString details, message;

  auto url = reply->url();

  if(url.toString().contains("edns.ip", Qt::CaseInsensitive))
  {
    if(url.toString().contains(m_DNSIP, Qt::CaseSensitive))
    {
      if(reply->error() == QNetworkReply::NetworkError::NoError)
      {
        auto data = reply->readAll();

        m_DNSIP = data.split(' ').at(1);
        m_DNSIP = m_DNSIP.remove('\n');
        requestGeolocation();
      }
      else
      {
        message = tr("Invalid reply from Geo-Locator server.\nCouldn't get location information.\nIf you have a firewall change the configuration to allow this program to access the network.");
        details = reply->errorString();

        QMessageBox box(this);
        box.setWindowTitle(tr("Network Error"));
        box.setWindowIcon(QIcon(":/TrayWeather/application.ico"));
        box.setDetailedText(details);
        box.setText(message);
        box.setBaseSize(400, 400);
        box.exec();
      }
    }

    reply->deleteLater();

    return;
  }

  if(!url.toString().startsWith("http://ip-api.com/csv", Qt::CaseSensitive))
  {
    m_apiTest->setEnabled(true);

    if(reply->error() == QNetworkReply::NetworkError::NoError)
    {
      auto type = reply->header(QNetworkRequest::ContentTypeHeader);
      if(type.toString().startsWith("application/json"))
      {
        const auto data = reply->readAll();
        auto jsonDocument = QJsonDocument::fromJson(data);

        if(!jsonDocument.isNull() && jsonDocument.isObject() && jsonDocument.object().contains("cod"))
        {
          auto jsonObj = jsonDocument.object();
          auto code    = jsonObj.value("cod").toInt(0);

          if(code != 401)
          {
            reply->deleteLater();

            m_testedAPIKey = true;
            m_testLabel->setStyleSheet("QLabel { color : green; }");
            m_testLabel->setText(tr("The API Key is valid!"));
            return;
          }
        }
      }

      m_testLabel->setStyleSheet("QLabel { color : red; }");
      m_testLabel->setText(tr("Invalid OpenWeatherMap API Key!"));

      message = tr("Invalid OpenWeatherMap API Key.");
    }
    else
    {
      m_testLabel->setStyleSheet("QLabel { color : red; }");
      m_testLabel->setText(tr("Invalid OpenWeatherMap API Key!"));

      message = tr("Invalid reply from OpenWeatherMap server.");
      details = reply->errorString();
    }
  }
  else
  {
    m_geoRequest->setEnabled(true);

    if(reply->error() == QNetworkReply::NetworkError::NoError)
    {
      message = tr("Invalid reply from Geo-Locator server.\nCouldn't get location information.\nIf you have a firewall change the configuration to allow this program to access the network.");

      auto type = reply->header(QNetworkRequest::ContentTypeHeader);
      if(type.toString().startsWith("text/plain", Qt::CaseInsensitive))
      {
        auto data = QString::fromUtf8(reply->readAll());
        const auto values = parseCSV(data);

        if((values.first().compare("success", Qt::CaseInsensitive) == 0) && (values.size() == 14))
        {
          m_country->setText(values.at(1));
          m_region->setText(values.at(4));
          m_city->setText(values.at(5));
          m_zipCode->setText(values.at(6));
          m_latitude->setText(values.at(7));
          m_longitude->setText(values.at(8));
          m_timezone->setText(values.at(9));
          m_isp->setText(values.at(10));
          m_ip->setText(values.at(13));

          m_ipapiLabel->setStyleSheet("QLabel { color : green; }");
          m_ipapiLabel->setText(tr("Success"));
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

  QMessageBox box(this);
  box.setWindowTitle(tr("Network Error"));
  box.setWindowIcon(QIcon(":/TrayWeather/application.ico"));
  box.setDetailedText(details);
  box.setText(message);
  box.setBaseSize(400, 400);
  box.exec();

  reply->deleteLater();
}

//--------------------------------------------------------------------
void ConfigurationDialog::getConfiguration(Configuration &configuration) const
{
  configuration.city           = m_city->text();
  configuration.country        = m_country->text();
  configuration.ip             = m_ip->text();
  configuration.isp            = m_isp->text();
  configuration.owm_apikey     = m_testedAPIKey ? m_apikey->text() : QString();
  configuration.region         = m_region->text();
  configuration.timezone       = m_timezone->text();
  configuration.zipcode        = m_zipCode->text();
  configuration.updateTime     = m_updateTime->value();
  configuration.units          = static_cast<Units>(m_unitsComboBox->currentIndex());
  configuration.useDNS         = m_useDNS->isChecked();
  configuration.useGeolocation = m_useGeolocation->isChecked();
  configuration.roamingEnabled = m_roamingCheck->isChecked();
  configuration.lightTheme     = m_theme->currentIndex() == 0;
  configuration.iconType       = static_cast<unsigned int>(m_trayIconType->currentIndex());
  configuration.iconTheme      = static_cast<unsigned int>(m_trayIconTheme->currentIndex());
  configuration.iconThemeColor = QColor(m_iconThemeColor->property("iconColor").toString());
  configuration.trayTextColor  = QColor(m_trayTempColor->property("iconColor").toString());
  configuration.trayTextMode   = m_fixed->isChecked();
  configuration.minimumColor   = QColor(m_minColor->property("iconColor").toString());
  configuration.maximumColor   = QColor(m_maxColor->property("iconColor").toString());
  configuration.minimumValue   = m_minSpinBox->value();
  configuration.maximumValue   = m_maxSpinBox->value();
  configuration.update         = static_cast<Update>(m_updatesCombo->currentIndex());
  configuration.autostart      = m_autostart->isChecked();
  configuration.language       = m_languageCombo->itemData(m_languageCombo->currentIndex(), Qt::UserRole).toString();
  configuration.tempUnits      = static_cast<TemperatureUnits>(m_tempCombo->currentIndex());
  configuration.pressureUnits  = static_cast<PressureUnits>(m_pressionCombo->currentIndex());
  configuration.precUnits      = static_cast<PrecipitationUnits>(m_precipitationCombo->currentIndex());
  configuration.windUnits      = static_cast<WindUnits>(m_windCombo->currentIndex());
  configuration.graphUseRain   = m_rainGraph->isChecked();
  configuration.showAlerts     = m_showAlerts->isChecked();

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
void ConfigurationDialog::requestOpenWeatherMapAPIKeyTest() const
{
  // protect from abuse
  static auto lastRequest = std::chrono::steady_clock::now();
  const auto now = std::chrono::steady_clock::now();
  const std::chrono::duration<double> interval = now - lastRequest;
  const auto duration = interval.count();

  if(duration > 0.1 && duration < 2.5) return;
  lastRequest = now;

  auto url = QUrl{QString("http://api.openweathermap.org/data/2.5/weather?lat=%1&lon=%2&appid=%3").arg(m_latitude->text())
                                                                                                  .arg(m_longitude->text())
                                                                                                  .arg(m_apikey->text())};
  m_netManager->get(QNetworkRequest{url});

  m_apiTest->setEnabled(false);
  m_testLabel->setStyleSheet("QLabel { color : blue; }");
  m_testLabel->setText(tr("Testing API Key..."));
}

//--------------------------------------------------------------------
void ConfigurationDialog::onDNSRequestStateChanged(int state)
{
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
          this,      SLOT(requestOpenWeatherMapAPIKeyTest()));

  connect(m_geoRequest, SIGNAL(pressed()),
          this,         SLOT(requestGeolocation()));

  connect(m_useDNS, SIGNAL(stateChanged(int)),
          this,     SLOT(onDNSRequestStateChanged(int)));

  connect(m_useGeolocation, SIGNAL(toggled(bool)),
          this,             SLOT(onRadioChanged()));

  connect(m_useManual, SIGNAL(toggled(bool)),
          this,        SLOT(onRadioChanged()));

  connect(m_longitudeSpin, SIGNAL(editingFinished()),
          this,            SLOT(onCoordinatesChanged()));

  connect(m_latitudeSpin, SIGNAL(editingFinished()),
          this,           SLOT(onCoordinatesChanged()));

  connect(m_theme, SIGNAL(currentIndexChanged(int)),
         this,     SLOT(onThemeIndexChanged(int)));

  connect(m_trayTempColor, SIGNAL(clicked()),
          this,            SLOT(onColorButtonClicked()));

  connect(m_minColor, SIGNAL(clicked()),
          this,       SLOT(onColorButtonClicked()));

  connect(m_maxColor, SIGNAL(clicked()),
          this,       SLOT(onColorButtonClicked()));

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
          this,          SLOT(onIconSummaryPressed()));

  connect(m_trayIconTheme, SIGNAL(currentIndexChanged(int)),
          this,            SLOT(onIconThemeIndexChanged(int)));

  connect(m_iconThemeColor, SIGNAL(pressed()),
          this,             SLOT(onColorButtonClicked()));

  for(auto &w: {m_tempCombo, m_pressionCombo, m_windCombo, m_precipitationCombo})
  {
    connect(w, SIGNAL(currentIndexChanged(int)), this, SLOT(onUnitComboChanged(int)));
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::onRadioChanged()
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

  // First tab is the larger one, so we need Qt to resize the dialog
  // according to its contents...
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
  m_tabWidget->setCurrentIndex(3);

  QApplication::restoreOverrideCursor();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onColorButtonClicked()
{
  auto button = qobject_cast<QToolButton *>(sender());
  auto color  = QColor(button->property("iconColor").toString());

  QColorDialog dialog;
  dialog.setCurrentColor(color);
  dialog.setWindowIcon(QIcon(":/TrayWeather/application.ico"));

  if(dialog.exec() != QColorDialog::Accepted) return;

  QPixmap icon(QSize(64,64));
  icon.fill(dialog.selectedColor());
  button->setIcon(QIcon(icon));
  button->setProperty("iconColor", dialog.selectedColor().name(QColor::HexArgb));

  if(button == m_minColor || button == m_maxColor)
  {
    updateRange();
  }
  else
  {
    if(button == m_iconThemeColor)
    {
      onIconThemeIndexChanged(m_trayIconTheme->currentIndex());
    }
  }
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
  this->m_tabWidget->setCurrentIndex(0);

  QDialog::showEvent(e);

  scaleDialog(this);
  fixVisuals();
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
  const QString APPLICATION_NAME{"TrayWeather"};
  QSettings settings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
  bool updated = false;

  if(value == Qt::Checked)
  {
    if(!settings.allKeys().contains(APPLICATION_NAME, Qt::CaseInsensitive))
    {
      settings.setValue(APPLICATION_NAME, QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
      updated = true;
    }
  }
  else
  {
    if(settings.allKeys().contains(APPLICATION_NAME, Qt::CaseInsensitive))
    {
      settings.remove(APPLICATION_NAME);
      updated = true;
    }
  }

  if(updated)
  {
    settings.sync();
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::setConfiguration(const Configuration &configuration)
{
  m_minSpinBox->setValue(configuration.minimumValue);
  m_maxSpinBox->setValue(configuration.maximumValue);
  m_autostart->setChecked(configuration.autostart);

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

  m_useManual->setChecked(!configuration.useGeolocation);
  m_useGeolocation->setChecked(configuration.useGeolocation);
  m_city->setText(configuration.city);
  m_country->setText(configuration.country);
  m_ip->setText(configuration.ip);
  m_isp->setText(configuration.isp);
  m_region->setText(configuration.region);
  m_timezone->setText(configuration.timezone);
  m_zipCode->setText(configuration.zipcode);
  m_updateTime->setValue(configuration.updateTime);
  m_useDNS->setChecked(configuration.useDNS);
  m_roamingCheck->setChecked(configuration.roamingEnabled);
  m_apikey->setText(configuration.owm_apikey);
  m_theme->setCurrentIndex(configuration.lightTheme ? 0 : 1);
  m_trayIconTheme->setCurrentIndex(static_cast<int>(configuration.iconTheme));

  m_trayIconType->setCurrentIndex(static_cast<int>(configuration.iconType));
  m_updatesCombo->setCurrentIndex(static_cast<int>(configuration.update));

  m_unitsComboBox->setCurrentIndex(static_cast<int>(configuration.units));
  m_tempCombo->setCurrentIndex(static_cast<int>(configuration.tempUnits));
  m_pressionCombo->setCurrentIndex(static_cast<int>(configuration.pressureUnits));
  m_precipitationCombo->setCurrentIndex(static_cast<int>(configuration.precUnits));
  m_windCombo->setCurrentIndex(static_cast<int>(configuration.windUnits));

  m_fixed->setChecked(configuration.trayTextMode);
  m_variable->setChecked(!configuration.trayTextMode);

  m_minSpinBox->setMaximum(configuration.maximumValue-1);
  m_maxSpinBox->setMinimum(configuration.minimumValue+1);

  m_tooltipList->clear();
  m_tooltipList->setAlternatingRowColors(true);

  m_rainGraph->setChecked(configuration.graphUseRain);
  m_snowGraph->setChecked(!configuration.graphUseRain);

  m_showAlerts->setChecked(configuration.showAlerts);

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

  if(!configuration.isValid())
  {
    m_longitudeSpin->setValue(0.);
    m_latitudeSpin->setValue(0.);

    m_latitude->setText("0");
    m_longitude->setText("0");

    m_updateTime->setValue(15);
    m_unitsComboBox->setCurrentIndex(0);
  }
  else
  {
    m_longitudeSpin->setValue(configuration.longitude);
    m_latitudeSpin->setValue(configuration.latitude);

    m_latitude->setText(QString::number(configuration.latitude));
    m_longitude->setText(QString::number(configuration.longitude));
  }

  if(configuration.useGeolocation || !m_ipapiLabel->text().isEmpty())
  {
    requestGeolocation();
  }

  onRadioChanged();
  onCoordinatesChanged();

  if(configuration.isValid())
  {
    requestOpenWeatherMapAPIKeyTest();
  }
  else
  {
    m_apiTest->setEnabled(true);

    if(configuration.owm_apikey.isEmpty())
    {
      m_testLabel->setStyleSheet("QLabel { color : red; }");
      m_testLabel->setText(tr("Invalid OpenWeatherMap API Key!"));
    }
    else
    {
      m_testLabel->setStyleSheet("QLabel { color : red; }");
      m_testLabel->setText(tr("Untested OpenWeatherMap API Key!"));
    }
  }

  m_tempCombo->setProperty(SELECTED, static_cast<int>(configuration.tempUnits));
  m_pressionCombo->setProperty(SELECTED, static_cast<int>(configuration.pressureUnits));
  m_windCombo->setProperty(SELECTED, static_cast<int>(configuration.windUnits));
  m_precipitationCombo->setProperty(SELECTED, static_cast<int>(configuration.precUnits));

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
             this,      SLOT(requestOpenWeatherMapAPIKeyTest()));

  disconnect(m_geoRequest, SIGNAL(pressed()),
             this,         SLOT(requestGeolocation()));

  disconnect(m_useDNS, SIGNAL(stateChanged(int)),
             this,     SLOT(onDNSRequestStateChanged(int)));

  disconnect(m_useGeolocation, SIGNAL(toggled(bool)),
             this,             SLOT(onRadioChanged()));

  disconnect(m_useManual, SIGNAL(toggled(bool)),
             this,        SLOT(onRadioChanged()));

  disconnect(m_longitudeSpin, SIGNAL(editingFinished()),
             this,            SLOT(onCoordinatesChanged()));

  disconnect(m_latitudeSpin, SIGNAL(editingFinished()),
             this,           SLOT(onCoordinatesChanged()));

  disconnect(m_theme, SIGNAL(currentIndexChanged(int)),
             this,     SLOT(onThemeIndexChanged(int)));

  disconnect(m_trayTempColor, SIGNAL(clicked()),
             this,            SLOT(onColorButtonClicked()));

  disconnect(m_minColor, SIGNAL(clicked()),
             this,       SLOT(onColorButtonClicked()));

  disconnect(m_maxColor, SIGNAL(clicked()),
             this,       SLOT(onColorButtonClicked()));

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
             this,          SLOT(onIconSummaryPressed()));

  disconnect(m_trayIconTheme, SIGNAL(currentIndexChanged(int)),
             this,            SLOT(onIconThemeIndexChanged(int)));

  disconnect(m_iconThemeColor, SIGNAL(pressed()),
             this,             SLOT(onColorButtonClicked()));

  for(auto &w: {m_tempCombo, m_pressionCombo, m_windCombo, m_precipitationCombo})
  {
    disconnect(w, SIGNAL(currentIndexChanged(int)), this, SLOT(onUnitComboChanged(int)));
  }
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
  for(int i = 0; i < TRANSLATIONS.size(); ++i)
  {
    const auto &lang = TRANSLATIONS.at(i);
    const auto translated = QApplication::translate("QObject", lang.name.toUtf8());
    m_languageCombo->addItem(QIcon(lang.icon), translated, lang.file);
    if(lang.file.compare(current) == 0) selected = i;
  }
  m_languageCombo->blockSignals(true);
  m_languageCombo->setCurrentIndex(selected);
  m_languageCombo->blockSignals(false);
}

//--------------------------------------------------------------------
void ConfigurationDialog::onLanguageChanged(int index)
{
  auto idx = std::max(std::min(index, TRANSLATIONS.size() - 1), 0);

  emit languageChanged(TRANSLATIONS.at(idx).file);
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
}

//--------------------------------------------------------------------
void ConfigurationDialog::onUnitComboChanged(int index)
{
  auto w = qobject_cast<QComboBox*>(sender());
  if(w)
  {
    w->setProperty(SELECTED, index);
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
void ConfigurationDialog::onIconSummaryPressed()
{
  const auto color = QColor(m_iconThemeColor->property("iconColor").toString());
  const auto image = createIconsSummary(m_trayIconTheme->currentIndex(), 32, color);
  auto summaryWidget = new IconSummaryWidget(image, this);

  const QPoint pos = m_iconSummary->mapToGlobal(QPoint{0,0});
  summaryWidget->move(pos);
  summaryWidget->show();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onIconThemeIndexChanged(int idx)
{
  const auto iconColor = QColor(m_iconThemeColor->property("iconColor").toString());
  m_iconSummary->setIcon(QIcon(weatherPixmap("01d", idx, iconColor)));
  m_iconThemeColor->setEnabled(!ICON_THEMES.at(idx).colored);
}

//--------------------------------------------------------------------
IconSummaryWidget::IconSummaryWidget(QPixmap image, QWidget *p)
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
void IconSummaryWidget::leaveEvent(QEvent *e)
{
  QWidget::leaveEvent(e);

  close();
  deleteLater();
}

//--------------------------------------------------------------------
void ConfigurationDialog::fixVisuals()
{
  // Fix Visuals layout.
  const int visualsFix = std::max(m_fixed->width(), m_variable->width());
  m_fixed->setFixedWidth(visualsFix);
  m_variable->setFixedWidth(visualsFix);
  m_from->setFixedWidth(visualsFix);
}
