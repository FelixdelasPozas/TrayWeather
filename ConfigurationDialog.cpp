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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>
#include <QIcon>
#include <QDomNode>
#include <QDomNodeList>
#include <QDomDocument>
#include <QString>
#include <QDebug>

//--------------------------------------------------------------------
ConfigurationDialog::ConfigurationDialog(const Configuration &configuration, QWidget* parent, Qt::WindowFlags flags)
: QDialog     {parent}
, m_netManager{std::make_shared<QNetworkAccessManager>(this)}
{
  setupUi(this);

  if(!configuration.isValid())
  {
    m_longitude->setText(tr("Requesting IP Coordinates..."));
  }
  else
  {
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

    auto url = QUrl{QString(tr("http://api.openweathermap.org/data/2.5/weather?lat=%1&lon=%2&appid=%3").arg(configuration.latitude)
                                                                                                       .arg(configuration.longitude)
                                                                                                       .arg(configuration.owm_apikey))};
    m_netManager->get(QNetworkRequest{url});
  }

  connect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
          this,               SLOT(replyFinished(QNetworkReply*)));

  // CSV is easier to parse later.
  m_netManager->get(QNetworkRequest{QUrl{"http://ip-api.com/csv"}});

  m_request->setEnabled(false);
  m_request->setText(tr("Waiting"));
}

//--------------------------------------------------------------------
void ConfigurationDialog::replyFinished(QNetworkReply* reply)
{
  QString details, message;

  m_request->setEnabled(true);
  m_request->setText(tr("Request"));

  if(reply->url() != QUrl{"http://ip-api.com/csv"})
  {
    auto type = reply->header(QNetworkRequest::ContentTypeHeader);
    if(type.toString().startsWith("application/json")) return;

    message = tr("Invalid OpenWeatherMap API Key.");
  }
  else
  {
    if(reply->error() == QNetworkReply::NoError)
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

          return;
        }
        else
        {
          details = tr("Error parsing location data. Failure or invalid number of fields.");
        }
      }
      else
      {
        details = tr("Data request failure. Invalid data format.");
      }
    }
    else
    {
      details = tr("%1").arg(reply->errorString());
    }
  }


  auto box = std::make_shared<QMessageBox>(this);
  box->setWindowTitle(tr("Application error"));
  box->setWindowIcon(QIcon(":/TrayWeather/application.ico"));
  box->setDetailedText(tr("Error description: %1").arg(reply->errorString()));
  box->setText(message);

  box->exec();
}

//--------------------------------------------------------------------
void ConfigurationDialog::getConfiguration(Configuration &configuration) const
{
  configuration.city       = m_city->text();
  configuration.country    = m_country->text();
  configuration.ip         = m_ip->text();
  configuration.isp        = m_isp->text();
  configuration.latitude   = m_latitude->text().toDouble();
  configuration.longitude  = m_longitude->text().toDouble();
  configuration.owm_apikey = m_apikey->text();
  configuration.region     = m_region->text();
  configuration.timezone   = m_timezone->text();
  configuration.zipcode    = m_zipCode->text();
}
