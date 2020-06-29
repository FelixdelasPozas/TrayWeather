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
#include <Utils.h>

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
#include <QPainter>

//--------------------------------------------------------------------
TrayWeather::TrayWeather(Configuration& configuration, QObject* parent)
: QSystemTrayIcon{parent}
, m_configuration{configuration}
, m_netManager   {std::make_shared<QNetworkAccessManager>(this)}
, m_timer        {this}
, m_weatherDialog{nullptr}
, m_aboutDialog  {nullptr}
, m_configDialog {nullptr}
{
  m_timer.setSingleShot(true);

  connectSignals();

  createMenuEntries();

  requestData();
}

//--------------------------------------------------------------------
TrayWeather::~TrayWeather()
{
  disconnectSignals();

  m_timer.stop();
}

//--------------------------------------------------------------------
void TrayWeather::replyFinished(QNetworkReply* reply)
{
  bool hasError = false;

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
          auto values  = jsonObj.value("list").toArray();

          auto hasEntry = [this](unsigned long dt) { for(auto entry: this->m_data) if(entry.dt == dt) return true; return false; };

          for(auto i = 0; i < values.count(); ++i)
          {
            auto entry = values.at(i).toObject();

            ForecastData data;
            parseForecastEntry(entry, data, m_configuration.units);

            if(!hasEntry(data.dt))
            {
              m_data << data;
              if(!m_configuration.useGeolocation)
              {
                if(data.name    != "Unknown") m_configuration.region = m_configuration.city = data.name;
                if(data.country != "Unknown") m_configuration.country = data.country;
              }
            }
          }

          if(!m_data.isEmpty())
          {
            auto lessThan = [](const ForecastData &left, const ForecastData &right) { if(left.dt < right.dt) return true; return false; };
            qSort(m_data.begin(), m_data.end(), lessThan);
          }
        }
        else
        {
          parseForecastEntry(jsonObj, m_current, m_configuration.units);
          if(!m_configuration.useGeolocation)
          {
            if(m_current.name    != "Unknown") m_configuration.region = m_configuration.city = m_current.name;
            if(m_current.country != "Unknown") m_configuration.country = m_current.country;
          }
        }
      }
    }
    else
    {
      if(type.startsWith("text/plain"))
      {
        auto url = reply->url();
        if(url.toString().contains("edns.ip", Qt::CaseInsensitive))
        {
          auto data = reply->readAll();

          m_DNSIP = data.split(' ').at(1);
          requestGeolocation();
        }
        else
        {
          const auto data = QString::fromUtf8(reply->readAll());
          const auto values = parseCSV(data);

          if((values.first().compare("success", Qt::CaseInsensitive) == 0) && (values.size() == 14))
          {
            m_configuration.country   = values.at(1);
            m_configuration.region    = values.at(4);
            m_configuration.city      = values.at(5);
            m_configuration.zipcode   = values.at(6);
            m_configuration.latitude  = values.at(7).toDouble();
            m_configuration.longitude = values.at(8).toDouble();
            m_configuration.timezone  = values.at(9);
            m_configuration.isp       = values.at(10);
            m_configuration.ip        = values.at(13);

            requestForecastData();
          }
          else
          {
            hasError = true;
          }
        }
      }
    }

    if(validData())
    {
      m_timer.setInterval(m_configuration.updateTime*60*1000);
      m_timer.start();

      if(m_weatherDialog)
      {
        m_weatherDialog->setData(m_current, m_data, m_configuration);
      }
    }

    updateTooltip();

    reply->deleteLater();
    if(!hasError) return;
  }

  // change to error icon.
  setIcon(QIcon{":/TrayWeather/network_error.svg"});

  QString tooltip;
  auto url = reply->url().toString();
  if(url.contains("edns.ip", Qt::CaseInsensitive) || url.startsWith("http://ip-api.com/csv", Qt::CaseInsensitive))
  {
    tooltip = tr("Error requesting geolocation coordinates.");
  }
  else
  {
    tooltip = tr("Error requesting weather data.");
  }
  setToolTip(tooltip);

  reply->deleteLater();
}

//--------------------------------------------------------------------
void TrayWeather::showConfiguration()
{
  if(m_configDialog)
  {
    if(m_aboutDialog)
    {
      m_aboutDialog->raise();
    }
    else
    {
      m_configDialog->raise();
    }

    return;
  }

  m_configDialog = new ConfigurationDialog{m_configuration};

  auto scr = QApplication::desktop()->screenGeometry();
  m_configDialog->move(scr.center() - m_configDialog->rect().center());
  m_configDialog->setModal(true);
  m_configDialog->exec();

  Configuration configuration;
  m_configDialog->getConfiguration(configuration);

  delete m_configDialog;

  m_configDialog = nullptr;

  m_configuration.lightTheme    = configuration.lightTheme;
  m_configuration.iconType      = configuration.iconType;
  m_configuration.trayTextColor = configuration.trayTextColor;
  m_configuration.trayTextMode  = configuration.trayTextMode;
  m_configuration.minimumColor  = configuration.minimumColor;
  m_configuration.maximumColor  = configuration.maximumColor;
  m_configuration.minimumValue  = configuration.minimumValue;
  m_configuration.maximumValue  = configuration.maximumValue;

  if(configuration.isValid())
  {
    auto menu = this->contextMenu();

    if(menu && menu->actions().size() > 1 && menu->actions().first()->text().startsWith("Weather"))
    {
      QString iconLink = configuration.units == Temperature::CELSIUS ? ":/TrayWeather/temp-celsius.svg" : ":/TrayWeather/temp-fahrenheit.svg";
      QIcon icon{iconLink};

      menu->actions().first()->setIcon(icon);
    }

    auto changedCoords  = (configuration.latitude != m_configuration.latitude) || (configuration.longitude != m_configuration.longitude);
    auto changedMethod  = (configuration.useGeolocation != m_configuration.useGeolocation);
    auto changedIP      = (configuration.ip != m_configuration.ip);
    auto changedUpdate  = (configuration.updateTime != m_configuration.updateTime);
    auto changedAPIKey  = (configuration.owm_apikey != m_configuration.owm_apikey);
    auto changedUnits   = (configuration.units != m_configuration.units);
    auto changedRoaming = (configuration.roamingEnabled != m_configuration.roamingEnabled);

    if(changedIP || changedMethod || changedCoords || changedRoaming)
    {
      m_configuration.country        = configuration.country;
      m_configuration.region         = configuration.region;
      m_configuration.city           = configuration.city;
      m_configuration.zipcode        = configuration.zipcode;
      m_configuration.isp            = configuration.isp;
      m_configuration.ip             = configuration.ip;
      m_configuration.timezone       = configuration.timezone;
      m_configuration.latitude       = configuration.latitude;
      m_configuration.longitude      = configuration.longitude;
      m_configuration.useDNS         = configuration.useDNS;
      m_configuration.useGeolocation = configuration.useGeolocation;
      m_configuration.roamingEnabled = configuration.roamingEnabled;
      requestData();
    }

    if(changedUpdate)
    {
      m_configuration.updateTime = configuration.updateTime;
      m_timer.setInterval(m_configuration.updateTime);
      m_timer.start();
    }

    if(changedAPIKey)
    {
      m_configuration.owm_apikey = configuration.owm_apikey;
      requestData();
    }

    if(changedUnits)
    {
      m_configuration.units = configuration.units;
    }

    if(m_weatherDialog && validData())
    {
      m_weatherDialog->setData(m_current, m_data, m_configuration);
    }
  }

  updateTooltip();
}

//--------------------------------------------------------------------
void TrayWeather::updateTooltip()
{
  QString tooltip;
  QIcon icon;

  if(!validData())
  {
    tooltip = tr("Requesting weather data from the server...");
    icon = QIcon{":/TrayWeather/network_refresh.svg"};
  }
  else
  {
    const auto temperature = convertKelvinTo(m_current.temp, m_configuration.units);
    const auto tempString = QString::number(static_cast<int>(temperature));
    tooltip = tr("%1, %2\n%3\n%4%5").arg(m_configuration.city)
                                    .arg(m_configuration.country)
                                    .arg(toTitleCase(m_current.description))
                                    .arg(tempString)
                                    .arg(m_configuration.units == Temperature::CELSIUS ? "ºC" : "ºF");

    QPixmap pixmap = weatherPixmap(m_current);
    QPainter painter(&pixmap);

    auto interpolate = [this](int temp)
    {
      auto minColor = m_configuration.minimumColor;
      auto maxColor = m_configuration.maximumColor;

      double inc = static_cast<double>(temp-m_configuration.minimumValue)/(m_configuration.maximumValue - m_configuration.minimumValue);
      double rInc = (maxColor.red()   - minColor.red())   *inc;
      double gInc = (maxColor.green() - minColor.green()) *inc;
      double bInc = (maxColor.blue()  - minColor.blue())  *inc;
      double aInc = (maxColor.alpha() - minColor.alpha()) *inc;

      return QColor::fromRgb(minColor.red() + rInc, minColor.green() + gInc, minColor.blue() + bInc, minColor.alpha() + aInc);
    };

    switch(m_configuration.iconType)
    {
      case 0:
        break;
      case 1:
        pixmap.fill(Qt::transparent);
        // no break
      default:
      case 2:
        {
          QFont font = painter.font();
          font.setPixelSize(300);
          font.setBold(true);
          painter.setFont(font);

          QColor color;
          if(m_configuration.trayTextMode)
          {
            color = m_configuration.trayTextColor;
          }
          else
          {
            if(temperature < m_configuration.minimumValue) color = m_configuration.minimumColor;
            else if(temperature > m_configuration.maximumValue) color = m_configuration.maximumColor;
            else color = interpolate(temperature);
          }

          painter.setPen(color);
          painter.setRenderHint(QPainter::RenderHint::TextAntialiasing, false);
          painter.drawText(pixmap.rect(), Qt::AlignCenter, tempString);
        }
        break;
    }

    painter.end();

    icon = QIcon(pixmap);
  }

  setToolTip(tooltip);
  setIcon(icon);
}

//--------------------------------------------------------------------
void TrayWeather::createMenuEntries()
{
  auto menu = new QMenu(nullptr);

  QString iconLink = m_configuration.units == Temperature::CELSIUS ? ":/TrayWeather/temp-celsius.svg" : ":/TrayWeather/temp-fahrenheit.svg";
  auto weather = new QAction{QIcon{iconLink}, tr("Weather && forecast..."), menu};
  connect(weather, SIGNAL(triggered(bool)), this, SLOT(showForecast()));

  menu->addAction(weather);

  auto refresh = new QAction{QIcon{":/TrayWeather/network_refresh_black.svg"}, tr("Refresh..."), menu};
  connect(refresh, SIGNAL(triggered(bool)), this, SLOT(requestData()));

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
void TrayWeather::showAboutDialog()
{
  if(m_aboutDialog)
  {
    m_aboutDialog->raise();
    return;
  }

  m_aboutDialog = new AboutDialog{};

  auto scr = QApplication::desktop()->screenGeometry();
  m_aboutDialog->move(scr.center() - m_aboutDialog->rect().center());
  m_aboutDialog->setModal(true);
  m_aboutDialog->exec();

  delete m_aboutDialog;

  m_aboutDialog = nullptr;
}

//--------------------------------------------------------------------
void TrayWeather::connectSignals()
{
  connect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
          this,               SLOT(replyFinished(QNetworkReply*)));

  connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this, SLOT(onActivation(QSystemTrayIcon::ActivationReason)));

  connect(&m_timer, SIGNAL(timeout()),
          this,     SLOT(requestData()));
}

//--------------------------------------------------------------------
void TrayWeather::disconnectSignals()
{
  disconnect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
             this,               SLOT(replyFinished(QNetworkReply*)));

  disconnect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
             this, SLOT(onActivation(QSystemTrayIcon::ActivationReason)));

  disconnect(&m_timer, SIGNAL(timeout()),
             this,     SLOT(requestData()));
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
  if(m_weatherDialog)
  {
    if(m_aboutDialog)
    {
      m_aboutDialog->raise();
    }
    else
    {
      if(m_configDialog)
      {
        m_configDialog->raise();
      }
      else
      {
        m_weatherDialog->raise();
      }
    }

    return;
  }

  if(!validData())
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

  updateTooltip();

  m_weatherDialog = new WeatherDialog{};
  m_weatherDialog->setData(m_current, m_data, m_configuration);

  auto scr = QApplication::desktop()->screenGeometry();
  m_weatherDialog->move(scr.center() - m_weatherDialog->rect().center());
  m_weatherDialog->setModal(true);
  m_weatherDialog->exec();

  m_configuration.mapsEnabled = m_weatherDialog->mapsEnabled();

  delete m_weatherDialog;

  m_weatherDialog = nullptr;
}

//--------------------------------------------------------------------
void TrayWeather::requestData()
{
  if(m_netManager->networkAccessible() != QNetworkAccessManager::Accessible)
  {
    if(m_netManager)
    {
      disconnect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
                 this,               SLOT(replyFinished(QNetworkReply*)));
    }

    m_netManager = std::make_shared<QNetworkAccessManager>(this);

    connect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
            this,               SLOT(replyFinished(QNetworkReply*)));
  }

  invalidateData();
  updateTooltip();
  m_timer.setInterval(1*60*1000);
  m_timer.start();

  if(m_configuration.useGeolocation && m_configuration.roamingEnabled)
  {
    requestGeolocation();
  }
  else
  {
    requestForecastData();
  }
}

//--------------------------------------------------------------------
void TrayWeather::requestForecastData()
{
  if(m_netManager->networkAccessible() != QNetworkAccessManager::Accessible)
  {
    if(m_netManager)
    {
      disconnect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
                 this,               SLOT(replyFinished(QNetworkReply*)));
    }

    m_netManager = std::make_shared<QNetworkAccessManager>(this);

    connect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
            this,               SLOT(replyFinished(QNetworkReply*)));
  }

  auto url = QUrl{QString(tr("http://api.openweathermap.org/data/2.5/weather?lat=%1&lon=%2&appid=%3").arg(m_configuration.latitude)
                                                                                                     .arg(m_configuration.longitude)
                                                                                                     .arg(m_configuration.owm_apikey))};
  m_netManager->get(QNetworkRequest{url});

  url = QUrl{QString(tr("http://api.openweathermap.org/data/2.5/forecast?lat=%1&lon=%2&appid=%3").arg(m_configuration.latitude)
                                                                                                 .arg(m_configuration.longitude)
                                                                                                 .arg(m_configuration.owm_apikey))};
  m_netManager->get(QNetworkRequest{url});
}

//--------------------------------------------------------------------
bool TrayWeather::validData() const
{
  return !m_data.isEmpty() && m_current.isValid();
}

//--------------------------------------------------------------------
void TrayWeather::invalidateData()
{
  m_data.clear();
  m_current = ForecastData();
}

//--------------------------------------------------------------------
void TrayWeather::requestGeolocation()
{
  if(m_configuration.useDNS && m_DNSIP.isEmpty())
  {
    m_DNSIP = randomString();
    auto requestAddress = QString("http://%1.edns.ip-api.com/csv").arg(m_DNSIP);
    m_netManager->get(QNetworkRequest{QUrl{requestAddress}});

    return;
  }

  // CSV is easier to parse later.
  auto ipAddress = QString("http://ip-api.com/csv/%1").arg(m_DNSIP);
  m_netManager->get(QNetworkRequest{QUrl{ipAddress}});

  m_DNSIP.clear();
}
