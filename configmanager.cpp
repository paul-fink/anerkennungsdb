/*
 * configmanager.cpp
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

#include "configmanager.h"

/*!
 * \class ConfigManager
 *
 * \brief This class provides an interface to access global settings
 *
 * It manages the saving and loading of the settings.
 * Furthermore, it allows to query and set the setting from within the application
 *
 * \since 2.0
 */

/*!
 * \brief globalConfigManager
 *
 * This is the static pointer to the application wide ConfigManager
 */
static ConfigManager *globalConfigManager = nullptr;


/*!
 * \brief Constructs the ConfigManager
 *
 * It initialises the pointer to the persisent QSettings object as null pointer.
 * It sets up all the necessary translator objects for the GUI.
 * It sets the pointer of the global application manager to itself and
 * finally loads the settings
 */
ConfigManager::ConfigManager() : persistentConfig(nullptr)
{
    languages = new QHash<QString,QString>();
    qtTranslator = new QTranslator(QCoreApplication::instance());
    qtBaseTranslator = new QTranslator(QCoreApplication::instance());
    appTranslator = new QTranslator(QCoreApplication::instance());
    globalConfigManager = this;
}

/*!
 * \brief Destroys the ConfigManager
 *
 * It makes sure all created instances on the heap are destroyed alongside
 */
ConfigManager::~ConfigManager()
{
    globalConfigManager = nullptr;
    if(persistentConfig) delete persistentConfig;
    delete appTranslator;
    delete qtBaseTranslator;
    delete qtTranslator;
    if(languages) {
        languages->clear();
        delete languages;
    }
}

/*!
 * \brief Returns the pointer to the global ConfigManager
 */
ConfigManager* ConfigManager::getInstance()
{
    Q_ASSERT(globalConfigManager);
    return globalConfigManager;
}

/*!
 * \brief Loads the persistent settings
 *
 * If the persistent settings have not been loaded yet, the settings are loaded from file system
 */
void ConfigManager::loadSettingsFile()
{
    QSettings *config = persistentConfig;
    if(!config) {
        config = new QSettings(QSettings::IniFormat, QSettings::UserScope, "anerkennungsDB", "anerkennungsdb");
        persistentConfig = config;
    }
}

/*!
 * \brief Load all (currently available) settings except database location
 *
 * It reads the settings file and then loads the language and font size settings
 */
void ConfigManager::loadSettings() {
    loadSettingsFile();
    loadLanguage();
    loadFontSize();
}

/*!
 * \fn ConfigManager::readSetting(const QString &key, const QVariant &defaultValue = QVariant(), const QString &group = QString())
 *
 * \brief Returns a specific setting from the persistent settings
 *
 * From the persistent settings a setting of name \a key in \a group is loaded.
 * If the key is not found within the group \a defaultValue is used instead.
 * For generality the setting is returned as QVariant.
 */
QVariant ConfigManager::readSetting(const QString &key, const QVariant &defaultValue, const QString& group)
{
    Q_ASSERT(persistentConfig);
    if(!group.isEmpty()) persistentConfig->beginGroup(group);
    QVariant val = persistentConfig->value(key, defaultValue);
    if(!group.isEmpty()) persistentConfig->endGroup();
    return val;
}

/*!
 * \fn ConfigManager::writeSetting(const QString &key, const QVariant &value, const QString &group = QString())
 *
 * \brief Stores a specific setting in the persistent settings
 *
 * Stores a QVariant \a value as setting with \a key in \a group into the persistent settings.
 */
void ConfigManager::writeSetting(const QString &key, const QVariant &value, const QString &group)
{
    Q_ASSERT(persistentConfig);
    if(!group.isEmpty()) persistentConfig->beginGroup(group);
    persistentConfig->setValue(key, value);
    if(!group.isEmpty()) persistentConfig->endGroup();
    persistentConfig->sync();
}

/*!
 * \brief ConfigManager::removeGroupSettings
 * \param group
 * \return
 */
void ConfigManager::removeGroupSettings(const QString &group)
{
    Q_ASSERT(persistentConfig);
    if(!group.isEmpty()) {
        persistentConfig->beginGroup(group);
        persistentConfig->remove("");
        persistentConfig->endGroup();
        persistentConfig->sync();
    }
}

/*!
 * \brief Returns the database location
 *
 * Reads the specific database path from the settings
 * It creates the read path should it not exist
 *
 * The \c int values correspond to the index of the location combobox in the ConfigDialog
 * \list
 *   \li 0: Database is located in the application directory
 *   \li 1: Database is located in the system wide application directory
 *   \li 2: database is located in a custom directory, which is path is stored in another setting.
 * \endlist
 */
QString ConfigManager::getDatabaseLocation()
{
    int databaseLocationId = readSetting("locationId", 1, "database").toInt();

    QString path;
    if(databaseLocationId == 0) {
        path = QCoreApplication::applicationDirPath();
    } else if(databaseLocationId == 1) {
#if QT_VERSION >= 0x050400
        QStandardPaths::StandardLocation sloc = QStandardPaths::AppLocalDataLocation;
#else
        QStandardPaths::StandardLocation sloc = QStandardPaths::DataLocation;
#endif
        path = QStandardPaths::writableLocation(sloc);

    } else if (databaseLocationId == 2) {
        path = readSetting("locationPath", QStandardPaths::writableLocation(QStandardPaths::HomeLocation), "database").toString();
    }
    if(path.isEmpty()) return QString();

    // ensure directory exists
    QDir dir(path);
    dir.mkpath(path);
    return path;
}

/*!
 * \brief Loads the GUI language and instructs the translators
 */
void ConfigManager::loadLanguage()
{
    QLocale loc = QLocale(readSetting("language", "de", "interface").toString());
    QLocale::setDefault(loc);

    if(qtTranslator->load(loc, "qt", "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        QCoreApplication::installTranslator(qtTranslator);
    }
    if(qtBaseTranslator->load(loc, "qtbase", "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        QCoreApplication::installTranslator(qtBaseTranslator);
    }
    if(appTranslator->load(loc, "anerkennungsdb", "_", ":/translations")) {
        QCoreApplication::installTranslator(appTranslator);
    }
    languages->insert("de", QObject::tr("German"));
    languages->insert("en", QObject::tr("English"));

}

/*!
 * \brief Loads the GUI font size and updates it
 */
void ConfigManager::loadFontSize()
{
    QString interfaceFontFamily = "DejaVu Sans";

    int size = readSetting("fontsize", QApplication::font().pointSize(), "interface").toInt();

    QFont myfont = QFont(interfaceFontFamily, size);
    QApplication::setFont(myfont);
}
/*!
 * \fn ConfigManager::execConfigDialog(QWidget *parent = 0)
 *
 * \brief Handles the config dialog
 *
 * This function creates the ConfigDialog with the given QWidget pointer \a parent.
 * It initialises the settings in the dialog with the ones present in the persistent config.
 * On save of the dialog it also makes sure the (changed) settings are stored in the persistent confi
 */
void ConfigManager::execConfigDialog(QWidget *parent) {

    ConfigDialog *dlg = new ConfigDialog(parent);
    dlg->setDatabaseLocation(readSetting("locationId", 0, "database").toInt(),
                             readSetting("locationPath", "", "database").toString());
    dlg->setLanguage(languages->value(readSetting("language", "de", "interface").toString()));

    dlg->setFontSize(readSetting("fontsize", QApplication::font().pointSize(), "interface").toInt());

    if(dlg->exec()) {
        QString dblocraw = dlg->databaseLocation();
        int sepidx = dblocraw.indexOf(":::");
        writeSetting("locationId",dblocraw.left(sepidx), "database");
        writeSetting("locationPath", dblocraw.remove(0,sepidx + 3), "database");

        writeSetting("language", dlg->language(), "interface");

        int newfontsize = dlg->fontSize();
        writeSetting("fontsize", newfontsize, "interface");
        QApplication::setFont(QFont("DejaVu Sans", newfontsize));

        persistentConfig->sync();
    }

    delete dlg;
}




