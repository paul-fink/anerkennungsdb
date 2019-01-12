/*
 * aboutdialog.cpp
 *
 * This file is part of AnerkennungsDB.
 *
 * Copyright (C) 2016-2019 Paul Fink <paul.fink@mailbox.org>
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

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

/*!
 * \class AboutDialog
 * \brief Creates the about dialog
 *
 * Constructor for the about dialog.
 * It displays information about the application including a short notice on the used components and their license.
 *
 * \since 2.0
 */

/*!
 * \fn AboutDialog::AboutDialog(QWidget *parent = 0)
 *
 * Constructs the about dialog with \a parent as the pointer to the parental QWidget object
 */
AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->textBrowser->setOpenExternalLinks(true);
    ui->textBrowser->setHtml(QString("<b>%1 (%2)</b><br>").arg(ANERKENNUNGSDB).arg(ANERKENNUNGSDB_VERSION) +
                       tr("Using Qt version %1, compiled with Qt %2").arg(qVersion()).arg(QT_VERSION_STR) + "<br><br>"
                       "Copyright (C)<br>" +
                       "AnerkennungsDB: Paul Fink 2016-2019<br>" +
                       tr("AnerkennungsDB uses") + " SQLite (Public Domain). <br>" +
                       tr("AnerkennungsDB uses") + " QuaZip (LGPL v2.1 + Static Linking Exception, Copyright (C) 2005-2018 Sergey A. Tachenov and contributors). <br><br>" +
                       tr("Contact:") + " <a href=\"mailto:paul.fink@mailbox.org\">paul.fink@mailbox.org</a><br><br>" +
                       tr("This program is licensed to you under the terms of the GNU General Public License Version 3 as published by the Free Software Foundation."));

}

/*!
 * \fn AboutDialog::~AboutDialog()
 *
 * Destroys the about dialog
 */
AboutDialog::~AboutDialog()
{
    delete ui;
}
