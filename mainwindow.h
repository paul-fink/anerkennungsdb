/*
 * mainwindow.h
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define max_search 6

#include <QMainWindow>
#include <QStack>
#include <QLayoutItem>
#include <QtDebug>
#include <QFileDialog>
#include <QTemporaryDir>
#include <QPrintPreviewDialog>
#include <QtGlobal>
#include <QStatusBar>
#include "quazip5/JlCompress.h"
#include "database.h"
#include "modifydialog.h"
#include "transferadddialog.h"
#include "aboutdialog.h"
#include "configmanager.h"
#include "csvwriter.h"
#include "tableprinter.h"
#include "printlayout.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool initGuiElements();
    bool initDatabase();

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private slots:

    void searchConditionAdd();
    void searchConditionRemove();
    void on_searchButtonsSearch_clicked();
    void on_searchButtonsReset_clicked();

    void on_viewComboBox_currentIndexChanged(int index);

    void tableViewSelectionModel_currentRowChanged(const QModelIndex &current, const QModelIndex &previous);
    void tableView_headerResized(int logicalIndex, int oldSize, int newSize);

    void on_editButtonsDelete_clicked();
    void on_editButtonsModify_clicked();
    void on_editButtonsAdd_clicked();

    void on_actionModuleAll_triggered();
    void on_actionCourseAll_triggered();
    void on_actionModuleCourse_triggered();
    void on_actionCourseModule_triggered();
    void on_actionTransfers_triggered();

    void on_actionAbout_triggered();

    void on_actionExportSqlite_triggered();
    void on_actionExportSinglecsv_triggered();
    void on_actionExportCsv_triggered();

    void on_actionRestore_triggered();

    void print(QPrinter *printer);
    void on_actionPrint_triggered();

    void on_readonlyComboBox_currentIndexChanged(int index);

    void on_actionOptions_triggered();

private:
    Ui::MainWindow *ui;

    Database db;
    QSqlQueryModel *tableModel;
    QSortFilterProxyModel *qsfpm;

    QSqlQueryModel *readonlyModel;
    QSortFilterProxyModel *readonlyProxy;

    QStack<QLayoutItem*> *searchWidgetStack;

    ConfigManager cm;

    int ididx;
    int lastididx;
    int nameIndex;

    int lastTableIndex;

    bool hasSearched;   
    bool allowResize;

    QString readonlyId;

    QStringList filefiltersSqlite;
    QStringList filefiltersCsv;

    QString getSelectedId() const;
    QSqlQuery getSelectedQuery();

    void adjustModel(const QString &table, const QStringList &addcols = QStringList(), const QStringList &addvals = QStringList(), const QStringList &connectrelation = QStringList());

    void enablePrint();
    void enableModify();
    void enableSearchButtons();
    void visibilityReadonly();

    void resetViewAndSearch(bool makeEmpty);

    bool isReadonly(const QString &view);
    bool isDeleteAllowed();
    bool isAddAllowed();
    bool isEditAllowed();
    QString getCurrentView() const;
    bool restoreSizeViewColumns(const QString &table = QString());
    bool saveSizeViewColumns(const QString &table = QString());
};

#endif // MAINWINDOW_H
