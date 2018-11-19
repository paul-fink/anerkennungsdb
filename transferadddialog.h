/*
 * transferadddialog.h
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

#ifndef TRANSFERADDDIALOG_H
#define TRANSFERADDDIALOG_H

#include <QDialog>
#include <QtSql>
#include <QDebug>
#include "database.h"


namespace Ui {
class TransferAddDialog;
}

class TransferAddDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransferAddDialog(Database *mydb, QWidget *parent = 0);
    ~TransferAddDialog();
    inline QString getMid() const {return mid;}
    inline QString getCid() const {return cid;}

private slots:
    void on_kursNameComboBox_currentIndexChanged(int index);

    void on_modulNameComboBox_currentIndexChanged(int index);

private:
    Ui::TransferAddDialog *ui;
    Database *db;
    QSqlQueryModel *coursemodel;
    QSqlQueryModel *modulemodel;

    static void checkError(QSqlError error, const QString &text);
    void initModels();
    void enableOkButton();
    QString mid;
    QString cid;

};

#endif // TRANSFERADDDIALOG_H
