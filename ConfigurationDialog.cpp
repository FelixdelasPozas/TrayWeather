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

// Qt
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QIcon>
#include <QString>

//--------------------------------------------------------------------
ConfigurationDialog::ConfigurationDialog(const Configuration &configuration, QWidget* parent, Qt::WindowFlags flags)
: QDialog       {parent}
, m_netManager  {std::make_shared<QNetworkAccessManager>(this)}
, m_testedAPIKey{false}
{
  setupUi(this);

  connect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
          this,               SLOT(replyFinished(QNetworkReply*)));

  connect(m_apiTest, SIGNAL(pressed()),
          this,      SLOT(requestOpenWeatherMapAPIKeyTest()));

  connect(m_request, SIGNAL(pressed()),
          this,      SLOT(requestIPGeolocation()));

  if(configuration.isValid())
  {
    m_geoipLabel->setStyleSheet("QLabel { color : green; }");
    m_geoipLabel->setText(tr("IP Geolocation successful."));
    m_request->setEnabled(true);

    m_city->setText(configuration.city);
    m_country->setText(configuration.country);
    m_ip->setText(configuration.ip);
    m_isp->setText(configuration.isp);
    m_latitude->setText(QString::number(configuration.latitude));
    m_longitude->setText(QString::number(configuration.longitude));
    m_apikey->setText(configuration.owm_apikey);
    m_region->setText(configuration.region);
    m_timezone->setText(configuration.timezone);
    m_zipCode->setText(configuration.zipcode);
    m_updateTime->setValue(configuration.updateTime);
    m_tempComboBox->setCurrentIndex(static_cast<int>(configuration.units));

    requestOpenWeatherMapAPIKeyTest();
  }
  else
  {
    m_updateTime->setValue(15);
    m_tempComboBox->setCurrentIndex(0);

    requestIPGeolocation();
  }
}

//--------------------------------------------------------------------
void ConfigurationDialog::replyFinished(QNetworkReply* reply)
{
  QString details, message;

  if(reply->url() != QUrl{"http://ip-api.com/csv"})
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

      message = tr("Network error.");
      details = tr("%1").arg(reply->errorString());
    }
  }
  else
  {
    m_request->setEnabled(true);

    if(reply->error() == QNetworkReply::NetworkError::NoError)
    {
      message = tr("Couldn't get location information.\nIf you have a firewall change the configuration to allow this program to access the network.");

      auto type = reply->header(QNetworkRequest::ContentTypeHeader);
      if(type.toString().startsWith("text/plain", Qt::CaseInsensitive))
      {
        const auto data = QString::fromUtf8(reply->readAll());
        const auto values = data.split(',', QString::SplitBehavior::KeepEmptyParts, Qt::CaseInsensitive);

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

          m_geoipLabel->setStyleSheet("QLabel { color : green; }");
          m_geoipLabel->setText(tr("IP Geolocation successful."));
          reply->deleteLater();

          if(!m_testedAPIKey) requestOpenWeatherMapAPIKeyTest();

          return;
        }
        else
        {
          m_geoipLabel->setStyleSheet("QLabel { color : red; }");
          m_geoipLabel->setText(tr("IP Geolocation unsuccessful."));

          details = tr("Error parsing location data. Failure or invalid number of fields.");
        }
      }
      else
      {
        m_geoipLabel->setStyleSheet("QLabel { color : red; }");
        m_geoipLabel->setText(tr("IP Geolocation unsuccessful."));

        details = tr("Data request failure. Invalid data format.");
      }
    }
    else
    {
      m_geoipLabel->setStyleSheet("QLabel { color : red; }");
      m_geoipLabel->setText(tr("IP Geolocation unsuccessful."));

      details = tr("%1").arg(reply->errorString());
    }
  }


  auto box = std::make_shared<QMessageBox>(this);
  box->setWindowTitle(tr("Application error"));
  box->setWindowIcon(QIcon(":/TrayWeather/application.ico"));
  box->setDetailedText(tr("Error description: %1").arg(reply->errorString()));
  box->setText(message);

  box->exec();

  reply->deleteLater();
}

//--------------------------------------------------------------------
void ConfigurationDialog::getConfiguration(Configuration &configuration) const
{
  configuration.city       = m_city->text();
  configuration.country    = m_country->text();
  configuration.ip         = m_ip->text();
  configuration.isp        = m_isp->text();
  configuration.latitude   = !m_latitude->text().isEmpty() ? m_latitude->text().toDouble() : -181.0;
  configuration.longitude  = !m_longitude->text().isEmpty() ? m_longitude->text().toDouble() : -181.0;
  configuration.owm_apikey = m_testedAPIKey ? m_apikey->text() : QString();
  configuration.region     = m_region->text();
  configuration.timezone   = m_timezone->text();
  configuration.zipcode    = m_zipCode->text();
  configuration.updateTime = m_updateTime->value();
  configuration.units      = static_cast<Temperature>(m_tempComboBox->currentIndex());
}

//--------------------------------------------------------------------
void ConfigurationDialog::requestIPGeolocation() const
{
  // CSV is easier to parse later.
  m_netManager->get(QNetworkRequest{QUrl{"http://ip-api.com/csv"}});

  m_request->setEnabled(false);
  m_geoipLabel->setStyleSheet("QLabel { color : blue; }");
  m_geoipLabel->setText(tr("Requesting IP Geolocation..."));
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
