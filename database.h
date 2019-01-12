/*
 * database.h
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

#ifndef DATABASE_H
#define DATABASE_H

#include <QMessageBox>
#include <QtSql>
#include <QtDebug>
#include <QtGlobal>
#include "configmanager.h"

class Database
{
public:
    Database();
    ~Database();

    bool openDatabase();
    bool closeDatabase();

    QString getDBFilePath();

    QSqlQuery executeQuery(const QString &table, const QStringList &addcols = QStringList(), const QStringList &addvals = QStringList(), const QString &selectcols = "*", const QStringList &connectrelation = QStringList());

    int deleteEntry(const QString &table, const QString &id);

    int updateEntry(const QString &table, const QStringList &updcols, const QStringList &updvals, const QString &id);

    int insertEntry(const QString &table, const QStringList &updcols, const QStringList &updvals);

    int countEntries(const QString &table, const QStringList &addcols = QStringList(), const QStringList &addvals = QStringList());


    static void exec(QSqlQuery *query, const QString &errorText);
private:
    QSqlDatabase SqliteDatabase;
    bool initDatabase();
    bool TimestampAdditionMigration();
    bool columnNotExistsForTable(const QString &table, const QString &column);
    QString getTimestamp();
};

#endif // DATABASE_H
