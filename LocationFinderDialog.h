/*
 File: LocationFinderDialog.h
 Created on: 16/04/2024
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

#ifndef __LOCATION_FINDER_DIALOG_H_
#define __LOCATION_FINDER_DIALOG_H_

// Project
#include "ui_LocationFinderDialog.h"
#include <Utils.h>

// Qt
#include <QDialog>
#include <QFile>

class QNetworkAccessManager;
class QNetworkReply;

/** \class LocationFinderDialog
 * \brief Dialog to enter a location and interact with OpenWeatherMap geolocation results. 
 * 
 */
class LocationFinderDialog
: public QDialog
, private Ui::LocationFinderDialog
{
  Q_OBJECT
    public:
      /** \brief LocationFinderDialog class constructor. 
       * \param[in] apiKey OpenWeathermap valid api key.
       * \param[in] langCode Language code.
       * \param[in] manager QNetworkAccessManager from the ConfigurationDialog. Not null.
       * \param[in] parent Raw pointer of the widget parent of this one. 
       * \param[in] f Window flags. 
       * 
      */
      LocationFinderDialog(const QString &apiKey, const QString &langCode, QNetworkAccessManager* manager, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

      /** \brief LocationFinderDialog class virtual destructor. 
       * 
      */
      virtual ~LocationFinderDialog()
      {};

      /** \struct Location
       * \brief Just a struct to return the selected location information.
       * 
       */
      struct Location
      {
        QString location;  /** location name.       */
        float   latitude;  /** latitude coordinate  */
        float   longitude; /** longitude coordinate */
        QString country;   /** country              */
        QString region;    /** region/state         */

        explicit Location(const QString &loc, const float lat, const float lon, const QString &coun, const QString &reg)
        : location{loc}
        , latitude{lat}
        , longitude{lon}
        , country{coun}
        , region{reg}
        {}

        bool isValid() const
        {
          return !location.isEmpty();
        }
      };

      /** \brief Returns the selected location information.
       * 
       */
      Location selected() const;

    protected:
      void keyPressEvent(QKeyEvent * event) override;

    private slots:
      /** \brief Launches the location search.
       * 
      */
      void onSearchButtonPressed();

      /** \brief Updates the UI when the user changes the location text. 
       * \param[in] text New location text. 
       * 
      */
      void onLocationTextModified(const QString &text);

      /** \brief Manages network manager replies. 
       * \param[in] reply Network reply.
       * 
      */
      void replyFinished(QNetworkReply *reply);

    private:
      /** \brief Helper method that connects signals to slots. 
       * 
      */
      void connectSignals();

      QNetworkAccessManager* m_netManager; /** network manager from the ConfigurationDialog. */
      const QString         &m_apiKey;     /** OpenWeatherMap valid api key.                 */
      const QString         &m_language;   /** Language code.                                */
};

#endif