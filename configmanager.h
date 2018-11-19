/*
 * configmanager.h
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

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QtGlobal>
#include <QCoreApplication>
#include <QSettings>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QWidget>
#include <QLocale>
#include <QTranslator>
#include "configdialog.h"

class ConfigManager: public QObject
{
    Q_OBJECT

public:
    ConfigManager();
    ~ConfigManager();
    static ConfigManager * getInstance();

    QVariant readSetting(const QString &key, const QVariant &defaultValue = QVariant(), const QString &group = QString());
    void writeSetting(const QString &key, const QVariant &value, const QString &group = QString());

    void loadSettings();
    QString getDatabaseLocation();

    void execConfigDialog(QWidget *parent);

    void loadLanguage();
private:
    QSettings *persistentConfig;
    QString configFilePath() const;

    QHash<QString, QString> *languages;
    QTranslator *qtTranslator, *qtBaseTranslator, *appTranslator;
};

#endif // CONFIGMANAGER_H
