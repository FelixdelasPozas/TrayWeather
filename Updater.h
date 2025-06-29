/*
 File: Updater.h
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

#ifndef _TW_UPDATER_
#define _TW_UPDATER_

// Project
#include <Ui_Updater.h>

// Qt
#include <QWidget>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QByteArray>

class QNetworkReply;

/** \class TrayWeatherUpdater
 * \brief Implements the dialog that downloads the update
 */
class TrayWeatherUpdater
: public QWidget
, private Ui::Updater
{
    Q_OBJECT
  public:
    /** \brief TrayWeatherUpdater class constructor. 
     * \param[in] parent Raw pointer of the QWidget parent of this one.
     * \param[in] f Window flags.
     */
    TrayWeatherUpdater(const QUrl &url, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /** \brief TrayWeatherUpdater class virtual destructor. 
     */
    virtual ~TrayWeatherUpdater() {};

    QString getDownloadedFilePath() const
    { return m_file; }

  protected:
    virtual void showEvent(QShowEvent *e);

  private slots:
    void onCancelPressed();
    void startDownload();
    void fileDownloaded(QNetworkReply*);
    void onProgressChanged(qint64 current, qint64 total);

  private:
    const QUrl m_url; /** download url. */
    QString m_file; /** downloaded file. */
    QNetworkAccessManager m_netManager;
    QByteArray m_DownloadedData;    
};

#endif