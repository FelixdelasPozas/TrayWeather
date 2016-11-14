/*
 File: ConfigurationDialog.h
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

#ifndef CONFIGURATIONDIALOG_H_
#define CONFIGURATIONDIALOG_H_

// Qt
#include "ui_ConfigurationDialog.h"
#include <QDialog>
#include <QNetworkAccessManager>

// C++
#include <memory>

class QNetworkReply;
class QDomNode;

/** \struct Configuration
 *
 */
struct Configuration
{
    double latitude;    /** location latitude in degrees.  */
    double longitude;   /** location longitude in degrees. */
    QString country;    /** location's country.            */
    QString region;     /** location's region.             */
    QString city;       /** location's city.               */
    QString zipcode;    /** location's zip code.           */
    QString isp;        /** internet service provider.     */
    QString ip;         /** internet address.              */
    QString timezone;   /** location's timezone.           */
    QString owm_apikey; /** OpenWeatherMap API Key.        */

    bool isValid() const
    {
      return (latitude <= 90.0) &&   (latitude >= -90.0) &&
             (longitude <= 180.0) && (longitude >= -180) &&
             !owm_apikey.isEmpty();

    }
};

class ConfigurationDialog
: public QDialog
, private Ui_ConfigurationDialog
{
    Q_OBJECT
  public:
    /** \brief LocationConfigDialog class constructor.
     * \param[in] configuration application configuration.
     * \param[in] parent pointer to the widget parent of this one.
     * \param[in] flags window flags.
     */
    ConfigurationDialog(const Configuration &configuration, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    /** \brief LocationConfigDialog class virtual destructor.
     *
     */
    virtual ~ConfigurationDialog()
    {}

    /** \brief Returns the configuration values.
     * \param[out] configuration application configuration values.
     *
     */
    void getConfiguration(Configuration &configuration) const;

  public slots:
    void replyFinished(QNetworkReply *reply);

  private:
    std::shared_ptr<QNetworkAccessManager> m_netManager;
};

#endif // CONFIGURATIONDIALOG_H_
