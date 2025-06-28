/*
 File: Updater.cpp
 Created on: 28/06/2025
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
#include <Updater.h>
#include <Utils.h>
#include <QGuiApplication>
#include <QScreen>
#include <QMessageBox>

constexpr int DEFAULT_LOGICAL_DPI = 96;

//----------------------------------------------------------------------------
TrayWeatherUpdater::TrayWeatherUpdater(const QUrl &url, QWidget* parent, Qt::WindowFlags f)
: QWidget{parent, f}
, m_url{url}
{
  setupUi(this);
  setWindowIcon(QIcon{":/TrayWeather/Updater.svg"});

  connect(cancelButton, SIGNAL(pressed()), this, SLOT(onCancelPressed()));
}

//----------------------------------------------------------------------------
void TrayWeatherUpdater::onCancelPressed()
{
  close();
}

//----------------------------------------------------------------------------
void TrayWeatherUpdater::showEvent(QShowEvent *e)
{
  QWidget::showEvent(e);
  const auto scale = (logicalDpiX() == DEFAULT_LOGICAL_DPI) ? 1. : 1.25;

  setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
  setMinimumSize(0, 0);

  adjustSize();
  setFixedSize(size() * scale);

  const auto screen = QGuiApplication::screens().at(0)->availableGeometry();
  move((screen.width()-size().width())/2,(screen.height()-size().height())/2);

  if(!m_url.isValid())
  {
      QMessageBox msgBox{this};
      msgBox.setWindowIcon(QIcon(":/TrayWeather/updater.svg"));
      msgBox.setWindowTitle(QObject::tr("TrayWeather Updater"));
      msgBox.setIcon(QMessageBox::Warning);
      msgBox.setText(QObject::tr("The url is invalid."));
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.exec();
      std::exit(0);
  }
}