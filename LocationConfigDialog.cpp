/*
 File: LocationConfigDialog.cpp
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
#include <LocationConfigDialog.h>

// Qt
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>
#include <QIcon>
#include <QDomNode>
#include <QDomNodeList>
#include <QDomDocument>
#include <QDebug>

//--------------------------------------------------------------------
LocationConfigDialog::LocationConfigDialog(QWidget* parent, Qt::WindowFlags flags)
: m_netManager{std::make_shared<QNetworkAccessManager>(this)}
{
  setupUi(this);

  connect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
          this,               SLOT(replyFinished(QNetworkReply*)));

  // CSV is easier to parse later.
  m_netManager->get(QNetworkRequest{QUrl{"http://ip-api.com/csv"}});
}

//--------------------------------------------------------------------
void LocationConfigDialog::replyFinished(QNetworkReply* reply)
{
  QString message = tr("Couldn't get location information.\nIf you have a firewall change the configuration to allow this program to access the network.");
  QString details;

  if(reply->error() == QNetworkReply::NoError)
  {
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

  auto box = std::make_shared<QMessageBox>(this);
  box->setWindowTitle(tr("Network error"));
  box->setWindowIcon(QIcon(":/TrayWeather/application.ico"));
  box->setDetailedText(tr("Error description: %1").arg(reply->errorString()));
  box->setText(message);

  box->exec();
}
