/*
 * csvwriter.cpp
 *
 * This file is part of AnerkennungsDB.
 *
 * Copyright (C) 2018 Paul Fink <paul.fink@mailbox.org>
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

#include "csvwriter.h"

/*!
 * \class CSVWriter
 *
 * \brief Converting a database table into a csv file
 * \since 3.0
 */

/*!
 * \fn CSVWriter::CSVWriter(Database *database, QWidget *parent = 0)
 *
 * \brief Constructs a CSVWriter
 *
 * It initialises the internal database pointer \a database and also a pointer to a parental QWidget \a parent.
 */
CSVWriter::CSVWriter(Database *database, QWidget *parent) : db(database), parentWidget(parent)
{}

/*!
 * \fn CSVWriter::writeCSV(QFile &file, const QString &tablename, const QString &selectcols = "*")
 *
 * \brief Write a database table to a csv file
 *
 * The function write a database table with name \a tablename to the file \a file.
 * If \a selectcols is not specified, then all columns are written, otherwise only the ones in \a selectocls.
 * They need to be given in a manner that is understood by a SQL select command.
 *
 * The text in each cell is wrapped in double quotes and columns are separated by a semi-colon.
 * This corresponds to the usual German csv setting.
 */
void CSVWriter::writeCSV(QFile &file, const QString &tablename, const QString &selectcols) {

    if (!file.open(QIODevice::WriteOnly)) {

        QMessageBox::warning(parentWidget, QObject::tr("Target file is not writable."),file.errorString());
        return;

    } else {

        QSqlQuery selectquery = db->executeQuery(tablename, QStringList(), QStringList(), selectcols);
        if(selectquery.lastError().isValid()) {
            qCritical() << QObject::tr("Error in query 'selectquery':") << selectquery.lastError();
        }

        QTextStream filestream(&file);

        bool first = true;
        QStringList textrow;
        QString wrapquotes = "\"%1\"";
        int fieldcount = -1;
        while(selectquery.next()) {
            if(first) {
                first = false;
                fieldcount = selectquery.record().count();
                for(int i = 0; i < fieldcount; i++) {
                    textrow << wrapquotes.arg(selectquery.record().fieldName(i));
                }
                filestream << textrow.join(";") + "\n";
            }
            textrow.clear();

            for(int i = 0; i < fieldcount; i++) {
                textrow << wrapquotes.arg(selectquery.value(i).toString());
            }
            filestream << textrow.join(";") + "\n";
        }
        file.close();

    }

}

