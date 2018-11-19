/*
 * modifydialog.cpp
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

#include "modifydialog.h"
#include "ui_modifydialog.h"

/*!
 * \class ModifyDialog
 *
 * \brief Dialog to add/edit courses and modules
 *
 * This dialog allows to edit or add courses and modules.
 *
 * \since 1.0
 */

/*!
 * \fn ModifyDialog::ModifyDialog(const QString &view, QWidget *parent = 0, bool add = false)
 *
 * \brief Constructs the ModifyDialog
 *
 * The dialog takes a \parent as pointer to its parental QWidget.
 * The functionality of the dialog depends on \a add:
 * If \a add is \c true, then the dialog is set up in a way to allow adding a new entry,
 * otherwise, all the GUI elements are preset by values from the database.
 * Finally, depending on \a view a label is changed to match the internal database structure,
 * as well as the dialog title is adapted.
 */
ModifyDialog::ModifyDialog(const QString &view, QWidget *parent, bool add) :
    QDialog(parent),
    ui(new Ui::ModifyDialog),
    view(view),
    isAdd(add)
{
    ui->setupUi(this);
    initGuiElements();
}

/*!
 * \brief Destroys the ModifyDialog
 */
ModifyDialog::~ModifyDialog()
{
    delete ui;
}

/*!
 * \brief Initialises the GUI Elements
 *
 * Depending on the view and isAdd member the title is set, as well as the label of a textedit.
 */
void ModifyDialog::initGuiElements()
{
    QString action = (isAdd ? tr("Add") : tr("Edit"));
    QString type;
    if(view == "Kurse") {
        type = tr("course entry");
        ui->otherLabel->setText(tr("Origin"));

    } else if (view == "Module") {
        type = tr("module entry");
        ui->otherLabel->setText(tr("PO"));
    }
    setWindowTitle(tr("%1 %2").arg(action).arg(type));
}

/*!
 * \brief Presets the value
 *
 * Depending on the values in \a query the GUI elements are prefilled.
 * Only the first row of \a query is used.
 */
void ModifyDialog::presetValues(QSqlQuery query)
{
    if(query.isActive()) {
        int idxname = -1;
        int idxects = query.record().indexOf("ECTS");
        int idxother = -1;
        if(view == "Module") {
            idxname = query.record().indexOf("Modulname");
            idxother = query.record().indexOf("PO");
        } else if (view == "Kurse") {
            idxname = query.record().indexOf("Kursname");
            idxother = query.record().indexOf("Herkunft");
        }
        query.next();
        ui->nameLineEdit->setText(query.value(idxname).toString());
        ui->ectsSpinBox->setValue(query.value(idxects).toInt());
        ui->otherLineEdit->setText(query.value(idxother).toString());
    }
}

/*!
 * \brief Returns the name of the course/module
 */
QString ModifyDialog::getNameValue()
{
    return ui->nameLineEdit->text();
}

/*!
 * \brief Returns the ects of the course/module
 */
int ModifyDialog::getEctsValue()
{
    return ui->ectsSpinBox->value();
}

/*!
 * \brief Returns the other value of the course/module
 *
 * For a course edit/add this is the origin and for a module edit/add the PO.
 */
QString ModifyDialog::getOtherValue()
{
    return ui->otherLineEdit->text();
}
