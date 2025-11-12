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
#include <dialogs/LocationFinderDialog.h>
#include <Provider.h>
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

//----------------------------------------------------------------------------
LocationFinderDialog::LocationFinderDialog(std::shared_ptr<WeatherProvider> provider, std::shared_ptr<QNetworkAccessManager> manager,
                                           QWidget *parent, Qt::WindowFlags f)
: QDialog(parent, f)
, m_provider{provider}
, m_netManager{manager}
{
  assert(manager);
  assert(provider);

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
LocationFinderDialog::~LocationFinderDialog()
{
  disconnect(m_provider.get(), SIGNAL(foundLocations(const Locations &)), this, SLOT(locations(const Locations &)));
}

//----------------------------------------------------------------------------
Location LocationFinderDialog::selected() const
{
  const auto selectedRow = m_locationsTable->currentIndex();
  
  if(selectedRow.isValid())
  {
    const auto location = m_locationsTable->item(selectedRow.row(), 0)->text();
    const auto translated = m_locationsTable->item(selectedRow.row(), 1)->text();
    const auto latitude = m_locationsTable->item(selectedRow.row(), 2)->text().toFloat();
    const auto longitude = m_locationsTable->item(selectedRow.row(), 3)->text().toFloat();
    const auto country = m_locationsTable->item(selectedRow.row(), 4)->text();
    const auto region = m_locationsTable->item(selectedRow.row(), 5)->text();
    return Location{location, translated, latitude, longitude, country, region};
  }
  
  return Location(); // Invalid
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
void LocationFinderDialog::locations(const Locations &locations)
{
  QApplication::restoreOverrideCursor();
  m_searchButton->setEnabled(true);
  m_locationsTable->clearContents();

  if(locations.isEmpty())
  {
    const QString message = tr("No locations found for '%1'.").arg(m_location->text());

    QMessageBox msgbox(this);
    msgbox.setWindowTitle(tr("Location finder"));
    msgbox.setWindowIcon(QIcon(":/TrayWeather/application.svg"));
    msgbox.setText(message);
    msgbox.setBaseSize(400, 400);
    msgbox.exec();

    m_searchButton->setEnabled(true);
    return;
  }

  m_locationsTable->clearContents();

  for (const auto &location: locations)
  {
    const auto row = m_locationsTable->rowCount();
    m_locationsTable->insertRow(row);

    const auto nameItem = new QTableWidgetItem(location.location);
    nameItem->setTextAlignment(Qt::AlignCenter);
    m_locationsTable->setItem(row, 0, nameItem);

    const auto localItem = new QTableWidgetItem(location.translated);
    if (location.translated.isEmpty())
    {
      localItem->setText(tr("No translation"));
      localItem->setTextColor(Qt::gray);
    }
    localItem->setTextAlignment(Qt::AlignCenter);
    m_locationsTable->setItem(row, 1, localItem);

    const auto latItem = new QTableWidgetItem(QString("%1").arg(location.latitude));
    latItem->setTextAlignment(Qt::AlignCenter);
    m_locationsTable->setItem(row, 2, latItem);

    const auto longItem = new QTableWidgetItem(QString("%1").arg(location.longitude));
    longItem->setTextAlignment(Qt::AlignCenter);
    m_locationsTable->setItem(row, 3, longItem);

    const auto countryItem = new QTableWidgetItem(location.country);
    countryItem->setTextAlignment(Qt::AlignCenter);
    if (!location.country.isEmpty())
    {
      const auto country = location.country;
      const auto countryCode = std::find_if(ISO3166.cbegin(), ISO3166.cend(), [&country](const QString &code)
                                            { return code.compare(country, Qt::CaseInsensitive) == 0; });
      if (countryCode != ISO3166.cend())
        countryItem->setText(ISO3166.key(*countryCode));
    }
    m_locationsTable->setItem(row, 4, countryItem);

    const auto stateItem = new QTableWidgetItem(location.region);
    stateItem->setTextAlignment(Qt::AlignCenter);
    m_locationsTable->setItem(row, 5, stateItem);
  }

  const bool hasResult = m_locationsTable->rowCount() > 0;
  m_locationsTable->setEnabled(hasResult);

  if (hasResult)
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
  connect(m_provider.get(), SIGNAL(foundLocations(const Locations &)), this, SLOT(locations(const Locations &)));
}

//----------------------------------------------------------------------------
void LocationFinderDialog::onSearchButtonPressed()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  m_locationsTable->clearContents();
  m_locationsTable->setRowCount(0);
  m_provider->searchLocations(m_location->text(), m_netManager);
}