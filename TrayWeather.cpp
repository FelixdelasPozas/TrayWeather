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
#include <AboutDialog.h>

// Qt
#include <QNetworkReply>
#include <QObject>
#include <QMenu>
#include <QApplication>
#include <QDesktopWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QDebug>

//--------------------------------------------------------------------
TrayWeather::TrayWeather(const Configuration& configuration, QObject* parent)
: QSystemTrayIcon{parent}
, m_configuration{configuration}
, m_netManager   {std::make_shared<QNetworkAccessManager>()}
{
  setIcon(QIcon{":/TrayWeather/application.ico"});

  connectSignals();

  createMenuEntries();

  requestForecastData();
}

//--------------------------------------------------------------------
void TrayWeather::replyFinished(QNetworkReply* reply)
{
  if(reply->error() == QNetworkReply::NoError)
  {
    auto type = reply->header(QNetworkRequest::ContentTypeHeader).toString();

    if(type.startsWith("application/json"))
    {
      auto jsonDocument = QJsonDocument::fromJson(reply->readAll());

      if(!jsonDocument.isNull() && jsonDocument.isObject())
      {
        auto jsonObj = jsonDocument.object();
        auto values  = jsonObj.value("list").toArray();

        qDebug() << values.count();

        for(auto i = 0; i < values.count(); ++i)
        {
          auto valueObj = values.at(i).toObject();
          auto dt = valueObj.value("dt").toInt(0);
          auto unixDate = unixTimeStampToDate(dt);
          qDebug() << i << "date" << unixDate->tm_mday << unixDate->tm_mon + 1 << unixDate->tm_year + 1900 << "/" << unixDate->tm_hour << unixDate->tm_min << unixDate->tm_sec;
        }

        qDebug() << jsonDocument.object().keys();
        auto object = jsonDocument.object();

        for(auto item = object.constBegin(); item != object.constEnd(); ++item)
        {
          qDebug() << item.key();
          auto value = item.value();
          qDebug() << value.type();
          if(value.isObject())
          {
            auto obj = value.toObject();
            qDebug() << "obj" << obj.keys();
          }
          qDebug() << value.toString();
          qDebug() << "---";
        }
      }
    }

    reply->deleteLater();
    return;
  }

  // change to error icon.

  reply->deleteLater();
}

//--------------------------------------------------------------------
void TrayWeather::createMenuEntries()
{
  auto menu = new QMenu(nullptr);

  auto weather = new QAction{QIcon{":/TrayWeather/temp-celsius.svg"}, tr("Forecast..."), menu};
  connect(weather, SIGNAL(triggered(bool)), this, SLOT(showForecast()));

  menu->addSeparator();

  auto about = new QAction{QIcon{":/TrayWeather/information.svg"}, tr("About..."), menu};
  connect(about, SIGNAL(triggered(bool)), this, SLOT(showAboutDialog()));

  menu->addAction(about);

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
void TrayWeather::showAboutDialog() const
{
  static AboutDialog dialog;

  if(!dialog.isVisible())
  {
    auto scr = QApplication::desktop()->screenGeometry();
    dialog.move(scr.center() - dialog.rect().center());
    dialog.setModal(true);

    dialog.exec();
  }
}

//--------------------------------------------------------------------
void TrayWeather::connectSignals()
{
  connect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
          this,               SLOT(replyFinished(QNetworkReply*)));

  connect(this, SIGNAL(messageClicked()),
          this, SLOT(onMessageClicked()));

  connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this, SLOT(onActivation(QSystemTrayIcon::ActivationReason)));
}

//--------------------------------------------------------------------
void TrayWeather::onMessageClicked()
{
  this->showMessage(QString(), QString(), QSystemTrayIcon::MessageIcon::NoIcon, 0);
}

//--------------------------------------------------------------------
void TrayWeather::onActivation(QSystemTrayIcon::ActivationReason reason)
{
  if(reason == QSystemTrayIcon::ActivationReason::DoubleClick)
  {
    // TODO: show forecast.
    showForecast();
  }
}

//--------------------------------------------------------------------
void TrayWeather::showForecast() const
{
}

//--------------------------------------------------------------------
void TrayWeather::requestForecastData() const
{
  auto url = QUrl{QString(tr("http://api.openweathermap.org/data/2.5/forecast?lat=%1&lon=%2&appid=%3").arg(m_configuration.latitude)
                                                                                                      .arg(m_configuration.longitude)
                                                                                                      .arg(m_configuration.owm_apikey))};
  m_netManager->get(QNetworkRequest{url});
}
