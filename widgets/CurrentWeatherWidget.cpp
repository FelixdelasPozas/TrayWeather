/*
 File: CurrentWeatherWidget.h
 Created on: 09/11/2025
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
#include <widgets/CurrentWeatherWidget.h>

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QToolTip>
#include <QPainter>
#include <QDateTime>
#include <QTimer>
#include <QScreen>
#include <QDateTime>

// C++
#include <cmath>
#include <ctime>

constexpr int PADDING = 20;

//-----------------------------------------------------------------------------
CurrentWeatherWidget::CurrentWeatherWidget(const ForecastData& data, const PollutionData& pData, const UVData& uData,
                                           const Configuration& config, const QPoint& trayPosition) :
    QWidget(QApplication::desktop(), Qt::WindowFlags()),
    m_trayPos{trayPosition}
{
    setupUi(this);
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    layout()->setMargin(3);
    layout()->setContentsMargins(QMargins{5, 5, 5, 5});

    m_icon->setPixmap(weatherPixmap(data, config.iconTheme, config.iconThemeColor)
                          .scaled(QSize{128, 128}, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // for translation
    const auto tempStr = tr("Temperature");
    const auto clouStr = tr("Cloudiness");
    const auto humiStr = tr("Humidity");
    const auto presStr = tr("Pressure");
    const auto rainStr = tr("Rain acc");
    const auto snowStr = tr("Snow acc");
    const auto uvStr = tr("UV");
    const auto riseStr = tr("Sunrise");
    const auto setStr = tr("Sunset");

    // Weather
    QString tempUnits, presUnits, accUnits;

    switch (config.units) {
        case Units::METRIC:
            tempUnits = "ºC";
            presUnits = tr("hPa");
            accUnits = tr("mm/h");
            break;
        case Units::IMPERIAL:
            tempUnits = "ºF";
            presUnits = tr("PSI");
            accUnits = tr("inch/h");
            break;
        case Units::CUSTOM:
            switch (config.tempUnits) {
                case TemperatureUnits::FAHRENHEIT:
                    tempUnits = "ºF";
                    break;
                default:
                case TemperatureUnits::CELSIUS:
                    tempUnits = "ºC";
                    break;
            }
            switch (config.pressureUnits) {
                case PressureUnits::INHG:
                    presUnits = tr("inHg");
                    break;
                case PressureUnits::MMGH:
                    presUnits = tr("mmHg");
                    break;
                case PressureUnits::PSI:
                    presUnits = tr("PSI");
                    break;
                default:
                case PressureUnits::HPA:
                    presUnits = tr("hPa");
                    break;
            }
            switch (config.precUnits) {
                case PrecipitationUnits::INCH:
                    accUnits = tr("inches/h");
                    break;
                default:
                case PrecipitationUnits::MM:
                    accUnits = tr("mm/h");
                    break;
            }
            break;
        default:
            break;
    }

    const auto dtTime = QDateTime::currentDateTimeUtc();
    m_dateTime->setText(toTitleCase(dtTime.toString("dddd dd/MM, hh:mm")));
    m_description->setText(toTitleCase(data.description));

    m_temperature->setText(QString("%1: %2 %3").arg(tempStr).arg(data.temp).arg(tempUnits));
    m_cloudiness->setText(QString("%1: %2%").arg(clouStr).arg(data.cloudiness));
    m_humidity->setText(QString("%1: %2%").arg(humiStr).arg(data.humidity));
    m_pressure->setText(QString("%1: %2 %3").arg(presStr).arg(data.pressure).arg(presUnits));

    if (data.rain == 0) {
        m_rain->hide();
    } else {
        m_rain->setText(QString("%1: %2 %3").arg(rainStr).arg(data.rain).arg(accUnits));
    }

    if (data.snow == 0) {
        m_snow->hide();
    } else {
        m_snow->setText(QString("%1: %2 %3").arg(snowStr).arg(data.snow).arg(accUnits));
    }

    // UV
    if (uData.isValid()) {
        const auto index = static_cast<int>(std::nearbyint(uData.idx));

        QString indexStr;
        switch (index) {
            case 0:
            case 1:
            case 2:
                indexStr = tr("Low");
                break;
            case 3:
            case 4:
            case 5:
                indexStr = tr("Moderate");
                break;
            case 6:
            case 7:
                indexStr = tr("High");
                break;
            case 8:
            case 9:
            case 10:
                indexStr = tr("Very high");
                break;
            default:
                indexStr = tr("Extreme");
                break;
        }

        const auto color = index == 0 ? "" : uvColor(index).darker(150).name();
        m_uv->setText(QString("%1: <font color=%2><b>%3</b></font>").arg(uvStr).arg(color).arg(indexStr));
    } else {
        m_uv->hide();
    }

    // Air
    if (pData.isValid()) {
        const auto airStr = tr("Air Quality");

        QString colorStr, qualityStr;
        switch (pData.aqi) {
            case 1:
                colorStr = "green";
                break;
            case 2:
                colorStr = "cyan";
                break;
            case 3:
                colorStr = "blue";
                break;
            case 4:
                colorStr = "purple";
                break;
            default:
                colorStr = "red";
                break;
        }
        qualityStr = pData.aqi_text;

        const auto color = QColor(colorStr).darker(150).name();
        m_air->setText(QString("%1:<font color=%2><b>%3</b></font>").arg(airStr).arg(color).arg(qualityStr));
    } else {
        m_air->hide();
    }

    struct tm t;
    unixTimeStampToDate(t, data.sunrise);
    const QDateTime riseTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
    m_sunrise->setText(QString("%1: %2").arg(riseStr).arg(riseTime.toString("hh:mm")));

    unixTimeStampToDate(t, data.sunset);
    const QDateTime setTime{QDate{t.tm_year + 1900, t.tm_mon + 1, t.tm_mday}, QTime{t.tm_hour, t.tm_min, t.tm_sec}};
    m_sunset->setText(QString("%1: %2").arg(setStr).arg(setTime.toString("hh:mm")));

    adjustSize();
    setFixedSize(size());
}

//--------------------------------------------------------------------
void CurrentWeatherWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.drawRect(0, 0, width() - 1, height() - 1);
    painter.end();

    QWidget::paintEvent(event);
}

//--------------------------------------------------------------------
void CurrentWeatherWidget::changeEvent(QEvent* e)
{
    if (e && e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
    }

    QWidget::changeEvent(e);
}

//--------------------------------------------------------------------
void CurrentWeatherWidget::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);

    auto point = computeWidgetPosition();
    move(point);

    QTimer::singleShot(durationInMs(), this, SLOT(onFinished()));
}

//--------------------------------------------------------------------
QPoint CurrentWeatherWidget::computeWidgetPosition()
{
    QPoint point;

    for (const auto screen : qApp->screens()) {
        if (screen->geometry().contains(m_trayPos)) {
            const auto screen_center = screen->geometry().center();
            Qt::Corner corner = Qt::TopLeftCorner;
            if (m_trayPos.x() > screen_center.x() && m_trayPos.y() <= screen_center.y()) {
                corner = Qt::TopRightCorner;
            } else if (m_trayPos.x() > screen_center.x() && m_trayPos.y() > screen_center.y()) {
                corner = Qt::BottomRightCorner;
            } else if (m_trayPos.x() <= screen_center.x() && m_trayPos.y() > screen_center.y()) {
                corner = Qt::BottomLeftCorner;
            }

            const auto hWidth = this->width() / 2;
            const auto hHeight = this->height() / 2;

            // Use available to avoid overlapping taskbar area.
            const auto available = screen->availableGeometry();

            // Compute top-left widget position to move it.
            int x, y;
            switch (corner) {
                case Qt::Corner::BottomLeftCorner: {
                    x = std::max(m_trayPos.x(), available.left() + PADDING + hWidth);
                    y = std::min(m_trayPos.y(), available.bottom() - PADDING - hHeight);
                } break;
                case Qt::Corner::BottomRightCorner: {
                    x = std::min(m_trayPos.x(), available.right() - PADDING - hWidth);
                    y = std::min(m_trayPos.y(), available.bottom() - PADDING - hHeight);
                } break;
                case Qt::Corner::TopLeftCorner: {
                    x = std::max(m_trayPos.x(), available.left() + PADDING + hWidth);
                    y = std::max(m_trayPos.y(), available.top() + PADDING + hHeight);
                } break;
                case Qt::Corner::TopRightCorner: {
                    x = std::min(m_trayPos.x(), available.right() - PADDING - hWidth);
                    y = std::max(m_trayPos.y(), available.top() + PADDING + hHeight);
                } break;
            }

            return QPoint(x - hWidth, y - hHeight);
        }
    }

    return point;
}

//--------------------------------------------------------------------
void CurrentWeatherWidget::onFinished()
{
    close();
    deleteLater();
}
