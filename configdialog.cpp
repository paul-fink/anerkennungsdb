/*
 * configdialog.cpp
 *
 * This file is part of AnerkennungsDB.
 *
 * Copyright (C) 2016-2018 Paul Fink <paul.fink@mailbox.org>
 *
 * AnerkennungsDB is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * AnerkennungsDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include "configdialog.h"
#include "ui_configdialog.h"

/*!
 * \class ConfigDialog
 *
 * \brief Dialog for the configurable part of the application
 *
 * In the configuration dialog currently the GUI language and the location of the database can be configured
 *
 * \since 2.0
 */

/*!
 * \fn ConfigDialog::ConfigDialog(QWidget *parent = 0)
 *
 * Constructs the about dialog with \a parent as the pointer to the parental QWidget object. It also initialises the GUI elements
 */

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);
    initGuiElements();
}

/*!
 * \fn ConfigDialog::~ConfigDialog()
 *
 * Destroys the config dialog
 */
ConfigDialog::~ConfigDialog()
{
    delete ui;
}

/*!
 * \fn ConfigDialog::initGuiElements()
 *
 * Adding the translatable contents of the comboboxes for the database location and the language setting
 *
 * /sa ConfigManager::getDatabaseLocation()
 */
void ConfigDialog::initGuiElements()
{
    // Database location
    ui->databaseComboBox->blockSignals(true);
    ui->databaseComboBox->addItem(tr("Executable directory"));
    ui->databaseComboBox->addItem(tr("Generic application configuration directory"));
    ui->databaseComboBox->addItem(tr("Custom directory"));
    ui->databaseComboBox->setCurrentIndex(-1);
    ui->databaseComboBox->blockSignals(false);

    //Language
    ui->languageComboBox->blockSignals(true);
    ui->languageComboBox->addItem(tr("German"), "de");
    ui->languageComboBox->addItem(tr("English"), "en");
    ui->languageComboBox->setCurrentIndex(-1);
    ui->languageComboBox->blockSignals(false);

    //Font size
    ui->fontsizeSpinBox->blockSignals(true);
    ui->fontsizeSpinBox->setValue(QApplication::font().pointSize());
    ui->fontsizeSpinBox->blockSignals(false);

}

/*!
 * \fn ConfigDialog::on_databaseButton_clicked()
 *
 * Triggering the FileChooser dialog to select a directory for the database
 */
void ConfigDialog::on_databaseButton_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(this,
                                tr("Database location"),
                                ui->databaseLineEdit->text(),
                                QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly);
    if (!directory.isEmpty()) {
        ui->databaseLineEdit->setText(directory);
    }
}


/*!
 * \fn ConfigDialog::on_databaseComboBox_currentIndexChanged(int index)
 *
 * Slot is evaluated when the database combobox is changed.
 * It enables the text field if the \a index equals 2, corresponding to a custom location
 */
void ConfigDialog::on_databaseComboBox_currentIndexChanged(int index)
{
    setDatabaseEditEnabled(index == 2);
}

/*!
 * \fn ConfigDialog::setDatabaseEditEnabled(bool enable)
 *
 * If \a enable is \c true then the button and textfield for choosing a custom database location are enabled,
 * otherwise it disables them and also clears the content fo the textfield in advance.
 */
void ConfigDialog::setDatabaseEditEnabled(bool enable)
{
    if(!enable) {
        ui->databaseLineEdit->clear();
    }
    ui->databaseLineEdit->setEnabled(enable);
    ui->databaseButton->setEnabled(enable);
}

/*!
 * \brief This function sets the database location
 *
 * It first sets the combobox to the index of \a locId.
 * If \a locId equals to 2 (the custom location) it also sets the \a location into the textfield.
 */
void ConfigDialog::setDatabaseLocation(int locId, const QString &location)
{
    ui->databaseComboBox->setCurrentIndex(locId);
    if(locId == 2) ui->databaseLineEdit->setText(location);
}

/*!
 * \brief Returns the database location
 *
 * \return A single QString consisting of
 * \list
 *   \li the current index of the location combobox and
 *   \li the current text in the location textfield.
 * \endlist
 * They are separated by three colons.
 */
QString ConfigDialog::databaseLocation() const
{
    return QString("%1:::%2").arg(ui->databaseComboBox->currentIndex()).arg(ui->databaseLineEdit->text());
}


/*!
 * \brief This function sets the language of the GUI
 *
 * The QString \a lang is set into the language combobox
 */
void ConfigDialog::setLanguage(const QString &lang)
{
    ui->languageComboBox->setCurrentText(lang);
}

/*!
 * \brief  Returns the language of the GUI in a QString
 *
 * \return A single QString of the text displayed currently in the language combobox.
 */
QString ConfigDialog::language() const
{
    return ui->languageComboBox->currentData().toString();
}


/*!
 * \brief This function sets the font size of the GUI
 *
 * The int \a size is set into the font size spinbox
 */
void ConfigDialog::setFontSize(const int size)
{
    ui->fontsizeSpinBox->setValue(size);
}

/*!
 * \brief  Returns the font size of the GUI in an int
 *
 * \return A single int of the value displayed currently in the font size spinbox
 */
int ConfigDialog::fontSize() const
{
    return ui->fontsizeSpinBox->value();
}
