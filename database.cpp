/*
 * database.cpp
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

#include "database.h"

/*!
 * \class Database
 *
 * \brief This class provides the interface to the SQLite database.
 *
 * All interactions with the database are managed by this class.
 * It also makes sure that the database in opened and closed appropriately and also initialised on first use.
 *
 * \since 1.0
 */

/*!
 * \brief Constructs the Database object
 *
 * It registers the database
 */
Database::Database()
{
    SqliteDatabase = QSqlDatabase::addDatabase("QSQLITE");
}

/*!
 * \brief Destroys the Database object
 *
 * It also makes sure that the database is closed.
 */
Database::~Database()
{
    closeDatabase();
}

/*!
 * \brief Returns the file path of the database in a QString
 *
 * The directory path is querried from the ConfigManager and then the database name is appended.
 * The database is always named \tt anerkennungen.sqlite
 */
QString Database::getDBFilePath() {
    return ConfigManager::getInstance()->getDatabaseLocation() + QDir::separator() + "anerkennungen.sqlite";
}

/*!
 * \brief Closes the database
 *
 * Returns \c true if the closing of the database was successful, otherwise \c false
 */
bool Database::closeDatabase()
{
    SqliteDatabase.close();
    qDebug() << QObject::tr("Connection to database closed");
    return true;
}

/*!
 * \brief Opens the database
 *
 * Returns \c true if the database could be opened, otherwise \c false and a dialog is presented with the given error.
 * Furthermore, the database is initiliased.
 *
 * \sa initDatabase()
 */
bool Database::openDatabase()
{
    qsrand(static_cast<uint>(QTime::currentTime().msec()));
    SqliteDatabase.setDatabaseName(getDBFilePath());

    if (!SqliteDatabase.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Connection to database failed"),
                             QObject::tr("An error occured on opening the database connection: %1").arg(SqliteDatabase.lastError().text()));
        return false;
    }
    qDebug() << QObject::tr("Connection to database successful");
    initDatabase();
    TimestampAdditionMigration();
    return true;
}

/*!
 * \brief Executes a SQLQuery object
 *
 * This function executes the given \a query object.
 *
 * The main purpose of this function is uniformly check and report errors, which might be present in the \a query object itself
 * (e.g.,a failed bindValue() call in case of a corrupt table scheme) or which might be present after the execution.
 * Furthermore by the QString \a method the respective calling function is stored and added to the error log.
 * \param query
 * \param method
 */
void Database::exec(QSqlQuery *query, const QString &method)
{

    if(query->lastError().isValid()) {
        qCritical() << QObject::tr("Database error in '%1': %2").arg(method).arg(query->lastError().text());
    }
    if(!query->exec()) {
        qCritical() << QObject::tr("Database error in '%1': %2").arg(method).arg(query->lastError().text());
    }
}

/*!
 * \brief Initialises the Database
 *
 * It returns \c true if init successful and otherwise \c false.
 *
 * This function makes sure that the foreign_keys pragma is set.
 * Furthermore, it creates all necessary tables and views if they are not present within the database
 */
bool Database::initDatabase() {

    QSqlQuery query;
    query.exec("PRAGMA foreign_keys = ON");

    // Init tables
    QStringList tableList = SqliteDatabase.tables(QSql::Tables);
    if(!tableList.contains("Module")) {
        query.prepare("CREATE TABLE Module ("
                        "ID INTEGER NOT NULL PRIMARY KEY,"
                        "Modulname TEXT NOT NULL,"
                        "PO TEXT,"
                        "ECTS INTEGER NOT NULL,"
                        "Datum TEXT NOT NULL"
                      ")");
        exec(&query, "CREATE TABLE Module");
    }
    if(!tableList.contains("Kurse")) {
        query.prepare("CREATE TABLE Kurse ("
                        "ID INTEGER NOT NULL PRIMARY KEY,"
                        "Kursname TEXT NOT NULL,"
                        "Herkunft TEXT,"
                        "ECTS INTEGER NOT NULL,"
                        "Datum TEXT NOT NULL"
                      ")");
        exec(&query, "CREATE TABLE Kurse");
    }
    if(!tableList.contains("Anerkennungen")) {
        query.prepare("CREATE TABLE Anerkennungen ("
                        "ID INTEGER NOT NULL PRIMARY KEY,"
                        "MID INTEGER, KID INTEGER,"
                        "Datum TEXT NOT NULL,"
                        "FOREIGN KEY(MID) REFERENCES Module(ID),"
                        "FOREIGN KEY(KID) REFERENCES Kurse(ID)"
                      ")");
        exec(&query, "CREATE TABLE Anerkennungen");
    }
    if(!tableList.contains("DBMigration")) {
        query.prepare("CREATE TABLE DBMigration("
                        "ID INTEGER NOT NULL PRIMARY KEY,"
                        "Name TEXT NOT NULL,"
                        "Datum TEXT NOT NULL"
                      ")");
        exec(&query, "CREATE TABLE DBMigration");
    }

    // Init views
    QStringList viewList = SqliteDatabase.tables(QSql::Views);
    if(!viewList.contains("anerkmodule")) {
        query.prepare("CREATE VIEW anerkmodule AS "
                      "SELECT A.MID AS ID, K.Kursname AS Kursname, K.ECTS AS ECTS, K.Herkunft AS Herkunft, K.Datum AS Datum "
                      "FROM Anerkennungen A JOIN Kurse K ON K.ID = A.KID");
        exec(&query, "CREATE VIEW anerkmodule");
    }
    if(!viewList.contains("anerkkurse")) {
        query.prepare("CREATE VIEW anerkkurse AS "
                      "SELECT A.KID AS ID, M.Modulname AS Modulname, M.ECTS AS ECTS, M.PO AS PO, M.Datum AS Datum "
                      "FROM Module M JOIN Anerkennungen A ON A.MID=M.ID");
        exec(&query,"CREATE VIEW anerkkurse");
    }

    return true;
}

/*!
 * \fn executeQuery(const QString &table, const QStringList &addcols = QStringList(), const QStringList &addvals = QStringList(), const QString &selectcols = "*", const QStringList &connectrelation = QStringList())
 *
 * \brief Returns an executed SELECT query QSqlQuery object
 *
 * This function performs a select query on the table of name \a table.
 * If \a selectcols is not specified, then all columns are included, otherwise only the ones in \a selectocls.
 * They need to be given in a manner that is understood by a SQL select command.
 *
 * If \a addcols and \a addvals are specified they need to be of the same length (otherwise the query is not executed).
 * They are used in the WHERE statement where \a addcols give the column names and \a addvals are the respective values.
 *
 * In the QStringList \a connectrelation it is stored how the entries in \a addcols are to be connected.
 * If the argument is missing they are connected by \c AND.
 *
 * The constructed and executed \c QSqlQuery object is returned.
 */
QSqlQuery Database::executeQuery(const QString &table, const QStringList &addcols, const QStringList &addvals, const QString &selectcols, const QStringList &connectrelation)
{
    QSqlQuery query;
    if(addcols.size() != addvals.size()) {
        query.finish();
        return query;
    }
    QString rel;
    bool isgrouped = false;
    QString sql = QString("SELECT ");
    sql += selectcols;
    sql += " FROM ";
    sql += table;
    if(!addcols.isEmpty()) {
        sql += " WHERE (";
        for(int i = 0; i < addcols.size(); i++) {
            if(i>0) {
                rel = connectrelation.value(i-1, " AND ");
                if(rel.contains('(')) {
                    isgrouped = true;
                }
                sql += rel;
            }
            sql += addcols.at(i);
            sql += "?";
            if(addcols.at(i).contains(" LIKE ")) {
                sql += " ESCAPE '!'";
            }
        }
        sql += ")";
        if(isgrouped) {
            sql += ")";
        }
    }

    query.prepare(sql);
    int idx = 0;

    if(!(addcols.isEmpty() || addvals.isEmpty())) {
        QString bindparam;
        for(int i = 0; i < addvals.size(); i++) {
            bindparam = addvals.at(i);
            if(bindparam.isEmpty()) {
                query.bindValue(idx++, QVariant(QVariant::String));
            } else {
                query.bindValue(idx++, bindparam);
            }
        }
    }
    exec(&query, "executeQuery");
    return query;
}

/*!
 * \brief Removes an entry from a table
 *
 * This function removes the entry with ID \c id from the table \c table
 * It returns the number of entries actually deleted.
 *
 */
int Database::deleteEntry(const QString &table, const QString &id )
{
    QSqlQuery query;
    query.prepare("DELETE FROM " + table + " WHERE ID=?");
    query.bindValue(0, id );
    exec(&query, "deleteEntry");
    return query.numRowsAffected();
}

/*!
 * \brief Updates an entry in a table
 *
 * This function updates the entry with ID \a id in table of name \a table.
 * The columns to be updated are given in \a updcols and the according values in \a updvals.
 * If the lengths of those 2 QStringLists are different then no update is performed.
 *
 * It returns the number of entries actually updated or -1 if no update was performed.
 */
int Database::updateEntry(const QString &table, const QStringList &updcols, const QStringList &updvals, const QString &id)
{
    if(updcols.size() == updvals.size()) {

        QSqlQuery query;
        QString bindparam;
        QString sql = "UPDATE " + table + " SET ";
        for(int i = 0; i < updcols.size(); i++) {
            if(i>0) {
                sql += ",";
            }
            sql += updcols.at(i);
            sql += "=?";
        }
        sql += ",Datum=? WHERE ID=?";
        query.prepare(sql);
        int idx = 0;
        for(int i = 0; i < updvals.size(); i++) {
            bindparam = updvals.at(i);
            if(bindparam.isEmpty()) {
                query.bindValue(idx++, QVariant(QVariant::String));
            } else {
                query.bindValue(idx++, bindparam);
            }
        }
        query.bindValue(idx++, getTimestamp());
        query.bindValue(idx++, id);
        exec(&query, "updateEntry");
        return query.numRowsAffected();
    }
    return -1;
}

/*!
 * \brief Insert an entry into a table
 *
 * This function inserts an entry into table of name \a table.
 *
 * The columns to be inserted are given in \a updcols and the according values in \a updvals.
 * If the lengths of those 2 QStringLists are different then no insert is performed.
 * Columns not present in those lists are to be filled by default values (according to the SQLite database implementation).
 *
 * It returns the number of entries actually inserted or -1 if no insert was performed.
 */
int Database::insertEntry(const QString &table, const QStringList &updcols, const QStringList &updvals)
{
    if(updcols.size() == updvals.size()) {

        QSqlQuery query;
        QString bindparam;
        QString sql = QString("INSERT INTO %1 (%2,Datum) VALUES (%3,?)").arg(table).arg(updcols.join(",")).arg(QString("?,").repeated(updcols.size()-1).append("?"));
        query.prepare(sql);
        for(int i = 0; i < updvals.size(); i++) {
            bindparam = updvals.at(i);
            if(bindparam.isEmpty()) {
                query.bindValue(i, QVariant(QVariant::String));
            } else {
                query.bindValue(i, bindparam);
            }
        }
        query.bindValue(updvals.size(), getTimestamp());
        exec(&query, "insertEntry");
        return query.numRowsAffected();
    }
    return -1;
}

/*!
 * \brief Count number of entries in a table
 *
 * This function counts the number of entries in table of name \a table.
 *
 * If \a addcols and \a addvals are present and of same length, they are incorporated into a WHERE-clause.
 *
 * This functions returns the number of entries as \c int.
 */
int Database::countEntries(const QString &table, const QStringList &addcols, const QStringList &addvals)
{
    QSqlQuery query;
    QString bindparam;
    QString sql = "SELECT COUNT(*) FROM ";
    sql += table;
    if((!(addcols.isEmpty() || addvals.isEmpty())) &&  (addvals.size() == addcols.size())) {
        sql += " WHERE ";
        for(int i = 0; i < addcols.size(); i++) {
            if(i>0) {
                sql += " AND ";
            }
            sql += addcols.at(i);
            sql += " IS ?";

        }
    }
    query.prepare(sql);
    for(int i = 0; i < addvals.size(); i++) {
        bindparam = addvals.at(i);
        if(bindparam.isEmpty()) {
            query.bindValue(i, QVariant(QVariant::String));
        } else {
            query.bindValue(i, bindparam);
        }
    }
    exec(&query, "countEntries");
    query.next();
    return query.value(0).toInt();
}

/*!
 * \brief Timestamp database migration
 *
 * This function performs the database migration of adding a column 'Datum'
 * containing the timestamp of modification to all tables.
 *
 * The views are recreated if necessary
 *
 * If any modification is actually performed, the settings of the tableview column widths are also removed.
 * (otherwise it would lead to columns being visible).
 */
bool Database::TimestampAdditionMigration()
{
    QSqlQuery query;
    // Has the migration already be performed?
    query.prepare("SELECT COUNT(*) FROM DBMigration WHERE NAME=?");
    query.bindValue(0, "timestapAddition");
    exec(&query, "TimeStampAdditionCount");
    query.next();
    bool isMigrated = query.value(0).toBool();

    if(!isMigrated) {

        // It is not in database and thus yet to do for us.
        bool refreshviews = false;

        QString sqlstring("ALTER TABLE %1 ADD COLUMN Datum Text NOT NULL DEFAULT '%2'");

        if(columnNotExistsForTable("Module", "Datum")) {
            query.prepare(sqlstring.arg("Module").arg(getTimestamp()));
            exec(&query, "ALTER TABLE Module (Timestamp)");
            refreshviews = true;
        }

        if(columnNotExistsForTable("Kurse", "Datum")) {
            query.prepare(sqlstring.arg("Kurse").arg(getTimestamp()));
            exec(&query, "ALTER TABLE Kurse (Timestamp)");
            refreshviews = true;
        }

        if(columnNotExistsForTable("Anerkennungen", "Datum")) {
            query.prepare(sqlstring.arg("Anerkennungen").arg(getTimestamp()));
            exec(&query, "ALTER TABLE Anerkennungen (Timestamp)");
            refreshviews = true;
        }

        if(refreshviews){
            // Recreate Views
            //anerkmodule
            query.exec("DROP VIEW anerkmodule");
            query.prepare("CREATE VIEW anerkmodule AS "
                          "SELECT A.MID AS ID, K.Kursname AS Kursname, K.ECTS AS ECTS, K.Herkunft AS Herkunft, K.Datum AS Datum "
                          "FROM Anerkennungen A JOIN Kurse K ON K.ID = A.KID");
            exec(&query, "CREATE VIEW anerkmodule (Timestamp)");

            // anerkkurse
            query.exec("DROP VIEW anerkkurse");
            query.prepare("CREATE VIEW anerkkurse AS "
                          "SELECT A.KID AS ID, M.Modulname AS Modulname, M.ECTS AS ECTS, M.PO AS PO, M.Datum AS Datum "
                          "FROM Module M JOIN Anerkennungen A ON A.MID=M.ID");
            exec(&query,"CREATE VIEW anerkkurse (Timestamp)");

            // As we have updated some table scheme, we need to remove all the settings of the tableView to avoid inconsistencies
            ConfigManager::getInstance()->removeGroupSettings("tableViewWidths");
        }

        query.prepare("INSERT INTO DBMigration (Name,Datum) VALUES (?,?)");
        query.bindValue(0, "timestapAddition");
        query.bindValue(1, getTimestamp());
        exec(&query, "TimeStampAdditionInsert");
    }
    return true;
}

/*!
 * \brief This function checks for the non existence of columns in a table
 *
 * It queries the table structure of \a table if the \a column exists.
 * This function returns a \c bool which is false if the column does not exists.
 */
bool Database::columnNotExistsForTable(const QString &table, const QString &column)
{
    QSqlQuery query;
    query.prepare(QString("PRAGMA table_info(%1)").arg(table));
    exec(&query, "columnExists");
    while (query.next()) {
        if(column.compare(query.value(1).toString(), Qt::CaseInsensitive) == 0) {
            return false;
        }
    }
    return true;
}

/*!
 * \brief Create a Timestamp
 *
 * This function creates a timestamp of the current date.
 * It returns it as QString in the format yyy-MM-dd hh:mm:ss, that is, 2019-01-25 15:01:59
 */
QString Database::getTimestamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}
