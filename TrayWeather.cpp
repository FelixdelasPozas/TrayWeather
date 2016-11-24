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
#include <ConfigurationDialog.h>

// Qt
#include <QMessageBox>
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
TrayWeather::TrayWeather(Configuration& configuration, QObject* parent)
: QSystemTrayIcon{parent}
, m_configuration{configuration}
, m_netManager   {std::make_shared<QNetworkAccessManager>()}
, m_timer        {this}
{
  setIcon(QIcon{":/TrayWeather/application.svg"});
  m_timer.setSingleShot(true);

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

        if(jsonObj.keys().contains("cnt"))
        {
          m_data.clear();

          auto values  = jsonObj.value("list").toArray();

          for(auto i = 0; i < values.count(); ++i)
          {
            auto entry = values.at(i).toObject();

            ForecastData data;
            parseForecastEntry(entry, data, m_configuration.units);

            m_data << data;
          }
        }
        else
        {
          parseForecastEntry(jsonObj, m_current, m_configuration.units);
        }
      }
    }

    if(!m_data.isEmpty() && m_current.isValid())
    {
      updateTooltip();

      m_timer.setInterval(m_configuration.updateTime*60*1000);
      m_timer.start();

      m_weatherDialog.setData(m_current, m_data, m_configuration.units);
    }

    reply->deleteLater();
    return;
  }

  // change to error icon.
  setIcon(QIcon{":/TrayWeather/network_error.svg"});

  reply->deleteLater();
}

//--------------------------------------------------------------------
void TrayWeather::showConfiguration()
{
  static ConfigurationDialog dialog{m_configuration};

  if(!dialog.isVisible())
  {
    auto scr = QApplication::desktop()->screenGeometry();
    dialog.move(scr.center() - dialog.rect().center());
    dialog.setModal(true);
    dialog.exec();

    Configuration configuration;
    dialog.getConfiguration(configuration);

    if(configuration.updateTime != m_configuration.updateTime)
    {
      m_configuration.updateTime = configuration.updateTime;
      m_timer.setInterval(m_configuration.updateTime);
      m_timer.start();
    }

    if(configuration.owm_apikey != m_configuration.owm_apikey)
    {
      m_configuration.owm_apikey = configuration.owm_apikey;
      m_current = ForecastData();
      m_data.clear();

      requestForecastData();
    }

    if(configuration.units != m_configuration.units)
    {
      m_configuration.units = configuration.units;
      if(!m_data.isEmpty() && m_current.isValid())
      {
        updateTooltip();
        m_weatherDialog.setData(m_current, m_data, m_configuration.units);
      }
    }
  }
}

//--------------------------------------------------------------------
void TrayWeather::updateTooltip()
{
  QString tooltip;

  if(m_data.isEmpty() || !m_current.isValid())
  {
    tooltip = tr("Requesting weather data from the server...");
    setIcon(QIcon{":/TrayWeather/network_refresh.svg"});
  }
  else
  {
    tooltip = tr("%1, %2\n%3\n%4%5").arg(m_configuration.city)
                                    .arg(m_configuration.country)
                                    .arg(toTitleCase(m_current.description))
                                    .arg(QString::number(static_cast<int>(convertKelvinTo(m_current.temp, m_configuration.units))))
                                    .arg(m_configuration.units == Temperature::CELSIUS ? "ºC" : "ºF");
    setIcon(weatherIcon(m_current));
  }

  setToolTip(tooltip);
}

//--------------------------------------------------------------------
void TrayWeather::createMenuEntries()
{
  auto menu = new QMenu(nullptr);

  auto weather = new QAction{QIcon{":/TrayWeather/temp-celsius.svg"}, tr("Weather && forecast..."), menu};
  connect(weather, SIGNAL(triggered(bool)), this, SLOT(showForecast()));

  menu->addAction(weather);

  auto refresh = new QAction{QIcon{":/TrayWeather/network_refresh_black.svg"}, tr("Refresh..."), menu};
  connect(refresh, SIGNAL(triggered(bool)), this, SLOT(requestForecastData()));

  menu->addAction(refresh);

  menu->addSeparator();

  auto config = new QAction{QIcon{":/TrayWeather/settings.svg"}, tr("Configuration..."), menu};
  connect(config, SIGNAL(triggered(bool)), this, SLOT(showConfiguration()));

  menu->addAction(config);

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

  connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this, SLOT(onActivation(QSystemTrayIcon::ActivationReason)));

  connect(&m_timer, SIGNAL(timeout()),
          this,     SLOT(requestForecastData()));
}

//--------------------------------------------------------------------
void TrayWeather::onActivation(QSystemTrayIcon::ActivationReason reason)
{
  if(reason == QSystemTrayIcon::ActivationReason::DoubleClick)
  {
    showForecast();
  }
}

//--------------------------------------------------------------------
void TrayWeather::showForecast()
{
  if(m_data.isEmpty() || !m_current.isValid())
  {
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/TrayWeather/application.ico"));
    msgBox.setWindowTitle(QObject::tr("Tray Weather"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("TrayWeather has requested the weather data for your geographic location\nand it's still waiting for the response.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();

    return;
  }

  if(!m_weatherDialog.isVisible())
  {
    auto scr = QApplication::desktop()->screenGeometry();
    m_weatherDialog.move(scr.center() - m_weatherDialog.rect().center());
    m_weatherDialog.setModal(true);

    m_weatherDialog.exec();
  }
}

//--------------------------------------------------------------------
void TrayWeather::requestForecastData()
{
  updateTooltip();
  m_timer.setInterval(1*60*1000);
  m_timer.start();

  auto url = QUrl{QString(tr("http://api.openweathermap.org/data/2.5/weather?lat=%1&lon=%2&appid=%3").arg(m_configuration.latitude)
                                                                                                      .arg(m_configuration.longitude)
                                                                                                      .arg(m_configuration.owm_apikey))};
  m_netManager->get(QNetworkRequest{url});

  url = QUrl{QString(tr("http://api.openweathermap.org/data/2.5/forecast?lat=%1&lon=%2&appid=%3").arg(m_configuration.latitude)
                                                                                                .arg(m_configuration.longitude)
                                                                                                .arg(m_configuration.owm_apikey))};

  m_netManager->get(QNetworkRequest{url});
}
