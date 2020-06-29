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

//--------------------------------------------------------------------
ConfigurationDialog::ConfigurationDialog(const Configuration &configuration, QWidget* parent, Qt::WindowFlags flags)
: QDialog       {parent}
, m_netManager  {std::make_shared<QNetworkAccessManager>(this)}
, m_testedAPIKey{false}
{
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setupUi(this);

  m_minSpinBox->setValue(configuration.minimumValue);
  m_maxSpinBox->setValue(configuration.maximumValue);

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
  m_tempComboBox->setCurrentIndex(static_cast<int>(configuration.units));
  m_useDNS->setChecked(configuration.useDNS);
  m_roamingCheck->setChecked(configuration.roamingEnabled);
  m_apikey->setText(configuration.owm_apikey);
  m_theme->setCurrentIndex(configuration.lightTheme ? 0 : 1);
  m_trayIconType->setCurrentIndex(static_cast<int>(configuration.iconType));

  m_fixed->setChecked(configuration.trayTextMode);
  m_variable->setChecked(!configuration.trayTextMode);

  m_minSpinBox->setMaximum(configuration.maximumValue-1);
  m_maxSpinBox->setMinimum(configuration.minimumValue+1);

  QPixmap icon(QSize(64,64));
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
    m_tempComboBox->setCurrentIndex(0);
  }
  else
  {
    m_longitudeSpin->setValue(configuration.longitude);
    m_latitudeSpin->setValue(configuration.latitude);

    m_latitude->setText(QString::number(configuration.latitude));
    m_longitude->setText(QString::number(configuration.longitude));
  }

  if(configuration.useGeolocation)
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

  setFixedSize(size());
  updateRange();
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
        message = tr("Invalid reply from Geo-Locator server.\n.Couldn't get location information.\nIf you have a firewall change the configuration to allow this program to access the network.");
        details = tr("%1").arg(reply->errorString());

        auto box = std::make_shared<QMessageBox>(this);
        box->setWindowTitle(tr("Network Error"));
        box->setWindowIcon(QIcon(":/TrayWeather/application.ico"));
        box->setDetailedText(details);
        box->setText(message);
        box->setBaseSize(400, 400);
        box->exec();
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
      details = tr("%1").arg(reply->errorString());
    }
  }
  else
  {
    m_geoRequest->setEnabled(true);

    if(reply->error() == QNetworkReply::NetworkError::NoError)
    {
      message = tr("Invalid reply from Geo-Locator server.\n.Couldn't get location information.\nIf you have a firewall change the configuration to allow this program to access the network.");

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
      details = tr("%1").arg(reply->errorString());
    }
  }

  auto box = std::make_shared<QMessageBox>(this);
  box->setWindowTitle(tr("Network Error"));
  box->setWindowIcon(QIcon(":/TrayWeather/application.ico"));
  box->setDetailedText(details);
  box->setText(message);
  box->setBaseSize(400, 400);

  box->exec();

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
  configuration.units          = static_cast<Temperature>(m_tempComboBox->currentIndex());
  configuration.useDNS         = m_useDNS->isChecked();
  configuration.useGeolocation = m_useGeolocation->isChecked();
  configuration.roamingEnabled = m_roamingCheck->isChecked();
  configuration.lightTheme     = m_theme->currentIndex() == 0;
  configuration.iconType       = static_cast<unsigned int>(m_trayIconType->currentIndex());
  configuration.trayTextColor  = QColor(m_trayTempColor->property("iconColor").toString());
  configuration.trayTextMode   = m_fixed->isChecked();
  configuration.minimumColor   = QColor(m_minColor->property("iconColor").toString());
  configuration.maximumColor   = QColor(m_maxColor->property("iconColor").toString());
  configuration.minimumValue   = m_minSpinBox->value();
  configuration.maximumValue   = m_maxSpinBox->value();

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
  auto url = QUrl{QString(tr("http://api.openweathermap.org/data/2.5/weather?lat=%1&lon=%2&appid=%3").arg(m_latitude->text())
                                                                                                     .arg(m_longitude->text())
                                                                                                     .arg(m_apikey->text()))};
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
  setFixedSize(size());
  updateRange();

  QApplication::restoreOverrideCursor();
}

//--------------------------------------------------------------------
void ConfigurationDialog::onColorButtonClicked()
{
  auto button = qobject_cast<QToolButton *>(sender());
  auto color  = QColor(button->property("iconColor").toString());

  QColorDialog dialog;
  dialog.setCurrentColor(color);

  if(dialog.exec() != QColorDialog::Accepted) return;

  QPixmap icon(QSize(64,64));
  icon.fill(dialog.selectedColor());
  button->setIcon(QIcon(icon));
  button->setProperty("iconColor", dialog.selectedColor().name(QColor::HexArgb));

  if(button == m_minColor || button == m_maxColor)
  {
    updateRange();
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
