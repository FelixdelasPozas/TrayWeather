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
#include <Utils.h>

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

  protected:
    virtual void showEvent(QShowEvent *e);

  private slots:
    /** \brief Aborts the download and exits the updater application. 
     */
    void onCancelPressed();

    /** \brief Starts the download of the url.
     */
    void startDownload();

    /** \brief Starts the update process when the file has been downloaded.
     * \param[in] reply Network reply.
     *
     */
    void fileDownloaded(QNetworkReply *reply);

    /** \brief Updates the progress bar while the url is being downlaoded.
     * \param[in] current Current downloaded byted.
     * \param[in] total Total of bytes to be downloaded.
     *
     */
    void onProgressChanged(qint64 current, qint64 total);

  private:
    /** \brief Unzips the portable file in the current directory. 
     *
     */
    void unzipRelease();

    /** \brief Shows the error message and exits the application.
     * \param[in] msg Error message.
     * \param[in] details Details of the error message.
     *
     */
    void showErrorAndExit(const QString &msg, const QString &details = QString());

    const QUrl m_url;                  /** download url. */
    QString m_file;                    /** downloaded file. */
    NetworkAccessManager m_netManager; /** network access manager. */
    QByteArray m_DownloadedData;       /** contents of downloaded file. */
};

#endif