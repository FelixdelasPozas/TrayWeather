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
#include <QFile>
#include <QImage>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QtWinExtras/QtWinExtrasDepends>
#include <QGraphicsBlurEffect>
#include <QDateTime>

// C++
#include <iostream>
#include <windows.h>
#include <processthreadsapi.h>

const QString RELEASES_ADDRESS = "https://api.github.com/repos/FelixdelasPozas/TrayWeather/releases";
QDateTime timeOfLastUpdate = QDateTime::currentDateTime();

QString m_url; // downloaded update file to launch in updater.

//--------------------------------------------------------------------
void launchUpdate()
{
  if(m_url.isEmpty())
    return;

  const auto appPath = qApp->applicationDirPath();
  const QDir appDir{appPath};
  const auto updaterPath = appDir.absoluteFilePath("Updater.exe");
  const auto commandLine = updaterPath + " " + m_url;
  std::string commandStr = commandLine.toStdString();

    LPSTARTUPINFO lpStartupInfo;
    LPPROCESS_INFORMATION lpProcessInfo;

    memset(&lpStartupInfo, 0, sizeof(lpStartupInfo));
    memset(&lpProcessInfo, 0, sizeof(lpProcessInfo));

    CreateProcess(NULL,               // No module name (use command line)
                  &commandStr[0],     // Command line
                  NULL,               // Process handle not inheritable
                  NULL,               // Thread handle not inheritable
                  FALSE,              // Set handle inheritance to FALSE
                  0,                  // No creation flags
                  NULL,               // Use parent's environment block
                  NULL,               // Use parent's starting directory
                  lpStartupInfo,      // Pointer to STARTUPINFO structure
                  lpProcessInfo);     // Pointer to PROCESSINFO structure
}

//--------------------------------------------------------------------
TrayWeather::TrayWeather(Configuration& configuration, QObject* parent)
: QSystemTrayIcon {parent}
, m_configuration {configuration}
, m_netManager    {std::make_shared<NetworkAccessManager>(this)}
, m_timer         {this}
, m_weatherDialog {nullptr}
, m_aboutDialog   {nullptr}
, m_configDialog  {nullptr}
, m_additionalTray{nullptr}
, m_eventFilter   {this}
, m_provider      {nullptr}
{
  m_timer.setSingleShot(true);

  qApp->installNativeEventFilter(&m_eventFilter);

  if(!m_configuration.providerId.isEmpty())
  {
    m_provider = WeatherProviderFactory::createProvider(m_configuration.providerId, m_configuration);
  }

  connectSignals();

  createMenuEntries();

  updateTooltip();

  updateMenuActions();

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
  const auto originUrl = reply->request().url().toString();

  if(originUrl.contains("github", Qt::CaseInsensitive))
  {
    if(reply->error() == QNetworkReply::NoError)
    {
      const auto contents  = reply->readAll();
      processGithubData(contents);
    }
    else
    {
      const auto errorText = tr("Error: ") + "Github.";
      setErrorTooltip(errorText);
    }
  }

  if(originUrl.contains("ip-api", Qt::CaseInsensitive))
  {
    if(reply->error() == QNetworkReply::NoError)
    {
      const auto contents  = reply->readAll();
      processGeolocationData(contents, originUrl.contains("edns", Qt::CaseInsensitive));
    }
    else
    {
      const auto errorText = tr("Error: ") + tr("No geolocation.");
      setErrorTooltip(errorText);
    }
  }

  if(m_provider)
  {
    m_provider->processReply(reply);
  }

  reply->deleteLater();
}

//--------------------------------------------------------------------
void TrayWeather::showConfiguration()
{
  if(m_aboutDialog)
    m_aboutDialog->close(); 

  if(m_weatherDialog)
    m_weatherDialog->close();

  if(m_configDialog)
  {
    m_configDialog->raise();
    return;
  }

  m_configDialog = new ConfigurationDialog{m_configuration};

  connect(m_configDialog, SIGNAL(languageChanged(const QString &)), this, SLOT(onLanguageChanged(const QString &)));

  const auto scr = QApplication::desktop()->screenGeometry();
  m_configDialog->move(scr.center() - m_configDialog->rect().center());
  m_configDialog->setModal(true);
  if(m_current.isValid()) m_configDialog->setCurrentTemperature(std::nearbyint(m_current.temp));
  else m_configDialog->setCurrentTemperature(15);
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

  m_configuration.lightTheme      = configuration.lightTheme;
  m_configuration.iconType        = configuration.iconType;
  m_configuration.iconTheme       = configuration.iconTheme;
  m_configuration.iconThemeColor  = configuration.iconThemeColor;
  m_configuration.trayTextColor   = configuration.trayTextColor;
  m_configuration.trayTextMode    = configuration.trayTextMode;
  m_configuration.trayTextBorder  = configuration.trayTextBorder;
  m_configuration.trayBorderWidth = configuration.trayBorderWidth;
  m_configuration.trayTextDegree  = configuration.trayTextDegree;
  m_configuration.trayTextFont    = configuration.trayTextFont;
  m_configuration.trayFontSpacing = configuration.trayFontSpacing;
  m_configuration.trayBackAuto    = configuration.trayBackAuto;
  m_configuration.trayBackColor   = configuration.trayBackColor;
  m_configuration.trayBorderAuto  = configuration.trayBorderAuto;
  m_configuration.trayBorderColor = configuration.trayBorderColor;
  m_configuration.stretchTempIcon = configuration.stretchTempIcon;
  m_configuration.minimumColor    = configuration.minimumColor;
  m_configuration.maximumColor    = configuration.maximumColor;
  m_configuration.minimumValue    = configuration.minimumValue;
  m_configuration.maximumValue    = configuration.maximumValue;
  m_configuration.autostart       = configuration.autostart;
  m_configuration.language        = configuration.language;
  m_configuration.tooltipFields   = configuration.tooltipFields;
  m_configuration.showAlerts      = configuration.showAlerts;
  m_configuration.keepAlertIcon   = configuration.keepAlertIcon;
  m_configuration.swapTrayIcons   = configuration.swapTrayIcons;
  m_configuration.trayIconSize    = configuration.trayIconSize;
  m_configuration.tempRepr        = configuration.tempRepr;
  m_configuration.rainRepr        = configuration.rainRepr;
  m_configuration.snowRepr        = configuration.snowRepr;
  m_configuration.tempReprColor   = configuration.tempReprColor;
  m_configuration.rainReprColor   = configuration.rainReprColor;
  m_configuration.snowReprColor   = configuration.snowReprColor;
  m_configuration.tempMapOpacity  = configuration.tempMapOpacity;
  m_configuration.cloudMapOpacity = configuration.cloudMapOpacity;
  m_configuration.rainMapOpacity  = configuration.rainMapOpacity;
  m_configuration.windMapOpacity  = configuration.windMapOpacity;
  m_configuration.barWidth        = configuration.barWidth;

  bool requestNewData = false;

  if(changedLanguage) requestNewData = true;

  if(configuration.isValid())
  {
    auto menu = this->contextMenu();

    if(menu && menu->actions().size() > 1)
    {
      QString iconLink = temperatureIconString(configuration);
      QIcon icon{iconLink};

      menu->actions().at(0)->setIcon(icon);
    }

    const auto changedCoords      = (configuration.latitude != m_configuration.latitude) || (configuration.longitude != m_configuration.longitude);
    const auto changedMethod      = (configuration.useGeolocation != m_configuration.useGeolocation);
    const auto changedIP          = (configuration.ip != m_configuration.ip);
    const auto changedUpdateTime  = (configuration.updateTime != m_configuration.updateTime);
    const auto changedUpdateCheck = (configuration.update != m_configuration.update);
    const auto changedUnits       = (configuration.units != m_configuration.units) ||
                                    (configuration.units == Units::CUSTOM && (configuration.tempUnits != m_configuration.tempUnits ||
                                                                              configuration.precUnits != m_configuration.precUnits ||
                                                                              configuration.windUnits != m_configuration.windUnits ||
                                                                              configuration.pressureUnits != m_configuration.pressureUnits));
                                                                              
    const auto changedProvider      = (configuration.providerId != m_configuration.providerId);

    if(changedIP || changedMethod || changedCoords || changedRoaming)
    {
      m_configuration.country        = configuration.country;
      m_configuration.region         = configuration.region;
      m_configuration.city           = configuration.city;
      m_configuration.ip             = configuration.ip;
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
      requestNewData = true;
    }

    if(changedProvider)
    {
      disconnectProviderSignals();
      m_provider = WeatherProviderFactory::createProvider(configuration.providerId, m_configuration);
      connectProviderSignals();
      updateMenuActions();

      m_configuration.providerId = configuration.providerId;
      m_configuration.lastTab = 0; // Different providers have different tabs.
      requestNewData = true;
    }

    if(changedUnits)
    {
      m_configuration.units         = configuration.units;
      m_configuration.tempUnits     = configuration.tempUnits;
      m_configuration.pressureUnits = configuration.pressureUnits;
      m_configuration.windUnits     = configuration.windUnits;
      m_configuration.precUnits     = configuration.precUnits;
      requestNewData = true;
    }

    if(m_weatherDialog)
    {
      const auto capabilites = m_provider->capabilities();
      if(!requestNewData)
      {
        m_weatherDialog->setWeatherProvider(m_provider);

        if(validData())
        {
          m_weatherDialog->setWeatherData(m_current, capabilites.hasWeatherForecast ? m_data : Forecast(), m_configuration);
        }

        if(capabilites.hasPollutionForecast && !m_pData.isEmpty())
        {
          m_weatherDialog->setPollutionData(m_pData);
        }

        if(capabilites.hasUVForecast && !m_vData.isEmpty())
        {
          m_weatherDialog->setUVData(m_vData);
        }

        if(capabilites.hasAlerts && !m_alerts.isEmpty())
        {
          removeExpiredAlerts();
          if(m_alerts.isEmpty())
            m_weatherDialog->setAlerts(m_alerts);
        }
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
    msgBox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
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
      m_additionalTray->setIcon(QIcon{":/TrayWeather/network_refresh.svg"});
      m_additionalTray->setVisible(true);

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
    }

    return;
  }

  if(m_configuration.showAlerts && !m_alerts.isEmpty() && m_provider && m_provider->capabilities().hasAlerts)
  {
    removeExpiredAlerts();

    if(!m_alerts.empty())
    {
      auto it = std::find_if(m_alerts.cbegin(), m_alerts.cend(), [](const Alert &a) { return !a.seen; });
      if(it != m_alerts.cend() || m_configuration.keepAlertIcon)
      {
        const auto alertIcon = QIcon{":/TrayWeather/alert.svg"};
        const QString msg = tr("There is a weather alert for your location!");

        setIcon(alertIcon);
        setToolTip(msg);

        if (m_additionalTray)
        {
          m_additionalTray->setIcon(alertIcon);
          m_additionalTray->setToolTip(msg);
        }

        return;
      }
    }
  }

  tooltip = tooltipText();

  QPixmap pixmap = weatherPixmap(m_current, m_configuration.iconTheme, m_configuration.iconThemeColor).scaled(384,384,Qt::KeepAspectRatio, Qt::SmoothTransformation);

  auto interpolate = [this](const int temp)
  {
    const auto minColor = m_configuration.minimumColor;
    const auto maxColor = m_configuration.maximumColor;
    const auto value = std::min(m_configuration.maximumValue, std::max(m_configuration.minimumValue, temp));

    const double inc = static_cast<double>(value-m_configuration.minimumValue)/(m_configuration.maximumValue - m_configuration.minimumValue);
    const double rInc = (maxColor.red()   - minColor.red())   * inc;
    const double gInc = (maxColor.green() - minColor.green()) * inc;
    const double bInc = (maxColor.blue()  - minColor.blue())  * inc;

    return QColor::fromRgb(minColor.red() + rInc, minColor.green() + gInc, minColor.blue() + bInc, 255);
  };

  switch(m_configuration.iconType)
  {
    case 0:
      break;
    case 3:
      if(m_additionalTray)
      {
        if (!m_configuration.trayBackAuto)
          pixmap = setIconBackground(m_configuration.trayBackColor, pixmap);

        icon = QIcon(pixmap);

        if(m_configuration.swapTrayIcons)
          setIcon(icon);
        else
          m_additionalTray->setIcon(icon);
      }
      /* fall through */
    case 1:
      pixmap.fill(Qt::transparent);
      /* fall through */
    default:
    case 2:
      {
        QPixmap tempPixmap{384,384};
        tempPixmap.fill(Qt::transparent);
        QPainter painter(&tempPixmap);

        const auto roundedTemp = static_cast<int>(std::nearbyint(m_current.temp));
        const auto roundedString = QString::number(roundedTemp) + (m_configuration.trayTextDegree ? QString::fromUtf8("\u00B0") : QString());

        QFont font;
        font.fromString(m_configuration.trayTextFont);
        font.setPixelSize(200);
        font.setLetterSpacing(QFont::AbsoluteSpacing, m_configuration.trayFontSpacing);
        painter.setFont(font);

        QColor color;
        if(m_configuration.trayTextMode)
          color = m_configuration.trayTextColor;
        else
          color = interpolate(roundedTemp);

        painter.setPen(color);
        painter.setRenderHint(QPainter::RenderHint::TextAntialiasing, true);
        painter.setRenderHint(QPainter::RenderHint::HighQualityAntialiasing, true);
        painter.drawText(tempPixmap.rect(), Qt::AlignCenter, roundedString);

        if(m_configuration.trayTextBorder)
        {
          const auto invertedColor = QColor{color.red() ^ 0xFF, color.green() ^ 0xFF, color.blue() ^ 0xFF};
          const auto customColor = QColor(m_configuration.trayTextColor);

          //constructing temporal object only to get path for border.
          QGraphicsPixmapItem tempItem(tempPixmap);
          tempItem.setShapeMode(QGraphicsPixmapItem::MaskShape);
          const auto path = tempItem.shape();

          QPen pen(m_configuration.trayBorderAuto ? invertedColor : customColor, m_configuration.trayBorderWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
          painter.setPen(pen);
          painter.drawPath(path);

          // repaint the temperature, it was overwritten by path.
          painter.setPen(color);
          painter.drawText(tempPixmap.rect(), Qt::AlignCenter, roundedString);
        }
        painter.end();

        const auto rect = computeDrawnRect(tempPixmap.toImage());
        if(rect.isValid())
        {
          tempPixmap = blurPixmap(tempPixmap, 5);

          const auto difference = (pixmap.rect().center() - rect.center())/2.;
          double ratioX = pixmap.width() * 1.0 / rect.width();
          double ratioY = pixmap.height() * 1.0 / rect.height();
          ratioX = std::min(ratioX, ratioY);
          ratioY = m_configuration.stretchTempIcon ? ratioY : ratioX;

          const auto ratio = static_cast<double>(m_configuration.trayIconSize) / 100.;
          if(ratio != 1 && ratio >= 0.5)
          {
            ratioX *= ratio;
            ratioY *= ratio;
          }

          painter.begin(&pixmap);
          painter.translate(rect.center());
          painter.scale(ratioX, ratioY);
          painter.translate(-rect.center()+difference);
          painter.drawImage(QPoint{0,0}, tempPixmap.toImage());
          painter.end();
        }
      }
      break;
  }

  if(!m_configuration.trayBackAuto)
    pixmap = setIconBackground(m_configuration.trayBackColor, pixmap);

  icon = QIcon(pixmap);
  setToolTip(tooltip);
  
  if(m_additionalTray)
    m_additionalTray->setToolTip(tooltip);

  if (m_configuration.swapTrayIcons && m_additionalTray)
    m_additionalTray->setIcon(icon);
  else
    setIcon(icon);
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
          fieldsText << QString::number(m_current.temp, 'f', 1) + temperatureIconText(m_configuration);
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
              pressUnits = tr("inHg");
              break;
            case PressureUnits::MMGH:
              pressUnits = tr("mmHg");
              break;
            case PressureUnits::PSI:
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
              windUnits = tr("ft/s");
              break;
            case WindUnits::KMHR:
              windUnits = tr("km/h");
              break;
            case WindUnits::MILHR:
              windUnits = tr("mph");
              break;
            case WindUnits::KNOTS:
              windUnits = tr("kts");
              break;
            default:
            case WindUnits::METSEC:
              windUnits = tr("m/s");
              break;
          }
          fieldsText << tr("Wind: ") + QString("%1 %2").arg(windValue).arg(windUnits);
        }
        break;
      case TooltipText::WIND_DIR:
        fieldsText << tr("Wind direction: ") + QString("%1º (%2)").arg(static_cast<int>(m_current.wind_dir) % 360).arg(windDegreesToName(m_current.wind_dir));
        break;
      case TooltipText::UPDATE_TIME:
        {
          QString text = tr("Last updated: ");
          if(timeOfLastUpdate == QDateTime())
            text += "Never";
          else
            text += timeOfLastUpdate.toString("hh:mm:ss");

          fieldsText << text;
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
          fieldsText << tr("Air: ") + m_pData.first().aqi_text;
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
      case TooltipText::UV:
        if(!m_vData.empty())
        {
          const auto index = static_cast<int>(std::nearbyint(m_vData.first().idx));
          fieldsText << QString("UV: ") + QString("%1").arg(index);
        }
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
  connect(refresh, SIGNAL(triggered(bool)), this, SLOT(forceRequestData()));

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
  if(m_configDialog)
  {
     m_configDialog->raise();
     return;
  }

  if(m_weatherDialog)
     m_weatherDialog->close();

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

  connectProviderSignals();
}

//--------------------------------------------------------------------
void TrayWeather::connectProviderSignals()
{
  if(m_provider)          
  {
    connect(m_provider.get(), SIGNAL(weatherDataReady()),            this, SLOT(processWeatherData()));
    connect(m_provider.get(), SIGNAL(weatherForecastDataReady()),    this, SLOT(processWeatherData()));
    connect(m_provider.get(), SIGNAL(pollutionForecastDataReady()),  this, SLOT(processPollutionData()));
    connect(m_provider.get(), SIGNAL(uvForecastDataReady()),         this, SLOT(processUVData()));
    connect(m_provider.get(), SIGNAL(errorMessage(const QString &)), this, SLOT(setErrorTooltip(const QString &)));
    connect(m_provider.get(), SIGNAL(weatherAlerts(const Alerts &)), this, SLOT(processAlerts(const Alerts &)));
  }  
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

  disconnectProviderSignals();             
}

//--------------------------------------------------------------------
void TrayWeather::disconnectProviderSignals()
{
  if(m_provider)          
  {
    disconnect(m_provider.get(), SIGNAL(weatherDataReady()),            this, SLOT(processWeatherData()));
    disconnect(m_provider.get(), SIGNAL(weatherForecastDataReady()),    this, SLOT(processWeatherData()));
    disconnect(m_provider.get(), SIGNAL(pollutionForecastDataReady()),  this, SLOT(processPollutionData()));
    disconnect(m_provider.get(), SIGNAL(uvForecastDataReady()),         this, SLOT(processUVData()));
    disconnect(m_provider.get(), SIGNAL(errorMessage(const QString &)), this, SLOT(setErrorTooltip(const QString &)));
    disconnect(m_provider.get(), SIGNAL(weatherAlerts(const Alerts &)), this, SLOT(processAlerts(const Alerts &)));
  }  
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
  if(m_aboutDialog)
    m_aboutDialog->close();

  if(m_configDialog)
  {
    m_configDialog->raise();
    return;
  }

  static int lastTab = 0;

  const auto caller = qobject_cast<QAction *>(sender());
  if(caller)
  {
    auto actions = contextMenu()->actions();

    for(const auto i: {0,1,2,3,4,8})
    {
      if(caller == actions.at(i))
      {
        // alerts tab has not the same number as action.
        lastTab = i == 8 ? 5 : i; 
        break;
      }
    }
  }
  else
  {
    lastTab = m_configuration.lastTab;
  }

  if(lastTab == 5)
  {
    removeExpiredAlerts();
    if(m_alerts.isEmpty())
      lastTab = 0;
  }

  m_configuration.lastTab = lastTab;

  if(m_weatherDialog)
  {
    m_weatherDialog->setWeatherData(m_current, m_data, m_configuration);
    m_weatherDialog->setPollutionData(m_pData);
    m_weatherDialog->setUVData(m_vData);
    m_weatherDialog->setAlerts(m_alerts);

    m_weatherDialog->raise();
    m_weatherDialog->m_tabWidget->setCurrentIndex(lastTab);
    return;
  }

  if(!validData())
  {
    QMessageBox msgBox;
    msgBox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    msgBox.setWindowTitle(QObject::tr("Tray Weather"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(QObject::tr("TrayWeather has requested the weather data for your geographic location\nand it's still waiting for the response."));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();

    return;
  }

  updateTooltip();

  m_weatherDialog = new WeatherDialog{m_provider};
  m_weatherDialog->setWeatherData(m_current, m_data, m_configuration);
  m_weatherDialog->setPollutionData(m_pData);
  m_weatherDialog->setUVData(m_vData);
  m_weatherDialog->setAlerts(m_alerts);

  connect(m_weatherDialog, SIGNAL(mapsEnabled(bool)), this, SLOT(onMapsStateChanged(bool)));
  connect(m_weatherDialog, SIGNAL(alertsSeen()), this, SLOT(onAlertsSeen()));

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
  updateNetworkManager();

  if(m_timer.isActive()) m_timer.stop();
  m_timer.setInterval(m_configuration.updateTime*60*1000);
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
  updateNetworkManager();

  if(m_provider)
    m_provider->requestData(m_netManager);
}

//--------------------------------------------------------------------
bool TrayWeather::validData() const
{
  return !m_data.isEmpty() && m_current.isValid();
}

//--------------------------------------------------------------------
void TrayWeather::forceRequestData()
{
  m_data.clear();
  m_pData.clear();
  m_vData.clear();
  m_current = ForecastData();

  updateTooltip();
  requestData();
}

//--------------------------------------------------------------------
void TrayWeather::requestGeolocation()
{
  // No need to request weather data here. We'll do that on replyRequest() when obtaining the geolocation.
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
  m_configuration.mapsEnabled = value;
  const auto actions = contextMenu()->actions();
  actions.at(4)->setEnabled(value);
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
    if(m_updatesTimer.isActive()) m_updatesTimer.stop();
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
      auto installButton = msgBox.addButton(tr("Install"), QMessageBox::ButtonRole::NoRole);
      installButton->setIcon(QIcon(":/TrayWeather/updater.svg"));
      msgBox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
      msgBox.setIconPixmap(QIcon(":/TrayWeather/application.svg").pixmap(QSize{64,64}));
      msgBox.exec();

      if(msgBox.clickedButton() == installButton)
      {
          const auto assets = lastRelease.value("assets").toArray();
          for (int i = 0; i < assets.size(); ++i) {
              const auto asset = assets.at(i).toObject();
              m_url = asset.value("browser_download_url").toString();
              if(m_url.endsWith(".exe", Qt::CaseInsensitive))
                break;
          }

          if (!m_url.endsWith(".exe", Qt::CaseInsensitive)) {
              QMessageBox msgBox;
              msgBox.setWindowTitle(title);
              msgBox.setText(tr("Unable to locate valid asset in Github. Install aborted."));
              msgBox.setDetailedText(details);
              msgBox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
              msgBox.setIconPixmap(QIcon(":/TrayWeather/application.svg").pixmap(QSize{64, 64}));
              msgBox.exec();
          }

          std::atexit(launchUpdate);
          exitApplication();
      }
    }
  }
  else
  {
    auto githubError = tr("Error: ") + "Github.";
    const auto tooltipText = toolTip();

    if(!tooltipText.contains(githubError, Qt::CaseSensitive) && (tooltipText.length() + githubError.length() <= 125))
    {
      const auto finalText = tooltipText + "\n\n" + githubError;
      setToolTip(finalText);
      if(m_additionalTray)
        m_additionalTray->setToolTip(finalText);
    }
  }
}

//--------------------------------------------------------------------
void TrayWeather::processWeatherData()
{
  if(m_provider)
  {
    const auto current = m_provider->weather();
    const auto forecast = m_provider->weatherForecast();

    if(current.dt != 0) // checks if the returned value is Forecast() empty constructor.
      m_current = current;

    if(!forecast.isEmpty())
      m_data = forecast;

    timeOfLastUpdate = QDateTime::currentDateTime();
    updateTooltip();
    
    if(m_weatherDialog)
      m_weatherDialog->setWeatherData(m_current, m_data, m_configuration);
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
      m_configuration.latitude  = values.at(7).toDouble();
      m_configuration.longitude = values.at(8).toDouble();
      m_configuration.ip        = values.at(13);

      requestForecastData();
    }
    else
    {
      const auto errorIcon = QIcon{":/TrayWeather/network_error.svg"};
      const auto tooltip = tr("Error: ") + tr("No geolocation.");

      setIcon(errorIcon);
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
void TrayWeather::processPollutionData()
{
  if(m_provider)
  {
    const auto forecast = m_provider->pollutionForecast();

    if(!forecast.isEmpty())
      m_pData = forecast;
  
    timeOfLastUpdate = QDateTime::currentDateTime();
    updateTooltip();

    if(m_weatherDialog)
      m_weatherDialog->setPollutionData(m_pData);
  }
}

//--------------------------------------------------------------------
void TrayWeather::processUVData()
{
  if(m_provider)
  {
    const auto forecast = m_provider->uvForecast();

    if(!forecast.isEmpty())
    {
      m_vData = forecast;

      timeOfLastUpdate = QDateTime::currentDateTime();
      updateTooltip();

      if(m_weatherDialog)
        m_weatherDialog->setUVData(m_vData);
    }
  }
}

//--------------------------------------------------------------------
void TrayWeather::processAlerts(const Alerts &alerts)
{
  if(m_configuration.showAlerts)
  {
    auto filterAlerts = [&](const Alert &alert)
    {
      if(!m_alerts.contains(alert))
        m_alerts << alert;
    };
    std::for_each(alerts.cbegin(), alerts.cend(), filterAlerts);

    removeExpiredAlerts();

    Alerts notShown;
    auto filterNotShown = [&](const Alert &alert)
    {
      if(!alert.seen) notShown << alert;
    };
    std::for_each(m_alerts.cbegin(), m_alerts.cend(), filterNotShown);

    if(!m_alerts.isEmpty())
    {
      if(!notShown.isEmpty() || m_configuration.keepAlertIcon)
      {
        const auto actions = contextMenu()->actions();
        actions.at(8)->setEnabled(!notShown.isEmpty());
        actions.at(8)->setText(tr("Last alert...") + QString(" (%1)").arg(notShown.count()));

        updateTooltip();
      }
    }
  }
}

//--------------------------------------------------------------------
void TrayWeather::setErrorTooltip(const QString &msg)
{
  const auto tooltipText = toolTip();

  if (!tooltipText.contains(msg, Qt::CaseSensitive) && (tooltipText.length() + msg.length() <= 125))
  {
    const auto finalText = tooltipText + "\n\n" + msg;
    setToolTip(finalText);

    if (m_additionalTray)
      m_additionalTray->setToolTip(finalText);
  }
}

//--------------------------------------------------------------------
void TrayWeather::updateNetworkManager()
{
  if(m_netManager && (m_netManager->networkAccessible() != QNetworkAccessManager::Accessible))
  {
    disconnect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
               this,               SLOT(replyFinished(QNetworkReply*)));
               
    m_netManager = nullptr;           
  }
  
  if(!m_netManager)
  {
    m_netManager = std::make_shared<NetworkAccessManager>(this);

    connect(m_netManager.get(), SIGNAL(finished(QNetworkReply*)),
            this,               SLOT(replyFinished(QNetworkReply*)));
  }
}

//--------------------------------------------------------------------
void TrayWeather::updateMenuActions()
{
  if(m_provider)
  {
    const auto capabilities = m_provider->capabilities();
    const auto actions = contextMenu()->actions();
    actions.at(1)->setVisible(capabilities.hasWeatherForecast); // Forecast...
    actions.at(2)->setVisible(capabilities.hasPollutionForecast); // Pollution...
    actions.at(3)->setVisible(capabilities.hasUVForecast); // UV...
    actions.at(4)->setVisible(capabilities.hasMaps); // Maps...
    actions.at(7)->setVisible(capabilities.hasAlerts); // Separator
    actions.at(8)->setVisible(capabilities.hasAlerts); // Last alert...
  }
}

//--------------------------------------------------------------------
void TrayWeather::removeExpiredAlerts()
{
  const long long unsigned int currentDt = QDateTime::currentDateTimeUtc().currentMSecsSinceEpoch() / 1000;
  
  auto isExpired = [currentDt](const Alert &a)
  { return currentDt > a.endTime; };

  // remove expired Alerts.
  QMutableListIterator<Alert> i(m_alerts);
  while (i.hasNext())
  {
    if (isExpired(i.next()))
      i.remove();
  }
}

//--------------------------------------------------------------------
void TrayWeather::onAlertsSeen()
{
  auto actions = this->contextMenu()->actions();
  actions.at(8)->setEnabled(!m_alerts.empty());
  actions.at(8)->setText(tr("Last alert..."));
  std::for_each(m_alerts.begin(), m_alerts.end(), [](Alert &a){ a.seen = true; });
  
  updateTooltip(); 
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
void TrayWeather::showAlert()
{
  removeExpiredAlerts();

  if(!m_alerts.isEmpty())
    m_configuration.lastTab = 5;

  showTab();
}
