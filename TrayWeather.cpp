/*
 File: TrayWeather.cpp
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
#include <TrayWeather.h>

// Qt
#include <QNetworkReply>
#include <QObject>
#include <QMenu>
#include <QApplication>
#include <QJsonDocument>

#include <QDebug>

//--------------------------------------------------------------------
TrayWeather::TrayWeather(const Configuration& configuration, QObject* parent)
: QSystemTrayIcon{parent}
, m_configuration{configuration}
, m_netManager   {std::make_shared<QNetworkAccessManager>()}
{
  setIcon(QIcon{":/TrayWeather/application.ico"});

  connect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
          this,               SLOT(replyFinished(QNetworkReply*)));

  createMenuEntries();

  requestForecastData();
}

//--------------------------------------------------------------------
void TrayWeather::replyFinished(QNetworkReply* reply)
{
  if(reply->error() == QNetworkReply::NoError)
  {
    auto type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    qDebug() << type;

    if(type.startsWith("application/json"))
    {
      const auto data = reply->readAll();
      auto jsonDocument = QJsonDocument::fromJson(data);

      if(!jsonDocument.isNull())
      {
        qDebug() << "array?" << jsonDocument.isArray() << "object?" << jsonDocument.isObject();
      }
    }
  }

  reply->deleteLater();
}

//--------------------------------------------------------------------
void TrayWeather::createMenuEntries()
{
  auto menu = new QMenu(nullptr);

  auto exit = new QAction{QIcon{":/TrayWeather/exit.svg"}, tr("Quit"), menu};
  connect(exit, SIGNAL(triggered(bool)), this, SLOT(exitApplication()));

  menu->addAction(exit);

  this->setContextMenu(menu);
}

//--------------------------------------------------------------------
void TrayWeather::exitApplication()
{
  this->hide();

  QApplication::instance()->quit();
}

//--------------------------------------------------------------------
void TrayWeather::requestForecastData() const
{
  auto url = QUrl{QString(tr("http://api.openweathermap.org/data/2.5/weather?lat=%1&lon=%2&appid=%3").arg(m_configuration.latitude)
                                                                                                     .arg(m_configuration.longitude)
                                                                                                     .arg(m_configuration.owm_apikey))};
  m_netManager->get(QNetworkRequest{url});
}
