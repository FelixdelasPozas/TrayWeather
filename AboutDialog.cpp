/*
 File: AboutDialog.cpp
 Created on: 15/11/2016
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
#include "AboutDialog.h"
#include "Utils.h"

// Qt
#include <QtGlobal>
#include <QDateTime>
#include <QApplication>

const QString AboutDialog::VERSION{"1.20.1"};
const QString COPYRIGHT{"Copyright (c) 2016-%1 Félix de las Pozas Álvarez"};

//-----------------------------------------------------------------
AboutDialog::AboutDialog(QWidget *parent, Qt::WindowFlags flags)
: QDialog(parent, flags)
{
  setupUi(this);

  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & ~(Qt::WindowMaximizeButtonHint) & ~(Qt::WindowMinimizeButtonHint));

  auto compilation_date = QString(__DATE__);
  auto compilation_time = QString(" (") + QString(__TIME__) + QString(")");
  const auto verStr = tr("version");

  m_compilationDate->setText(tr("Compiled on ") + compilation_date + compilation_time);
  m_version->setText(QString("%1 %2").arg(verStr).arg(VERSION));
  m_qtVersion->setText(QString("%1 %2").arg(verStr).arg(qVersion()));
  m_chartsVersion->setText(QString("%1 %2").arg(verStr).arg("2.1.0"));
  m_copyright->setText(COPYRIGHT.arg(QDateTime::currentDateTime().date().year()));

  fillTranslationsTable();
  fillThemesTable();

  tabWidget->setCurrentIndex(0);
}

//-----------------------------------------------------------------
void AboutDialog::showEvent(QShowEvent *e)
{
  QDialog::showEvent(e);

  scaleDialog(this);
}

//-----------------------------------------------------------------
void AboutDialog::changeEvent(QEvent *e)
{
  if(e && e->type() == QEvent::LanguageChange)
  {
    retranslateUi(this);
  }

  QDialog::changeEvent(e);
}

//-----------------------------------------------------------------
void AboutDialog::fillTranslationsTable() const
{
  m_translations->clear();
  m_translations->verticalHeader()->setVisible(false);
  m_translations->horizontalHeader()->setVisible(false);
  m_translations->setRowCount(TRANSLATIONS.size());
  m_translations->setColumnCount(2);

  for(int i = 0; i < TRANSLATIONS.size(); ++i)
  {
    const auto &lang = TRANSLATIONS.at(i);

    auto item = new QTableWidgetItem();
    item->setIcon(QIcon(lang.icon));
    item->setData(Qt::DisplayRole, lang.name);
    m_translations->setItem(i, 0, item);

    item = new QTableWidgetItem();
    item->setData(Qt::DisplayRole, lang.author);
    m_translations->setItem(i, 1, item);
  }

  m_translations->resizeColumnToContents(0);
}

//-----------------------------------------------------------------
void AboutDialog::fillThemesTable() const
{
  m_themes->clear();
  m_themes->verticalHeader()->setVisible(false);
  m_themes->horizontalHeader()->setVisible(false);
  m_themes->setRowCount(ICON_THEMES.size());
  m_themes->setColumnCount(2);

  for(int i = 0; i < ICON_THEMES.size(); ++i)
  {
    const auto &theme = ICON_THEMES.at(i);

    auto item = new QTableWidgetItem();
    item->setData(Qt::DisplayRole, theme.name);
    m_themes->setItem(i, 0, item);

    auto label = new QLabel{QString("<a href=\"%1\">%1</a>").arg(theme.author)};
    label->setOpenExternalLinks(true);
    m_themes->setCellWidget(i, 1, label);
  }

  m_themes->resizeColumnToContents(0);
  m_themes->horizontalHeader()->setStretchLastSection(true);
}
