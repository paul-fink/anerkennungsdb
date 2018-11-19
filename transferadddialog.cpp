/*
 * transferadddialog.cpp
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

#include "transferadddialog.h"
#include "ui_transferadddialog.h"

/*!
 * \class TransferAddDialog
 *
 * \brief Dialog to enter new transfers into the db
 *
 * The dialog does some consistency checking prior to saving into the database.
 *
 * \since 1.0
 */

/*!
 * \fn TransferAddDialog::TransferAddDialog(Database *mydb, QWidget *parent = 0)
 *
 * \brief Constructs the TransferAddDialog
 *
 * The dialog takes \a mydb as Database pointer and \a parent as pointer to the parental QWidget.
 * It also sets up all the models for the course and module comboboxes by fetching the data from the database
 */
TransferAddDialog::TransferAddDialog(Database *mydb, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TransferAddDialog),
    db(mydb)
{
    ui->setupUi(this);
    coursemodel = new QSqlQueryModel(ui->kursNameComboBox);
    modulemodel = new QSqlQueryModel(ui->modulNameComboBox);
    initModels();
}

/*!
 * \brief Destroys the TransferAddDialog
 */
TransferAddDialog::~TransferAddDialog()
{
    delete ui;
}

/*!
 * \brief Helper function to display QSqlErrors
 * \internal
 *
 * Prints out \a text as mesage start, followed by the error text of \a error.
 * Nothing is done when the error is not valid.
 */
void TransferAddDialog::checkError(QSqlError error, const QString &text)
{
    if(error.isValid()) qCritical() << QString("%1: %2").arg(text).arg(error.text());
}

/*!
 * \brief Initialises the combobox models
 *
 * This function fetches the contents of the tables \tt Kurse and \tt Module into separate models.
 * Each of them serves then as the source for a proxy model, which is actually used in the combobox.
 *
 * Those two layers are needed as sorting according to the used locale is required
 * which cannot be consistenly done on the database level.
 *
 * Finally, the save button is disabled
 */
void TransferAddDialog::initModels()
{
    // Setting up the course combobox
    QSqlQuery selectcoursequery = db->executeQuery("Kurse");
    checkError(selectcoursequery.lastError(), tr("Error in 'selectcoursequery'"));
    coursemodel->clear();
    coursemodel->setQuery(selectcoursequery);
    while (coursemodel->canFetchMore()) coursemodel->fetchMore();
    checkError(coursemodel->lastError(), tr("Error setting query to 'coursemodel'"));

    QSortFilterProxyModel *coursesfpm = new QSortFilterProxyModel(this);
    coursesfpm->setSourceModel(coursemodel);
    coursesfpm->setSortLocaleAware(true);  
    coursesfpm->sort(1, Qt::AscendingOrder);

    ui->kursNameComboBox->blockSignals(true);
    ui->kursNameComboBox->setModel(coursesfpm);
    ui->kursNameComboBox->setModelColumn(1);
    ui->kursNameComboBox->setCurrentIndex(-1);
    ui->kursNameComboBox->blockSignals(false);

    // Setting up the module combobox
    QSqlQuery selectmodulequery = db->executeQuery("Module");
    checkError(selectmodulequery.lastError(), tr("Error in 'selectmodulequery'"));
    modulemodel->clear();
    modulemodel->setQuery(selectmodulequery);
    while (modulemodel->canFetchMore()) modulemodel->fetchMore();
    checkError(modulemodel->lastError(), tr("Error setting query to 'modulemodel'"));

    QSortFilterProxyModel *modulesfpm = new QSortFilterProxyModel(this);
    modulesfpm->setSourceModel(modulemodel);
    modulesfpm->setSortLocaleAware(true);
    modulesfpm->sort(1, Qt::AscendingOrder);

    ui->modulNameComboBox->blockSignals(true);
    ui->modulNameComboBox->setModel(modulesfpm);
    ui->modulNameComboBox->setModelColumn(1);
    ui->modulNameComboBox->setCurrentIndex(-1);
    ui->modulNameComboBox->blockSignals(false);

    enableOkButton();
}

/*!
 * \brief Updates the GUI components for the course display
 *
 * This function sets the ECTS, origin and name of the selected course into its according GUI elements.
 * It also saves the underlying ID in a private dialog variable.
 *
 * Finally, the status of the buttons and the status message to display is adjusted.
 */
void TransferAddDialog::on_kursNameComboBox_currentIndexChanged(int index)
{

    QSortFilterProxyModel *courseproxy = static_cast<QSortFilterProxyModel*>(ui->kursNameComboBox->model());

    QModelIndex qidx = courseproxy->mapToSource(ui->kursNameComboBox->model()->index(index,0));

    cid = courseproxy->sourceModel()->data(qidx).toString();
    ui->kursEctsLineEdit->setText(courseproxy->sourceModel()->data(courseproxy->sourceModel()->index(qidx.row(),3)).toString());
    ui->kursHerkunftLineEdit->setText(courseproxy->sourceModel()->data(courseproxy->sourceModel()->index(qidx.row(),2)).toString());
    ui->kursNameComboBox->setToolTip(ui->kursNameComboBox->currentText());

    enableOkButton();
}

/*!
 * \brief Updates the GUI components for the module display
 *
 * This function sets the ECTS, PO and name of the selected module into its according GUI elements.
 * It also saves the underlying ID in a private dialog variable.
 *
 * Finally, the status of the buttons and the status message to display is adjusted.
 */
void TransferAddDialog::on_modulNameComboBox_currentIndexChanged(int index)
{
    QSortFilterProxyModel *moduleproxy = static_cast<QSortFilterProxyModel*>(ui->modulNameComboBox->model());

    QModelIndex qidx = moduleproxy->mapToSource(ui->modulNameComboBox->model()->index(index,0));

    mid = moduleproxy->sourceModel()->data(qidx).toString();
    ui->modulEctsLineEdit->setText(moduleproxy->sourceModel()->data(moduleproxy->sourceModel()->index(qidx.row(),3)).toString());
    ui->modulPoLineEdit->setText(moduleproxy->sourceModel()->data(moduleproxy->sourceModel()->index(qidx.row(),2)).toString());
    ui->modulNameComboBox->setToolTip(ui->modulNameComboBox->currentText());

    enableOkButton();
}

/*!
 * \brief Updates the Buttons and the status message
 *
 * The save button is only enabled if
 * \list
 *   \li both a course and a module are selected and
 *   \li the resulting transfer does not exist in the database.
 * \endlist
 *
 * If the above case is not satisfied the status message is given as an error.
 * If the ECTS of course and module do not match the status message is set to a warning.
 */
void TransferAddDialog::enableOkButton()
{
    if(mid.isEmpty() || mid.isEmpty()) {
        ui->statusIconLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(32));
        ui->statusLabel->setText(tr("Please select a course and module first!"));
        ui->okButton->setEnabled(false);
        return;
    }
    QStringList cols, vals;
    cols << "KID" << "MID";
    vals << cid << mid;

    if(db->countEntries("Anerkennungen",cols,vals) > 0) {
        ui->statusIconLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(32));
        ui->statusLabel->setText(tr("This transfer already exists!"));
        ui->okButton->setEnabled(false);
        return;
    }
    if(ui->kursEctsLineEdit->text().toInt() != ui->modulEctsLineEdit->text().toInt()) {
        ui->statusIconLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(32));
        ui->statusLabel->setText(tr("This transfer does not exist yet. Beware the ECTS-credits differ!"));
    } else {
        ui->statusIconLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(32));
        ui->statusLabel->setText(tr("This transfer does not exist yet."));
    }
    ui->okButton->setEnabled(true);
}
