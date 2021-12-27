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
#include <AlertDialog.h>

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
#include <QFile>
#include <QImage>
#include <QtWinExtras/QtWinExtrasDepends>

// C++
#include <chrono>

const QString RELEASES_ADDRESS = "https://api.github.com/repos/FelixdelasPozas/TrayWeather/releases";

//--------------------------------------------------------------------
TrayWeather::TrayWeather(Configuration& configuration, QObject* parent)
: QSystemTrayIcon {parent}
, m_configuration {configuration}
, m_netManager    {std::make_shared<QNetworkAccessManager>(this)}
, m_timer         {this}
, m_weatherDialog {nullptr}
, m_aboutDialog   {nullptr}
, m_configDialog  {nullptr}
, m_additionalTray{nullptr}
, m_eventFilter   {this}
, m_alertsDialog  {nullptr}
{
  m_timer.setSingleShot(true);

  qApp->installNativeEventFilter(&m_eventFilter);

  connectSignals();

  createMenuEntries();

  requestData();
}

//--------------------------------------------------------------------
TrayWeather::~TrayWeather()
{
  disconnectSignals();

  m_timer.stop();
  m_updatesTimer.stop();
}

//--------------------------------------------------------------------
void TrayWeather::replyFinished(QNetworkReply* reply)
{
  auto setErrorTooltip = [this](const QString &text)
  {
    const auto errorIcon = QIcon{":/TrayWeather/network_error.svg"};
    setIcon(errorIcon);

    setToolTip(text);

    if(m_additionalTray)
    {
      m_additionalTray->setIcon(errorIcon);
      m_additionalTray->setToolTip(text);
    }
  };

  const auto originUrl = reply->request().url().toString();
  const auto contents  = reply->readAll();

  if(originUrl.contains("github", Qt::CaseInsensitive))
  {
    if(reply->error() == QNetworkReply::NoError)
    {
      processGithubData(contents);
    }
    else
    {
      auto githubError = tr("Error requesting Github releases data.");
      if(!toolTip().contains(githubError, Qt::CaseSensitive))
      {
        githubError = toolTip() + "\n\n" + githubError;
        setToolTip(githubError);
        if(m_additionalTray) m_additionalTray->setToolTip(githubError);
      }
    }
  }

  if(originUrl.contains("ip-api", Qt::CaseInsensitive))
  {
    if(reply->error() == QNetworkReply::NoError)
    {
      processGeolocationData(contents, originUrl.contains("edns", Qt::CaseInsensitive));
    }
    else
    {
      const auto tooltip = tr("Error requesting geolocation coordinates.");
      setErrorTooltip(tooltip);
    }
  }

  if(originUrl.contains("pollution", Qt::CaseInsensitive))
  {
    if(reply->error() == QNetworkReply::NoError)
    {
      processPollutionData(contents);
    }
    else
    {
      const auto tooltip = tr("Error requesting weather data.");
      setErrorTooltip(tooltip);
    }
  }
  else
  {
    if(originUrl.contains("onecall", Qt::CaseInsensitive))
    {
      if(reply->error() == QNetworkReply::NoError)
      {
        processOneCallData(contents);
      }
      else
      {
        const auto tooltip = tr("Error requesting weather data.");
        setErrorTooltip(tooltip);
      }
    }
    else
    {
      if(originUrl.contains("openweathermap", Qt::CaseInsensitive))
      {
        if(reply->error() == QNetworkReply::NoError)
        {
          processWeatherData(contents);
        }
        else
        {
          const auto tooltip = tr("Error requesting weather data.");
          setErrorTooltip(tooltip);
        }
      }
    }
  }

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

  connect(m_configDialog, SIGNAL(languageChanged(const QString &)), this, SLOT(onLanguageChanged(const QString &)));

  const auto scr = QApplication::desktop()->screenGeometry();
  m_configDialog->move(scr.center() - m_configDialog->rect().center());
  m_configDialog->setModal(true);
  const auto result = m_configDialog->exec();

  Configuration configuration;
  m_configDialog->getConfiguration(configuration);

  disconnect(m_configDialog, SIGNAL(languageChanged(const QString &)), this, SLOT(onLanguageChanged(const QString &)));
  delete m_configDialog;
  m_configDialog = nullptr;

  if(result != QDialog::Accepted)
  {
    if(configuration.language != m_configuration.language)
    {
      onLanguageChanged(m_configuration.language);
    }

    if(configuration.lightTheme != m_configuration.lightTheme)
    {
      QString sheet;

      if(!m_configuration.lightTheme)
      {
        QFile file(":qdarkstyle/style.qss");
        file.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&file);
        sheet = ts.readAll();
      }

      qApp->setStyleSheet(sheet);
    }

    return;
  }

  const auto changedLanguage = (configuration.language != m_configuration.language);
  const auto changedRoaming  = (configuration.roamingEnabled != m_configuration.roamingEnabled);

  m_configuration.lightTheme       = configuration.lightTheme;
  m_configuration.iconType         = configuration.iconType;
  m_configuration.iconTheme        = configuration.iconTheme;
  m_configuration.iconThemeColor   = configuration.iconThemeColor;
  m_configuration.trayTextColor    = configuration.trayTextColor;
  m_configuration.trayTextMode     = configuration.trayTextMode;
  m_configuration.trayTextSize     = configuration.trayTextSize;
  m_configuration.minimumColor     = configuration.minimumColor;
  m_configuration.maximumColor     = configuration.maximumColor;
  m_configuration.minimumValue     = configuration.minimumValue;
  m_configuration.maximumValue     = configuration.maximumValue;
  m_configuration.autostart        = configuration.autostart;
  m_configuration.language         = configuration.language;
  m_configuration.tooltipFields    = configuration.tooltipFields;
  m_configuration.graphUseRain     = configuration.graphUseRain;
  m_configuration.showAlerts       = configuration.showAlerts;

  bool requestNewData = false;

  if(changedLanguage) requestNewData = true;

  if(configuration.isValid())
  {
    auto menu = this->contextMenu();

    if(menu && menu->actions().size() > 1)
    {
      QString iconLink = temperatureIconString(configuration);
      QIcon icon{iconLink};

      menu->actions().first()->setIcon(icon);
    }

    const auto changedCoords      = (configuration.latitude != m_configuration.latitude) || (configuration.longitude != m_configuration.longitude);
    const auto changedMethod      = (configuration.useGeolocation != m_configuration.useGeolocation);
    const auto changedIP          = (configuration.ip != m_configuration.ip);
    const auto changedUpdateTime  = (configuration.updateTime != m_configuration.updateTime);
    const auto changedAPIKey      = (configuration.owm_apikey != m_configuration.owm_apikey);
    const auto changedUpdateCheck = (configuration.update != m_configuration.update);
    const auto changedUnits       = (configuration.units != m_configuration.units) ||
                                    (configuration.units == Units::CUSTOM && (configuration.tempUnits != m_configuration.tempUnits ||
                                                                              configuration.precUnits != m_configuration.precUnits ||
                                                                              configuration.windUnits != m_configuration.windUnits ||
                                                                              configuration.pressureUnits != m_configuration.pressureUnits));

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
      requestNewData = true;
    }

    if(changedUpdateTime)
    {
      m_configuration.updateTime = configuration.updateTime;
      m_timer.setInterval(m_configuration.updateTime);
      m_timer.start();
    }

    if(changedAPIKey)
    {
      m_configuration.owm_apikey = configuration.owm_apikey;
      requestNewData = true;
    }

    if(changedUnits)
    {
      m_configuration.units = configuration.units;
      if(m_configuration.units == Units::CUSTOM)
      {
        m_configuration.tempUnits     = configuration.tempUnits;
        m_configuration.pressureUnits = configuration.pressureUnits;
        m_configuration.windUnits     = configuration.windUnits;
        m_configuration.precUnits     = configuration.precUnits;
      }
      requestNewData = true;
    }

    if(!requestNewData)
    {
      if(m_weatherDialog && validData())
      {
        m_weatherDialog->setWeatherData(m_current, m_data, m_configuration);
      }

      if(m_weatherDialog && !m_pData.isEmpty())
      {
        m_weatherDialog->setPollutionData(m_pData);
      }

      if(m_weatherDialog && !m_vData.isEmpty())
      {
        m_weatherDialog->setUVData(m_vData);
      }
    }

    if(changedUpdateCheck)
    {
      m_configuration.update = configuration.update;
      checkForUpdates();
    }
  }

  updateTooltip();

  save(m_configuration);

  if(changedRoaming)
  {
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/TrayWeather/application.ico"));
    msgBox.setWindowTitle(QObject::tr("Tray Weather"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(QObject::tr("TrayWeather needs to be restarted for the new configuration to take effect.\nThe application will exit now."));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();

    exitApplication();
  }

  if(requestNewData)
  {
    requestData();
  }
}

//--------------------------------------------------------------------
void TrayWeather::updateTooltip()
{
  QString tooltip;
  QIcon icon;

  if(m_configuration.iconType == 3)
  {
    if(!m_additionalTray)
    {
      m_additionalTray = new QSystemTrayIcon{this};
      m_additionalTray->setContextMenu(this->contextMenu());

      connect(m_additionalTray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
              this,             SLOT(onActivation(QSystemTrayIcon::ActivationReason)));
    }
  }
  else
  {
    if(m_additionalTray)
    {
      m_additionalTray->hide();

      disconnect(m_additionalTray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                 this,             SLOT(onActivation(QSystemTrayIcon::ActivationReason)));

      m_additionalTray->deleteLater();
      m_additionalTray = nullptr;
    }
  }

  if(!validData())
  {
    tooltip = tr("Requesting weather data from the server...");
    icon = QIcon{":/TrayWeather/network_refresh.svg"};

    setToolTip(tooltip);
    setIcon(icon);

    if(m_additionalTray)
    {
      m_additionalTray->setToolTip(tooltip);
      m_additionalTray->setIcon(icon);
      if(!m_additionalTray->isVisible()) m_additionalTray->show();
    }

    return;
  }

  tooltip = tooltipText();

  QPixmap pixmap = weatherPixmap(m_current, m_configuration.iconTheme, m_configuration.iconThemeColor).scaled(384,384,Qt::KeepAspectRatio, Qt::SmoothTransformation);

  auto interpolate = [this](int temp)
  {
    const auto minColor = m_configuration.minimumColor;
    const auto maxColor = m_configuration.maximumColor;
    const auto value = std::min(m_configuration.maximumValue, std::max(m_configuration.minimumValue, temp));

    const double inc = static_cast<double>(value-m_configuration.minimumValue)/(m_configuration.maximumValue - m_configuration.minimumValue);
    const double rInc = (maxColor.red()   - minColor.red())   * inc;
    const double gInc = (maxColor.green() - minColor.green()) * inc;
    const double bInc = (maxColor.blue()  - minColor.blue())  * inc;
    const double aInc = (maxColor.alpha() - minColor.alpha()) * inc;

    return QColor::fromRgb(minColor.red() + rInc, minColor.green() + gInc, minColor.blue() + bInc, minColor.alpha() + aInc);
  };

  switch(m_configuration.iconType)
  {
    case 0:
      break;
    case 3:
      if(m_additionalTray)
      {
        m_additionalTray->setIcon(QIcon(pixmap));
        if(!m_additionalTray->isVisible()) m_additionalTray->show();
      }
      /* fall through */
    case 1:
      pixmap.fill(Qt::transparent);
      /* fall through */
    default:
    case 2:
      {
        double temperatureValue = m_current.temp;
        if(m_configuration.units == Units::CUSTOM && m_configuration.tempUnits == TemperatureUnits::FAHRENHEIT)
        {
          temperatureValue = convertCelsiusToFahrenheit(m_current.temp);
        }

        QPixmap tempPixmap{384,384};
        tempPixmap.fill(Qt::transparent);
        QPainter painter(&tempPixmap);

        const auto roundedTemp = static_cast<int>(std::nearbyint(temperatureValue));
        const auto roundedString = QString::number(roundedTemp);
        QFont font = painter.font();
        font.setPixelSize(m_configuration.trayTextSize - ((roundedString.length() - 3) * 50));
        font.setWeight(QFont::Bold);
        painter.setFont(font);

        QColor color;
        if(m_configuration.trayTextMode)
        {
          color = m_configuration.trayTextColor;
        }
        else
        {
          color = interpolate(roundedTemp);
        }

        painter.setPen(color);
        painter.setRenderHint(QPainter::RenderHint::TextAntialiasing, false);
        painter.drawText(tempPixmap.rect(), Qt::AlignCenter, roundedString);

        const auto invertedColor = QColor{color.red() ^ 0xFF, color.green() ^ 0xFF, color.blue() ^ 0xFF};
        const auto image = addQuickBorderToImage(tempPixmap.toImage(), invertedColor, 16);

        painter.drawImage(QPoint{0,0}, image);
        painter.end();

        painter.begin(&pixmap);
        painter.drawImage(QPoint{0,0}, tempPixmap.toImage());
      }
      break;
  }

  icon = QIcon(pixmap);

  setToolTip(tooltip);
  setIcon(icon);

  if(m_additionalTray) m_additionalTray->setToolTip(tooltip);
}

//--------------------------------------------------------------------
QString TrayWeather::tooltipText() const
{
  QStringList fieldsText;
  const QString pollutionUnits = "µg/m3";

  for(int i = 0; i < m_configuration.tooltipFields.size(); ++i)
  {
    switch(m_configuration.tooltipFields.at(i))
    {
      case TooltipText::LOCATION:
        {
          QStringList place;
          if(!m_configuration.city.isEmpty())    place << m_configuration.city;
          if(!m_configuration.country.isEmpty()) place << m_configuration.country;
          fieldsText << place.join(", ");
        }
        break;
      case TooltipText::WEATHER:
        fieldsText << toTitleCase(m_current.description);
        break;
      case TooltipText::TEMPERATURE:
        {
          double temperatureValue = m_current.temp;
          if(m_configuration.units == Units::CUSTOM && m_configuration.tempUnits == TemperatureUnits::FAHRENHEIT)
          {
            temperatureValue = convertCelsiusToFahrenheit(m_current.temp);
          }
          fieldsText << QString::number(temperatureValue, 'f', 1) + temperatureIconText(m_configuration);
        }
        break;
      case TooltipText::CLOUDINESS:
        fieldsText << tr("Cloudiness: ") + QString("%1%").arg(m_current.cloudiness);
        break;
      case TooltipText::HUMIDITY:
        fieldsText << tr("Humidity: ") + QString("%1%").arg(m_current.humidity);
        break;
      case TooltipText::PRESSURE:
        {
          QString pressUnits;
          double pressureValue = m_current.pressure;
          switch(m_configuration.pressureUnits)
          {
            case PressureUnits::INHG:
              pressureValue = converthPaToinHg(m_current.pressure);
              pressUnits = tr("inHg");
              break;
            case PressureUnits::MMGH:
              pressureValue = converthPaTommHg(m_current.pressure);
              pressUnits = tr("mmHg");
              break;
            case PressureUnits::PSI:
              pressureValue = converthPaToPSI(m_current.pressure);
              pressUnits = tr("PSI");
              break;
            default:
            case PressureUnits::HPA:
              pressUnits = tr("hPa");
              break;
          }
          fieldsText << tr("Pressure: ") + QString("%1 %2").arg(pressureValue).arg(pressUnits);
        }
        break;
      case TooltipText::WIND_SPEED:
        {
          QString windUnits;
          double windValue = m_current.wind_speed;
          switch(m_configuration.windUnits)
          {
            case WindUnits::FEETSEC:
              windUnits = tr("feet/s");
              windValue = convertMetersSecondToFeetSecond(m_current.wind_speed);
              break;
            case WindUnits::KMHR:
              windUnits = tr("km/h");
              windValue = convertMetersSecondToKilometersHour(m_current.wind_speed);
              break;
            case WindUnits::MILHR:
              windUnits = tr("mil/h");
              windValue = convertMetersSecondToMilesHour(m_current.wind_speed);
              break;
            default:
            case WindUnits::METSEC:
              windUnits = tr("met/sec");
              break;
          }
          fieldsText << tr("Wind: ") + QString("%1 %2").arg(windValue).arg(windUnits);
        }
        break;
      case TooltipText::SUNRISE:
        if(m_current.sunrise != 0)
        {
          struct tm t;
          unixTimeStampToDate(t, m_current.sunrise);
          QDateTime dtTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
          fieldsText << tr("Sunrise: ") + dtTime.toString("hh:mm");
        }
        break;
      case TooltipText::SUNSET:
        if(m_current.sunset != 0)
        {
          struct tm t;
          unixTimeStampToDate(t, m_current.sunset);
          QDateTime dtTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
          fieldsText << tr("Sunset: ") + dtTime.toString("hh:mm");
        }
        break;
      case TooltipText::AIR_QUALITY:
        if(!m_pData.empty())
        {
          QString airQuality;
          switch(m_pData.first().aqi)
          {
            case 1:
              airQuality = tr("Good");
              break;
            case 2:
              airQuality = tr("Fair");
              break;
            case 3:
              airQuality = tr("Moderate");
              break;
            case 4:
              airQuality = tr("Poor");
              break;
            default:
              airQuality = tr("Very poor");
              break;
          }
          fieldsText << tr("Air: ") + airQuality;
        }
        break;
      case TooltipText::UV:
        if (!m_vData.isEmpty())
        {
          const auto index = static_cast<int>(std::nearbyint(m_vData.first().idx));
          fieldsText << tr("UV: ") + QString("%1").arg(index);
        }
        break;
      case TooltipText::AIR_CO:
        if (!m_pData.empty())
        {
          const auto &data = m_pData.first();
          fieldsText << QString("CO: ") + QString("%1%2").arg(data.co).arg(pollutionUnits);
        }
        break;
      case TooltipText::AIR_O3:
        if (!m_pData.empty())
        {
          const auto &data = m_pData.first();
          fieldsText << QString("O3: ") + QString("%1%2").arg(data.o3).arg(pollutionUnits);
        }
        break;
      case TooltipText::AIR_NO:
        if (!m_pData.empty())
        {
          const auto &data = m_pData.first();
          fieldsText << QString("NO: ") + QString("%1%2").arg(data.no).arg(pollutionUnits);
        }
        break;
      case TooltipText::AIR_NO2:
        if (!m_pData.empty())
        {
          const auto &data = m_pData.first();
          fieldsText << QString("NO2: ") + QString("%1%2").arg(data.no2).arg(pollutionUnits);
        }
        break;
      case TooltipText::AIR_SO2:
        if (!m_pData.empty())
        {
          const auto &data = m_pData.first();
          fieldsText << QString("SO2: ") + QString("%1%2").arg(data.so2).arg(pollutionUnits);
        }
        break;
      case TooltipText::AIR_NH3:
        if (!m_pData.empty())
        {
          const auto &data = m_pData.first();
          fieldsText << QString("NH3: ") + QString("%1%2").arg(data.nh3).arg(pollutionUnits);
        }
        break;
      case TooltipText::AIR_PM25:
        if (!m_pData.empty())
        {
          const auto &data = m_pData.first();
          fieldsText << QString("PM2.5: ") + QString("%1%2").arg(data.pm2_5).arg(pollutionUnits);
        }
        break;
      case TooltipText::AIR_PM10:
        if (!m_pData.empty())
        {
          const auto &data = m_pData.first();
          fieldsText << QString("PM10: ") + QString("%1%2").arg(data.pm10).arg(pollutionUnits);
        }
        break;
      default:
        break;
    }
  }

  auto text = fieldsText.join("\n");

  // NOTE: https://docs.microsoft.com/es-es/windows/win32/api/shellapi/ns-shellapi-notifyicondataa
  // Length of tooltip text is 128 including null termination in Windows.
  if(text.length() > 127) text = text.left(123) + " ...";

  return text;
}

//--------------------------------------------------------------------
void TrayWeather::createMenuEntries()
{
  auto menu = new QMenu(nullptr);

  QString iconLink = temperatureIconString(m_configuration);
  auto weather = new QAction{QIcon{iconLink}, tr("Current weather..."), menu};
  connect(weather, SIGNAL(triggered(bool)), this, SLOT(showTab()));

  menu->addAction(weather);

  auto forecast = new QAction{tr("Forecast..."), menu};
  connect(forecast, SIGNAL(triggered(bool)), this, SLOT(showTab()));

  menu->addAction(forecast);

  auto pollution = new QAction{tr("Pollution..."), menu};
  connect(pollution, SIGNAL(triggered(bool)), this, SLOT(showTab()));

  menu->addAction(pollution);

  auto uv = new QAction{tr("UV..."), menu};
  connect(uv, SIGNAL(triggered(bool)), this, SLOT(showTab()));

  menu->addAction(uv);

  auto maps = new QAction{tr("Maps..."), menu};
  connect(maps, SIGNAL(triggered(bool)), this, SLOT(showTab()));

  menu->addAction(maps);

  maps->setEnabled(m_configuration.mapsEnabled);

  menu->addSeparator();

  auto refresh = new QAction{QIcon{":/TrayWeather/network_refresh_black.svg"}, tr("Refresh..."), menu};
  connect(refresh, SIGNAL(triggered(bool)), this, SLOT(requestData()));

  menu->addAction(refresh);

  menu->addSeparator();

  auto alertAction = new QAction(QIcon(":/TrayWeather/alert.svg"), tr("Last alert..."), menu);
  connect(alertAction, SIGNAL(triggered(bool)), this, SLOT(showAlert()));

  menu->addAction(alertAction);

  alertAction->setEnabled(false);

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
    showTab();
  }
}

//--------------------------------------------------------------------
void TrayWeather::showTab()
{
  static int lastTab = 0;

  const auto caller = qobject_cast<QAction *>(sender());
  if(caller)
  {
    auto actions = contextMenu()->actions();

    for(const auto i: {0,1,2,3,4})
    {
      if(caller == actions.at(i))
      {
        lastTab = i;
        break;
      }
    }
  }
  else
  {
    lastTab = m_configuration.lastTab;
  }

  m_configuration.lastTab = lastTab;

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

    m_weatherDialog->m_tabWidget->setCurrentIndex(lastTab);
    return;
  }

  if(!validData())
  {
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/TrayWeather/application.ico"));
    msgBox.setWindowTitle(QObject::tr("Tray Weather"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(QObject::tr("TrayWeather has requested the weather data for your geographic location\nand it's still waiting for the response."));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();

    return;
  }

  updateTooltip();

  m_weatherDialog = new WeatherDialog{};
  m_weatherDialog->setWeatherData(m_current, m_data, m_configuration);
  m_weatherDialog->setPollutionData(m_pData);
  m_weatherDialog->setUVData(m_vData);

  connect(m_weatherDialog, SIGNAL(mapsEnabled(bool)), this, SLOT(onMapsStateChanged(bool)));

  m_weatherDialog->show();
  m_weatherDialog->m_tabWidget->setCurrentIndex(lastTab);

  auto scr = QApplication::desktop()->screenGeometry();
  m_weatherDialog->move(scr.center() - m_weatherDialog->rect().center());
  m_weatherDialog->setModal(true);
  m_weatherDialog->exec();

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

  checkForUpdates();
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

  QString lang = "en";
  if(!m_configuration.language.isEmpty() && m_configuration.language.contains('_'))
  {
    const auto settings_lang = m_configuration.language.split('_').first();
    if(OWM_LANGUAGES.contains(settings_lang, Qt::CaseSensitive)) lang = settings_lang;

    const auto settings_compl = m_configuration.language.toLower();
    if(OWM_LANGUAGES.contains(settings_compl, Qt::CaseInsensitive)) lang = settings_compl;
  }

  auto url = QUrl{QString("http://api.openweathermap.org/data/2.5/weather?lat=%1&lon=%2&lang=%3&units=%4&appid=%5").arg(m_configuration.latitude)
                                                                                                                   .arg(m_configuration.longitude)
                                                                                                                   .arg(lang)
                                                                                                                   .arg(unitsToText(m_configuration.units))
                                                                                                                   .arg(m_configuration.owm_apikey)};
  m_netManager->get(QNetworkRequest{url});

  url = QUrl{QString("http://api.openweathermap.org/data/2.5/forecast?lat=%1&lon=%2&lang=%3&units=%4&appid=%5").arg(m_configuration.latitude)
                                                                                                               .arg(m_configuration.longitude)
                                                                                                               .arg(lang)
                                                                                                               .arg(unitsToText(m_configuration.units))
                                                                                                               .arg(m_configuration.owm_apikey)};
  m_netManager->get(QNetworkRequest{url});

  url = QUrl{QString("http://api.openweathermap.org/data/2.5/air_pollution/forecast?lat=%1&lon=%2&lang=%3&units=%4&appid=%5").arg(m_configuration.latitude)
                                                                                                                             .arg(m_configuration.longitude)
                                                                                                                             .arg(lang)
                                                                                                                             .arg(unitsToText(m_configuration.units))
                                                                                                                             .arg(m_configuration.owm_apikey)};
  m_netManager->get(QNetworkRequest{url});

  url = QUrl{QString("http://api.openweathermap.org/data/2.5/onecall?lat=%1&lon=%2&lang=%3&exclude=minutely&units=%4&appid=%5").arg(m_configuration.latitude)
                                                                                                                               .arg(m_configuration.longitude)
                                                                                                                               .arg(lang)
                                                                                                                               .arg(unitsToText(m_configuration.units))
                                                                                                                               .arg(m_configuration.owm_apikey)};
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
  m_pData.clear();
  m_vData.clear();
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

//--------------------------------------------------------------------
void TrayWeather::onMapsStateChanged(bool value)
{
  auto menu = this->contextMenu();
  if(menu && menu->actions().size() >= 5)
  {
    auto entries = menu->actions();
    entries.at(4)->setEnabled(value);
  }

  m_configuration.mapsEnabled = value;
}

//--------------------------------------------------------------------
void TrayWeather::checkForUpdates()
{
  m_updatesTimer.stop();

  auto last = m_configuration.lastCheck;

  switch(m_configuration.update)
  {
    case Update::MONTHLY:
      if(last.isValid())
        last = last.addMonths(1);
      break;
    case Update::WEEKLY:
      if(last.isValid())
        last = last.addDays(7);
      break;
    case Update::DAILY:
      if(last.isValid())
        last = last.addDays(1);
      break;
    default:
    case Update::NEVER:
      return;
      break;
  }

  const auto now = QDateTime::currentDateTime();
  if(last.isValid() && last > now)
  {
    auto msec = now.msecsTo(last);
    m_updatesTimer.singleShot(msec, SLOT(checkForUpdates));
  }
  else
  {
    m_netManager->get(QNetworkRequest{QUrl{RELEASES_ADDRESS}});

    m_configuration.lastCheck = now;
    checkForUpdates();
  }
}

//--------------------------------------------------------------------
void TrayWeather::onLanguageChanged(const QString &lang)
{
  changeLanguage(lang);

  translateMenu();
}

//--------------------------------------------------------------------
void TrayWeather::translateMenu()
{
  const auto actions = contextMenu()->actions();
  actions.at(0)->setText(tr("Current weather..."));
  actions.at(1)->setText(tr("Forecast..."));
  actions.at(2)->setText(tr("Pollution..."));
  actions.at(3)->setText(tr("UV..."));
  actions.at(4)->setText(tr("Maps..."));
  actions.at(6)->setText(tr("Refresh..."));
  actions.at(8)->setText(tr("Last alert..."));
  actions.at(10)->setText(tr("Configuration..."));
  actions.at(12)->setText(tr("About..."));
  actions.at(13)->setText(tr("Quit"));
}

//--------------------------------------------------------------------
void TrayWeather::processGithubData(const QByteArray &data)
{
  const auto jsonDocument = QJsonDocument::fromJson(data);

  int currentNumbers[3], lastNumbers[3];
  const auto currentVersion = AboutDialog::VERSION.split(".");

  // Github parse tag of last release.
  auto jsonObj = jsonDocument.array();
  auto lastRelease = jsonObj.at(0).toObject();
  const auto version = lastRelease.value("tag_name").toString();
  const auto lastVersion = version.split(".");
  const auto body = lastRelease.value("body").toString();
  bool hasError = false;

  if(lastVersion.size() != 3 || body.isEmpty())
  {
    hasError = true;
  }
  else
  {
    bool ok = false;
    for(int i: {0,1,2})
    {
      currentNumbers[i] = currentVersion.at(i).toInt(&ok);
      if(!ok) hasError = true;
      lastNumbers[i] = lastVersion.at(i).toInt(&ok);
      if(!ok) hasError = true;
      if(hasError) break;
    }
  }

  if(!hasError)
  {
    if((currentNumbers[0] < lastNumbers[0]) ||
      ((currentNumbers[0] == lastNumbers[0]) && (currentNumbers[1] < lastNumbers[1])) ||
      ((currentNumbers[0] == lastNumbers[0]) && (currentNumbers[1] == lastNumbers[1]) && currentNumbers[2] < lastNumbers[2]))
    {
      const auto message = tr("There is a new release of <b>Tray Weather</b> at the <a href=\"https://github.com/FelixdelasPozas/TrayWeather/releases\">github website</a>!");
      const auto informative = tr("<center><b>Version %1</b> has been released!</center>").arg(version);
      const auto details = tr("Release notes:\n%1").arg(body);
      const auto title = tr("Tray Weather updated to version %1").arg(version);

      QMessageBox msgBox;
      msgBox.setWindowTitle(title);
      msgBox.setText(message);
      msgBox.setInformativeText(informative);
      msgBox.setDetailedText(details);
      msgBox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
      msgBox.setIconPixmap(QIcon(":/TrayWeather/application.svg").pixmap(QSize{64,64}));
      msgBox.exec();
    }
  }
  else
  {
    auto githubError = tr("Error requesting Github releases data.");
    if(!toolTip().contains(githubError, Qt::CaseSensitive))
    {
      githubError = toolTip() + "\n\n" + githubError;
      setToolTip(githubError);
    }
  }
}

//--------------------------------------------------------------------
void TrayWeather::processWeatherData(const QByteArray &data)
{
  const auto jsonDocument = QJsonDocument::fromJson(data);

  if(!jsonDocument.isNull() && jsonDocument.isObject())
  {
    // to discard entries older than 'right now'.
    const auto currentDt = std::chrono::duration_cast<std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto jsonObj = jsonDocument.object();

    const auto keys = jsonObj.keys();

    if(keys.contains("cnt"))
    {
      const auto values  = jsonObj.value("list").toArray();

      auto hasEntry = [this](unsigned long dt) { for(auto entry: this->m_data) if(entry.dt == dt) return true; return false; };

      for(auto i = 0; i < values.count(); ++i)
      {
        auto entry = values.at(i).toObject();

        const auto dt = entry.value("dt").toInt(0);
        if(dt < currentDt) continue;

        ForecastData data;
        parseForecastEntry(entry, data);

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
      parseForecastEntry(jsonObj, m_current);
      if(!m_configuration.useGeolocation)
      {
        if(m_current.name    != "Unknown") m_configuration.region = m_configuration.city = m_current.name;
        if(m_current.country != "Unknown") m_configuration.country = m_current.country;
      }
    }

    if(validData())
    {
      m_timer.setInterval(m_configuration.updateTime*60*1000);
      m_timer.start();

      if(m_weatherDialog)
      {
        m_weatherDialog->setWeatherData(m_current, m_data, m_configuration);
      }
    }

    updateTooltip();
  }
}

//--------------------------------------------------------------------
void TrayWeather::processGeolocationData(const QByteArray &data, const bool isDNS)
{
  if(isDNS)
  {
    m_DNSIP = data.split(' ').at(1);
    requestGeolocation();
  }
  else
  {
    const auto csvData = QString::fromUtf8(data);
    const auto values = parseCSV(csvData);

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
      const auto errorIcon = QIcon{":/TrayWeather/network_error.svg"};
      setIcon(errorIcon);

      QString tooltip;
      tooltip = tr("Error requesting geolocation coordinates.");
      setToolTip(tooltip);

      if(m_additionalTray)
      {
        m_additionalTray->setIcon(errorIcon);
        m_additionalTray->setToolTip(tooltip);
      }
    }
  }
}

//--------------------------------------------------------------------
void TrayWeather::processPollutionData(const QByteArray &data)
{
  const auto jsonDocument = QJsonDocument::fromJson(data);

  if(!jsonDocument.isNull() && jsonDocument.isObject())
  {
    // to discard entries older than 'right now'.
    const auto currentDt = std::chrono::duration_cast<std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto jsonObj = jsonDocument.object();
    const auto values  = jsonObj.value("list").toArray();

    auto hasEntry = [this](unsigned long dt) { for(auto entry: this->m_pData) if(entry.dt == dt) return true; return false; };

    for(auto i = 0; i < values.count(); ++i)
    {
      auto entry = values.at(i).toObject();

      const auto dt = entry.value("dt").toInt(0);
      if(dt < currentDt) continue;

      PollutionData data;
      parsePollutionEntry(entry, data);

      if(!hasEntry(data.dt)) m_pData << data;
    }

    if(!m_pData.isEmpty())
    {
      auto lessThan = [](const PollutionData &left, const PollutionData &right) { if(left.dt < right.dt) return true; return false; };
      qSort(m_pData.begin(), m_pData.end(), lessThan);
    }

    updateTooltip();
  }

  if(!m_pData.isEmpty() && m_weatherDialog)
  {
    m_weatherDialog->setPollutionData(m_pData);
  }
}

//--------------------------------------------------------------------
void TrayWeather::processOneCallData(const QByteArray &data)
{
  const auto jsonDocument = QJsonDocument::fromJson(data);

  if(!jsonDocument.isNull() && jsonDocument.isObject())
  {
    const auto currentDt = std::chrono::duration_cast<std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
    const auto jsonObj   = jsonDocument.object();
    const auto current   = jsonObj.value("current").toObject();

    UVData data;
    data.dt = current.value("dt").toInt(0);
    data.idx = current.value("uvi").toDouble(0);
    m_vData << data;

    auto hasEntry = [this](unsigned long dt) { for(auto entry: this->m_vData) if(entry.dt == dt) return true; return false; };

    if(jsonObj.keys().contains("hourly"))
    {
      auto uvList = jsonObj.value("hourly").toArray();

      for(int i = 0; i < uvList.count(); ++i)
      {
        const auto entry = uvList.at(i).toObject();

        const auto dt = entry.value("dt").toInt(0);
        if(dt < currentDt) continue;

        UVData data;
        data.dt = dt;
        data.idx = entry.value("uvi").toDouble(0);

        if(!hasEntry(data.dt)) m_vData << data;
      }
    }

    updateTooltip();

    if(m_configuration.showAlerts)
    {
      if(jsonObj.keys().contains("alerts"))
      {
        const auto alert = jsonObj.value("alerts").toObject();

        const bool isSame = (alert.value("event") == m_lastAlert.value("event") &&
                             alert.value("description") == m_lastAlert.value("description"));
        const bool isEmpty = alert.value("event").toString("").isEmpty();

        m_lastAlert = alert;

        contextMenu()->actions().at(8)->setEnabled(!isEmpty);

        if((!isSame || !m_lastAlertShown) && !isEmpty)
        {
          showAlert();
        }
      }
      else
      {
        contextMenu()->actions().at(8)->setEnabled(false);
        m_lastAlert = QJsonObject();
        m_lastAlertShown = false;
        onAlertDialogClosed();
      }
    }
  }

  if(!m_vData.isEmpty() && m_weatherDialog)
  {
    m_weatherDialog->setUVData(m_vData);
  }
}

//--------------------------------------------------------------------
bool NativeEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
  const auto msg = reinterpret_cast<MSG*>(message);

  if(msg->message == WM_POWERBROADCAST && msg->wParam == PBT_APMRESUMEAUTOMATIC && m_tw)
  {
    m_tw->requestData();
  }

  return false;
}

//--------------------------------------------------------------------
void TrayWeather::onAlertDialogClosed()
{
  if(m_alertsDialog)
  {
    m_lastAlertShown = m_alertsDialog->showAgain();

    if(m_alertsDialog->isVisible())
    {
      m_alertsDialog->blockSignals(true);
      m_alertsDialog->close();
    }

    delete m_alertsDialog;
    m_alertsDialog = nullptr;
  }
}

//--------------------------------------------------------------------
void TrayWeather::showAlert()
{
  if(!m_alertsDialog)
  {
    m_alertsDialog = new AlertDialog();
    connect(m_alertsDialog, SIGNAL(finished(int)), this, SLOT(onAlertDialogClosed()));
  }

  m_alertsDialog->setAlertData(m_lastAlert);
  m_alertsDialog->show();
}
