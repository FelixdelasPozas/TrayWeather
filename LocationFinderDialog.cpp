/*
 File: LocationFinderDialog.cpp
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

// Project
#include <LocationFinderDialog.h>
#include <ISO 3166-1-alpha-2.h>

// C++
#include <cassert>
#include <iostream>

// Qt
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QObject>
#include <QString>
#include <QKeyEvent>
#include <QPalette>
#include <QTableWidgetItem>

const QString REQUEST = "http://api.openweathermap.org/geo/1.0/direct?q=%1&limit=5&appid=%2";

// OpenWeatherMap JSON keys.
const QString NAME_KEY = "name";
const QString LOCAL_NAMES_KEY = "local_names";
const QString LATITUDE_KEY = "lat";
const QString LONGITUDE_KEY = "lon";
const QString COUNTRY_KEY = "country";
const QString STATE_KEY = "state";

//----------------------------------------------------------------------------
LocationFinderDialog::LocationFinderDialog(const QString &apiKey, const QString &languageCode, QNetworkAccessManager* manager, QWidget *parent, Qt::WindowFlags f)
: QDialog(parent, f)
, m_netManager{manager}
, m_apiKey{apiKey}
, m_language{languageCode}
{
  assert(manager);

  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

  setupUi(this);

  connectSignals();

  m_searchButton->setEnabled(false);

  m_locationsTable->setColumnCount(6);
  const QStringList HEADER_LABELS = {QObject::tr("Location"),
                                     QObject::tr("Local name"),
                                     QObject::tr("Latitude"),
                                     QObject::tr("Longitude"),
                                     QObject::tr("Country"),
                                     QObject::tr("State/Province/Region")};

  m_locationsTable->setHorizontalHeaderLabels(HEADER_LABELS);
  m_locationsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
  m_locationsTable->setEnabled(false);

  // Maintain the selected item colors when the table is out of focus to 
  // always highlight the one than will be selected when the users clicks the Ok button.
  auto palette = m_locationsTable->palette();
  palette.setBrush(QPalette::Inactive, QPalette::Highlight, palette.highlight());
  palette.setBrush(QPalette::Inactive, QPalette::HighlightedText, palette.highlightedText());
  m_locationsTable->setPalette(palette);

  setMinimumWidth(600);

  m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

  m_location->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  m_location->setFocus();
}

//----------------------------------------------------------------------------
LocationFinderDialog::Location LocationFinderDialog::selected() const
{
  const auto selectedRow = m_locationsTable->currentIndex();
  
  if(selectedRow.isValid())
  {
    const auto location = m_locationsTable->item(selectedRow.row(), 0)->text();
    const auto latitude = m_locationsTable->item(selectedRow.row(), 2)->text().toFloat();
    const auto longitude = m_locationsTable->item(selectedRow.row(), 3)->text().toFloat();
    const auto country = m_locationsTable->item(selectedRow.row(), 4)->text();
    const auto region = m_locationsTable->item(selectedRow.row(), 5)->text();
    return Location{location, latitude, longitude, country, region};
  }
  
  return Location{"",0.f,0.f, "", ""}; // Invalid
}

//----------------------------------------------------------------------------
void LocationFinderDialog::keyPressEvent(QKeyEvent *event)
{
  // Don't want the enter key on the QLineEdit to close the dialog. 
  if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
  {
    event->ignore();
    return;
  }

  QDialog::keyPressEvent(event);
}

//----------------------------------------------------------------------------
void LocationFinderDialog::onLocationTextModified(const QString &text)
{
  m_searchButton->setEnabled(!text.isEmpty());
}

//----------------------------------------------------------------------------
void LocationFinderDialog::replyFinished(QNetworkReply *reply)
{
  QApplication::restoreOverrideCursor();
  QString details, message;

  m_searchButton->setEnabled(true);

  if(reply->error() != QNetworkReply::NetworkError::NoError)
  {
    message = tr("Invalid reply from server.\nCouldn't get location information.\nIf you have a firewall change the configuration to allow this program to access the network.");
    details = reply->errorString();

    QMessageBox msgbox(this);
    msgbox.setWindowTitle(tr("Network Error"));
    msgbox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    msgbox.setDetailedText(details);
    msgbox.setText(message);
    msgbox.setBaseSize(400, 400);
    msgbox.exec();

    reply->deleteLater();
    return;
  }

  const auto url = reply->url();
  if(url.toString().contains("api.openweathermap.org", Qt::CaseInsensitive))
  {
    const auto data = reply->readAll();
    const auto jsonDocument = QJsonDocument::fromJson(data);

    if (!jsonDocument.isNull() && jsonDocument.isArray())
    {
      const auto locationsArray = jsonDocument.array();

      if(locationsArray.isEmpty())
      {
        const QString message = tr("No locations found for '%1'.").arg(m_location->text());

        QMessageBox msgbox(this);
        msgbox.setWindowTitle(tr("Location finder"));
        msgbox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
        msgbox.setText(message);
        msgbox.setBaseSize(400, 400);
        msgbox.exec();

        reply->deleteLater();
        m_searchButton->setEnabled(true);

        return;
      }

      m_locationsTable->clearContents();

      for(auto location: locationsArray)
      {
        const auto locObject = location.toObject();

        const auto row = m_locationsTable->rowCount();
        m_locationsTable->insertRow(row);

        const auto nameItem = new QTableWidgetItem(locObject.value(NAME_KEY).toString());
        nameItem->setTextAlignment(Qt::AlignCenter);
        m_locationsTable->setItem(row, 0, nameItem);

        const auto locListObj = locObject.value(LOCAL_NAMES_KEY).toObject();
        auto localName = locListObj.value(m_language).toString();
        const auto localItem = new QTableWidgetItem(localName);
        if(localName.isEmpty())
        {
          localItem->setText(tr("No translation"));
          localItem->setTextColor(Qt::gray);
        }
        localItem->setTextAlignment(Qt::AlignCenter);
        m_locationsTable->setItem(row, 1, localItem);

        const auto latItem = new QTableWidgetItem(QString("%1").arg(locObject.value(LATITUDE_KEY).toDouble()));
        latItem->setTextAlignment(Qt::AlignCenter);
        m_locationsTable->setItem(row, 2, latItem);

        const auto longItem = new QTableWidgetItem(QString("%1").arg(locObject.value(LONGITUDE_KEY).toDouble()));
        longItem->setTextAlignment(Qt::AlignCenter);
        m_locationsTable->setItem(row, 3, longItem);

        auto country = locObject.value(COUNTRY_KEY).toString();
        const auto countryItem = new QTableWidgetItem(country);
        countryItem->setTextAlignment(Qt::AlignCenter);
        if(!country.isEmpty())
        {
          const auto countryCode = std::find_if(ISO3166.cbegin(), ISO3166.cend(), [&country](const QString &code) { return code.compare(country, Qt::CaseInsensitive) == 0; });
          if(countryCode != ISO3166.cend())
            countryItem->setText(ISO3166.key(*countryCode));
        }
        m_locationsTable->setItem(row, 4, countryItem);

        const auto stateItem = new QTableWidgetItem(locObject.value(STATE_KEY).toString());
        stateItem->setTextAlignment(Qt::AlignCenter);
        m_locationsTable->setItem(row, 5, stateItem);
      }
    }
  }

  const bool hasResult = m_locationsTable->rowCount() > 0;
  reply->deleteLater();
  m_locationsTable->setEnabled(hasResult);

  if(hasResult)
  {
    m_locationsTable->resizeColumnsToContents();
    m_locationsTable->adjustSize();
    m_locationsTable->selectRow(0);
  }

  auto button = m_buttonBox->button(QDialogButtonBox::Ok);
  button->setEnabled(hasResult);

  m_searchButton->setEnabled(true);
  m_location->setFocus(Qt::FocusReason::TabFocusReason);
  m_location->grabKeyboard();
}

//----------------------------------------------------------------------------
void LocationFinderDialog::connectSignals()
{
  connect(m_searchButton, SIGNAL(clicked()), this, SLOT(onSearchButtonPressed()));
  connect(m_location, SIGNAL(textChanged(const QString &)), this, SLOT(onLocationTextModified(const QString &)));
  connect(m_location, SIGNAL(returnPressed()), m_searchButton, SLOT(click()));
  connect(m_netManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
}

//----------------------------------------------------------------------------
void LocationFinderDialog::onSearchButtonPressed()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  const auto location = m_location->text();
  m_locationsTable->clearContents();
  m_locationsTable->setRowCount(0);

  auto url = QUrl{REQUEST.arg(m_location->text()).arg(m_apiKey)};
  m_netManager->get(QNetworkRequest{url});
}