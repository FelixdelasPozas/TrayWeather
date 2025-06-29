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
#include <QDir>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QTimer>

// C++
#include <windows.h>
#include <processthreadsapi.h>
#include <string>

constexpr int DEFAULT_LOGICAL_DPI = 96;

std::string downloadedFile;

//----------------------------------------------------------------------------
void launchUpdate()
{
    LPSTARTUPINFO lpStartupInfo;
    LPPROCESS_INFORMATION lpProcessInfo;

    memset(&lpStartupInfo, 0, sizeof(lpStartupInfo));
    memset(&lpProcessInfo, 0, sizeof(lpProcessInfo));

    CreateProcess(NULL,               // No module name (use command line)
                  &downloadedFile[0], // Command line
                  NULL,               // Process handle not inheritable
                  NULL,               // Thread handle not inheritable
                  FALSE,              // Set handle inheritance to FALSE
                  0,                  // No creation flags
                  NULL,               // Use parent's environment block
                  NULL,               // Use parent's starting directory
                  lpStartupInfo,      // Pointer to STARTUPINFO structure
                  lpProcessInfo);     // Pointer to PROCESSINFO structure
}

//----------------------------------------------------------------------------
TrayWeatherUpdater::TrayWeatherUpdater(const QUrl &url, QWidget* parent, Qt::WindowFlags f)
: QWidget{parent, f}
, m_url{url}
, m_netManager{this}
{
  setupUi(this);
  setWindowIcon(QIcon{":/TrayWeather/Updater.svg"});
  progressBar->setRange(0,100);
  progressBar->setValue(0);
  progressBar->setToolTip("Operation progress");

  connect(cancelButton, SIGNAL(pressed()), 
          this,         SLOT(onCancelPressed()));
  connect(&m_netManager, SIGNAL(finished(QNetworkReply*)), 
          this,          SLOT(fileDownloaded(QNetworkReply*)));
}

//----------------------------------------------------------------------------
void TrayWeatherUpdater::onCancelPressed()
{
  close();
  std::exit(0);
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

  fileLabel->setText(m_url.fileName());

  QTimer::singleShot(100, SLOT(startDownload()));
}

//----------------------------------------------------------------------------
void TrayWeatherUpdater::startDownload()
{
  QNetworkRequest request(m_url);
  auto reply = m_netManager.get(request);  

  connect(reply, SIGNAL(downloadProgress(qint64, qint64)), 
          this,  SLOT(onProgressChanged(qint64, qint64)));
}

//----------------------------------------------------------------------------
void TrayWeatherUpdater::fileDownloaded(QNetworkReply *reply)
{
    progressBar->setValue(100);

    if (reply->error()) {
        QMessageBox msgBox{this};
        msgBox.setWindowIcon(QIcon(":/TrayWeather/updater.svg"));
        msgBox.setWindowTitle(QObject::tr("TrayWeather Updater"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(QObject::tr("Error downloading the update file."));
        msgBox.setDetailedText(reply->errorString());
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        std::exit(0);
    } else {
        m_file = QDir::temp().absoluteFilePath(m_url.fileName());
        downloadedFile = m_file.toStdString();

        QFile file(m_file);
        if (file.open(QFile::Append|QFile::Truncate)) {
            file.write(reply->readAll());
            file.flush();
            file.close();
        }
    }

    reply->deleteLater();

    std::atexit(launchUpdate);

    close();
}

//----------------------------------------------------------------------------
void TrayWeatherUpdater::onProgressChanged(qint64 current, qint64 total)
{
  const int value = std::min(0ll, std::max(100ll, (current * 100)/total));
  progressBar->setValue(value);
}
