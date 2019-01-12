/*
 * configdialog.h
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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QtGlobal>
#include <QDialog>
#include <QFileDialog>

namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog();

    void setDatabaseLocation(int locId, const QString &location);
    QString databaseLocation() const;

    void setLanguage(const QString &lang);
    QString language() const;

    void setFontSize(const int size);
    int fontSize() const;

private slots:
    void on_databaseButton_clicked();
    void on_databaseComboBox_currentIndexChanged(int index);

private:
    Ui::ConfigDialog *ui;
    void initGuiElements();
    void setDatabaseEditEnabled(bool enable);
};

#endif // CONFIGDIALOG_H
