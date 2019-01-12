/*
 * mainwindow.cpp
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

#include "mainwindow.h"
#include "ui_mainwindow.h"

/*!
 * \class MainWindow
 *
 * \brief The main window of the application
 *
 * \since 1.0
 */

/*!
 * \brief Constructs the main application window
 *
 * The main application window is constructed as extension to QMainWindow, to which \a parent is passed.
 *
 * All objects which are created on the heap are initiallized.
 * Additionally some constant lists are filled.
 */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    ididx(-1),
    lastididx(-1),
    lastTableIndex(-1),
    hasSearched(false), allowResize(true)
{
    ConfigManager::getInstance()->loadSettings();
    ui->setupUi(this);
    tableModel = new QSqlQueryModel();
    qsfpm = new QSortFilterProxyModel();
    readonlyModel = new QSqlQueryModel();
    readonlyProxy = new QSortFilterProxyModel();
    searchWidgetStack = new QStack<QLayoutItem*>();
    filefiltersSqlite << tr("SQLite database (*.sqlite)") << tr("All files (*)");
    filefiltersCsv << tr("CSV (*.csv)") << tr("All files (*)");
}

/*!
 * \brief Destroys the main application window
 *
 * It takes care that all object generated on the heap are properly destroyed.
 */
MainWindow::~MainWindow()
{
    if(searchWidgetStack) {
        while(searchWidgetStack->count()>0) {
            ui->scrollSearchContents->layout()->addItem(searchWidgetStack->pop());
        }
        delete searchWidgetStack;
    }
    delete qsfpm;
    delete readonlyProxy;
    delete tableModel;
    delete ui;
}

/*!
 * \brief Initialise the database
 *
 * If opening the database is successful \c true is returned, otherwise \c false.
 */
bool MainWindow::initDatabase() {
    return this->db.openDatabase();
}

/*!
 * \brief Initialise the GUI
 *
 * This functions takes care of setting up all customized GUI elements:
 * \list
 *   \li The window is recreated according to the last used sizes
 *   \li The combobox for the view is constructed with translatable entries
 *   \li Setting up the dynamic search area:
 *   \list
 *     \li Hiding all but first condition row
 *     \li Filling the relation combobox with translatable contents
 *     \li Enabling the a clear button in the entry text field
 *     \li Connecting signals to the add/remove condition buttons
 *   \endlist
 *   \li Putting the hidden condition rows on a stack
 *   \li Changing the looks for the tableView if on Windows 10
 *   \li Dis-/Enable elements for the readOnly view (which takes care of enabling the search buttons) via \l MainWindow::visibilityReadonly
 *   \li Disable the add/modify/delete buttons via \l MainWindow::enableModify()
 *   \li Disable the print entry in menu via \l MainWindow::enablePrint()
 * \endlist
 *
 * If no error occurred \c true is returned
 */
bool MainWindow::initGuiElements()
{
    // restore the last sizes, which are stored in the settings
    restoreGeometry(ConfigManager::getInstance()->readSetting("windowgeometry", 0, "applicationsize").toByteArray());
    restoreState(ConfigManager::getInstance()->readSetting("windowstate", 0, "applicationsize").toByteArray());
    ui->mainsplitter->restoreState(ConfigManager::getInstance()->readSetting("splittersizes", 0, "applicationsize").toByteArray());

    QComboBox *viewBox = ui->viewComboBox;
    if(viewBox) {
        // Blocking signals to avoid actions being triggered on adding items to the combobox
        // (addItem triggers and indexChanged signal)
        viewBox->blockSignals(true);
        viewBox->addItem(tr("Courses"), "Kurse");
        viewBox->addItem(tr("Modules"), "Module");
        viewBox->addItem(tr("Courses per module (not editable)"), "anerkmodule");
        viewBox->addItem(tr("Modules per course (not editable)"), "anerkkurse");
        viewBox->addItem(tr("Transfers"), "Anerkennungen");
        viewBox->setCurrentIndex(-1);
        viewBox->blockSignals(false);
    }

    // create 'max_search' rows of gui elements for a single search condition
    // All required GUI elements are defined in the UI
    for (int i = 0; i < max_search; i++) {

        QWidget *scrollWidget = ui->scrollSearchContents->findChild<QWidget *>(QString("scrollSearchWidget%1").arg(i));
        if(scrollWidget && i!=0) scrollWidget->hide();

        QComboBox *relationBox = ui->scrollSearchContents->findChild<QComboBox *>(QString("scrollSearchRelation%1").arg(i));
        if(relationBox) {
            relationBox->addItem(tr("contains"), " LIKE ");
            relationBox->addItem(tr("does not contain"), " NOT LIKE ");
            relationBox->addItem(tr("equals"), " IS ");
            relationBox->addItem(tr("does not equal"), " IS NOT ");
        }

        QLineEdit *inputEdit = ui->scrollSearchContents->findChild<QLineEdit *>(QString("scrollSearchInput%1").arg(i));
        if(inputEdit) {
            inputEdit->setClearButtonEnabled(true);
        }

        QPushButton *addButton = ui->scrollSearchContents->findChild<QPushButton *>(QString("scrollSearchAdd%1").arg(i));
        if(addButton) {
            connect(addButton, SIGNAL(clicked()), this, SLOT(searchConditionAdd()));
        }

        QPushButton *removeButton = ui->scrollSearchContents->findChild<QPushButton *>(QString("scrollSearchRemove%1").arg(i));
        if(removeButton) {
            if(i == 0) removeButton->setEnabled(false);
            connect(removeButton, SIGNAL(clicked()), this, SLOT(searchConditionRemove()));
        }
    }

    // Push the hidden ones onto a stack in reverse ordered, so that element with id=1 is  on top (id=0 is visible)
    for(int i = (max_search - 1); i > 0; i--) {
        searchWidgetStack->push(ui->scrollSearchContents->layout()->takeAt(i));
    }

#ifdef Q_OS_WIN
    if(QSysInfo::windowsVersion()==QSysInfo::WV_WINDOWS10)
    ui->viewTable->setStyleSheet(
                "QHeaderView::section{"
                    "border-top:0px solid #D8D8D8;"
                    "border-left:0px solid #D8D8D8;"
                    "border-right:1px solid #D8D8D8;"
                    "border-bottom: 1px solid #D8D8D8;"
                    "background-color:#DDDDDD;"
                    "padding:4px;"
                "}");
#endif
    visibilityReadonly(); // also takes care of enabling the search buttons
    enableModify();
    enablePrint();

    return true;
}

/*!
 * \fn MainWindow::adjustModel(const QString &table, const QStringList &addcols = QStringList(), const QStringList &addvals = QStringList(), const QStringList &connectrelation = QStringList())
 *
 * \brief Main function adjusting the contents of the table view
 *
 * This function populates the main table view area.
 * Furthermore, it makes sure that the GUI is adapted accordingly to the contents displayed in it.
 *
 * The contents of the database table of name \a table are displayed in it.
 * The additional arguments \a addcols, \a addvals and \a connectrelation are used to restrict the contents queried from the database (see also \l Database::executeQuery())
 *
 * If the table is not read-only, a text will be displayed in the status bar
 * on how many of the entries of that database table are actually displayed.
 */
void MainWindow::adjustModel(const QString &table, const QStringList &addcols, const QStringList &addvals, const QStringList &connectrelation)
{

    // first all models are cleared
    tableModel->clear();
    qsfpm->clear();

    // if table is empty just return
    if(table.isEmpty()) {
        enableSearchButtons();
        statusBar()->clearMessage();
        return;
    }

    ui->viewTable->horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);

    // Create the basic elements for Database::executeQuery()
    QString mytable = table;
    QString selectcols = "*";

    // If Anerkennungen table is to be displayed, the statement needs to be adjusted to perform the actual join, otherwise only IDs would be displayed
    if(table == "Anerkennungen") {
        mytable = "Module M JOIN Anerkennungen A ON M.ID = A.MID JOIN Kurse K ON K.ID = A.KID";
        selectcols = "A.ID AS ID, K.Kursname AS 'Kurs-Name', K.ECTS AS 'Kurs-ECTS', K.Herkunft AS 'Kurs-Herkunft', M.Modulname AS 'Modul-Name', M.ECTS AS 'Modul-ECTS', M.PO AS 'Modul-PO', A.Datum AS 'Datum'";
    }

    QStringList addcolums, addvalues, connectrel;

    // if the readonlyId contains an element, add it at first into the list of restrictions
    if(!readonlyId.isEmpty()) {
        addcolums.append("ID IS ");
        addvalues.append(readonlyId);
        connectrel.append(" AND (");
    }
    addcolums.append(addcols);
    addvalues.append(addvals);
    connectrel.append(connectrelation);

    QSqlQuery selectquery = db.executeQuery(mytable, addcolums, addvalues, selectcols, connectrel);
    if(selectquery.lastError().isValid()) {
        qCritical() << tr("Error in query 'selectquery':") << selectquery.lastError();
    }

    // set the query to the model
    tableModel->setQuery(selectquery);
    if(tableModel->lastError().isValid()){
        qCritical() << tr("Error setting query to 'tableModel':") << tableModel->lastError();
    }

    // As QSqlQuery does lazy loading, so not all are initially displayed
    // Fetching all entries first allows to display all as desired
    while (tableModel->canFetchMore()) tableModel->fetchMore();

    // disallow save of column sizes
    allowResize = false;

    // Create the horizontal header for the table view
    ididx = -1;
    QMap<QString, QString> columnnames;
    for(int i = 0; i < tableModel->columnCount(); i++) {

        QString colname = tableModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        // store the index of the ID column in a member
        if(QString("ID").compare(colname, Qt::CaseInsensitive) == 0) {
            ididx = i;
        } else if(table == "Anerkennungen") {
            // Due to the join the header column names and the database column names differ:
            // Store the according database column names in a map
            if(QString("Kurs-Name").compare(colname, Qt::CaseInsensitive) == 0) {
                columnnames.insert(colname, "K.Kursname");
            } else if(QString("Kurs-ECTS").compare(colname, Qt::CaseInsensitive) == 0) {
                columnnames.insert(colname, "K.ECTS");
            } else if(QString("Kurs-Herkunft").compare(colname, Qt::CaseInsensitive) == 0) {
                columnnames.insert(colname, "K.Herkunft");
            } else if(QString("Modul-Name").compare(colname, Qt::CaseInsensitive) == 0) {
                columnnames.insert(colname, "M.Modulname");
            } else if(QString("Modul-ECTS").compare(colname, Qt::CaseInsensitive) == 0) {
                columnnames.insert(colname, "M.ECTS");
            } else if(QString("Modul-PO").compare(colname, Qt::CaseInsensitive) == 0) {
                columnnames.insert(colname, "M.PO");
            } else if(QString("Datum").compare(colname, Qt::CaseInsensitive) == 0) {
                columnnames.insert(colname, "A.Datum");
            }
        } else {
            // directly store header names (which are same as database column names) in map
            columnnames.insert(colname,colname);
        }
        tableModel->setHeaderData(i, Qt::Horizontal, colname);
    }

    //Create a proxy model to allow for sorting
    qsfpm->setSourceModel(tableModel);
    qsfpm->setSortLocaleAware(true);
    qsfpm->sort(-1, Qt::AscendingOrder);

    // table view takes proxy model as model
    ui->viewTable->setModel(qsfpm);

    // Hide the ID column:
    // First unhide the previous id column and then id the new one
    if(lastididx > -1) {
        ui->viewTable->showColumn(lastididx);
    }
    if(ididx > -1){
        ui->viewTable->hideColumn(ididx);
        lastididx = ididx;
    }

    // Customisation of the features of the table view
    ui->viewTable->resizeColumnsToContents();

    // allow save of column sizes
    allowResize = true;
    ui->viewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->viewTable->horizontalHeader()->setStretchLastSection(true);
    ui->viewTable->verticalHeader()->setVisible(false);
    ui->viewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->viewTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->viewTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->viewTable->setSortingEnabled(true);

    // connect a row change to a (custom) signal
    connect(ui->viewTable->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
                this, SLOT(tableViewSelectionModel_currentRowChanged(QModelIndex, QModelIndex)));
    // connect a column header rezise event to a slot performing row height resizing
    connect(ui->viewTable->horizontalHeader(), SIGNAL(sectionResized(int, int, int)), this,
            SLOT(tableView_headerResized(int, int, int)));
    // restore the saved sizes
    restoreSizeViewColumns();
    ui->viewTable->resizeRowsToContents();



    // insert the column names into the comboboxes of the search condition rows
    nameIndex = -1;
    if(addvals.isEmpty() && addcols.isEmpty()) {

        for(int i = 0; i < max_search; i++) {
            QComboBox *searchCategoryBox = ui->scrollSearchContents->findChild<QComboBox *>(QString("scrollSearchField%1").arg(i));
            searchCategoryBox->clear();
            // Let the header names be displayed, the database names are stored in a differnt role
            QMap<QString,QString>::const_iterator it = columnnames.constBegin();
            while (it != columnnames.constEnd()) {
                searchCategoryBox->addItem(it.key(),it.value());
                ++it;
            };
            // sort the texts
            searchCategoryBox->model()->sort(0);
            if(nameIndex < 0) {
                nameIndex  = searchCategoryBox->findData("name", Qt::UserRole, Qt::MatchContains);
            }
            // pre-select the combobox entry which has "name" in it
            searchCategoryBox->setCurrentIndex(nameIndex);
        }
    }

    // dis- or enable the search buttons
    enableSearchButtons();

    // Display text in status bar
    if(!isReadonly(table)) {
        statusBar()->showMessage(tr("Displaying %1 of %2 entries").arg(qsfpm->rowCount()).arg(db.countEntries(table)), 10000);
    } else {
        statusBar()->clearMessage();
    }
}

bool MainWindow::restoreSizeViewColumns(const QString &table)
{
    QHeaderView *header = ui->viewTable->horizontalHeader();
    QString view = table;
    if(view.isEmpty()) {
        view = getCurrentView();
    }
    int ncols = header->count();
    if(ncols > 0) {
        allowResize = false;
        QStringList savedValues = ConfigManager::getInstance()->readSetting(QString("%1-sizes").arg(view), QVariant(), "tableViewWidths").toString().split(",");
        if(savedValues.size() == 1) return false;
        for(int i = 0; i < ncols; ++i) {
            if(!header->isSectionHidden(i)) {
                header->resizeSection(i, savedValues.at(i).toInt());
            }
        }
        allowResize = true;
        return true;
    }
    return false;
}

bool MainWindow::saveSizeViewColumns(const QString &table)
{
    QHeaderView *header = ui->viewTable->horizontalHeader();
    QString view = table;
    if(view.isEmpty()) {
       view = getCurrentView();
    }
    QStringList columnSizes;
    int ncols = header->count();
    if(ncols > 0) {
        for(int i = 0; i < ncols; ++i) {
            if(!header->isSectionHidden(i)) {
                columnSizes << QString::number(header->sectionSize(i));
            } else {
                columnSizes << "";
            }
        }
        ConfigManager::getInstance()->writeSetting(QString("%1-sizes").arg(view), columnSizes.join(","), "tableViewWidths");
        return true;
    }
    return false;
}

/*!
 * \brief Dis- or enables the print menu
 *
 * The print menu entry is only enabled when a proper table is displayed, even if it contains no entries
 */
void MainWindow::enablePrint()
{
    QString view = getCurrentView();
    bool enabled = view.isEmpty();
    if(isReadonly(view)) {
        enabled = ui->readonlyComboBox->currentIndex() < 0;
    }
    ui->actionPrint->setEnabled(!enabled);
}

/*!
 * \brief Dis- or enables the modify button
 *
 * \sa isAddAllowed(), isEditAllowed(), isDeleteAllowed()
 */
void MainWindow::enableModify()
{
    ui->editButtonsAdd->setEnabled(isAddAllowed());
    ui->editButtonsModify->setEnabled(isEditAllowed());
    ui->editButtonsDelete->setEnabled(isDeleteAllowed());
}

/*!
 * \brief Dis- or enables the search buttons
 *
 * The search button is only enabled, when a proper table is displayed, even if it contains no entries
 * The reset button has same condition as the search button, but also looks if a search has been performed previously
 */
void MainWindow::enableSearchButtons()
{
    QString view = getCurrentView();
    bool isNotEmptyView = !view.isEmpty();
    if(isReadonly(view)) {
        isNotEmptyView = ui->readonlyComboBox->currentIndex() >-1;
    }
    ui->searchButtonsSearch->setEnabled(isNotEmptyView);
    ui->searchButtonsReset->setEnabled(isNotEmptyView && hasSearched);
}

/*!
 * \brief Special visibility when read-only table is displayed
 *
 * A further combobox is displayed in readonly mode to select a specific/course module and display the transfers for it
 * This function checks for the appropriate visibility of the element:
 * For a read-only table the combobox is displayed and populated.
 * Otherwise it is hidden, as well as all read-only elements/members are resetted.
 */
void MainWindow::visibilityReadonly()
{
    QString view = getCurrentView();
    readonlyModel->clear();
    readonlyProxy->clear();

    // if the current table is a readonly, display another combobox
    if(isReadonly(view)) {

        // unhide the combobox
        ui->readonlywidget->show();

        // create aproxy model which has condensed information on the courses/modules
        QString columns, table, label;
        if(view == "anerkkurse") {
            columns = "ID, (Kursname || ' [' || ECTS || ']') AS Name";
            table = "Kurse";
            label = tr("Course") + ": ";
        } else {
            columns = "ID, (Modulname || ' [' || ECTS || ']') AS Name";
            table = "Module";
            label = tr("Module") + ": ";
        }
        readonlyModel->setQuery(db.executeQuery(table,QStringList(),QStringList(),columns));
        readonlyProxy->setSourceModel(readonlyModel);
        readonlyProxy->setSortLocaleAware(true);
        readonlyProxy->sort(1, Qt::AscendingOrder);

        // Disable signals to avoid event triggered when setting the model to the combobox
        ui->readonlyComboBox->blockSignals(true);
        ui->readonlyComboBox->setModel(readonlyProxy);
        ui->readonlyComboBox->setModelColumn(1);
        ui->readonlyComboBox->setCurrentIndex(-1);
        ui->readonlyComboBox->blockSignals(false);
        ui->readonlyLabel->setText(label);
    } else {
        // no readonly mode, so rewind everything and hide the combobox
        ui->readonlywidget->hide();
        ui->readonlyComboBox->setCurrentIndex(-1);
        ui->readonlyComboBox->clear();
        readonlyId.clear();
    }

    // dis- or enable search buttons
    enableSearchButtons();
}

/*!
 * \brief Resets the view and Search area
 *
 * This function resets the view and the search area.
 * If \a makeEmpty is \c false then the table view is made entirely empty and even the column names search element is cleared.
 *
 * This function is called if the view is changed or when the search is reset.
 */
void MainWindow::resetViewAndSearch(bool makeEmpty)
{
    // Reset the member variable indicating a search has happened
    hasSearched = false;
    QString view = getCurrentView();

    if(isReadonly(view) && makeEmpty) {
        // make everything empty
        view = "";
        nameIndex = -1;
    }
    adjustModel(view);
    for(unsigned int i = 0; i < max_search; i++) {

        QComboBox *searchFieldBox = ui->scrollSearchContents->findChild<QComboBox *>(QString("scrollSearchField%1").arg(i));

        // clear all visible user inputs
        if(searchFieldBox && searchFieldBox->isVisible()) {

            searchFieldBox->setCurrentIndex(nameIndex);
            ui->scrollSearchContents->findChild<QComboBox *>(QString("scrollSearchRelation%1").arg(i))->setCurrentIndex(0);
            ui->scrollSearchContents->findChild<QLineEdit *>(QString("scrollSearchInput%1").arg(i))->clear();
        }
    }
}

/*!
 * \brief Custom signal on selected row changed
 *
 * This signal is process when the current selection index of the table view changes.
 * It chekcs if modify buttons need to be dis- or enabled.
 *
 * \warning The arguments \a current and \a previous are unused.
 */
void MainWindow::tableViewSelectionModel_currentRowChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)

    enableModify();
}


/*!
 * \brief Custom singal on resized column
 *
 * This signal adapts the height of the rows so the content is still entirely visible.
 * It also saves the layout of the current tableview to settings
 *
 * \warning The arguments \a logicalIndex, \a oldSize and \a newSize are unused
 */
void MainWindow::tableView_headerResized(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(logicalIndex)
    Q_UNUSED(oldSize)
    Q_UNUSED(newSize)
    ui->viewTable->resizeRowsToContents();
    if(allowResize) {
        saveSizeViewColumns();
    }
}

/*!
 * \brief Returns the database ID of a selected entry
 *
 * This functions returns the database ID of a selected entry in the table view as QString.
 * If there is no selection model or when no entry is selected or when the table view has no (hidden) ID column,
 * then a default constructed QString is returned.
 */
QString MainWindow::getSelectedId() const
{
    if(!ui->viewTable->selectionModel()) return QString();

    QModelIndex qidx = qsfpm->mapToSource(ui->viewTable->selectionModel()->currentIndex());
    if(qidx.isValid() && (ididx > -1)) {
        return qsfpm->sourceModel()->data(qsfpm->sourceModel()->index(qidx.row(),ididx)).toString();
    }
    return QString();
}

/*!
 * \brief Returns the full database entry of the selected item
 *
 * This function returns the complete database entry of the selected entry as a single QSqlQuery.
 * If the applied \l getSelectedId() returns an empty ID, a default constructed QSqlQuery is returned.
 */
QSqlQuery MainWindow::getSelectedQuery()
{
    QString id = getSelectedId();

    if(!id.isEmpty()) {
        return (db.executeQuery(getCurrentView(), QStringList("ID ="), QStringList(id)));
    }
    return QSqlQuery();
}

/*!
 * \brief Returns the readonly mode
 *
 * If \a view corresponds to a database view, this function returns \c true, and otherwise \c false.
 */
bool MainWindow::isReadonly(const QString &view)
{
    return (QString("anerkmodule").compare(view) == 0) || (QString("anerkkurse").compare(view) == 0);
}

/*!
 * \brief Returns if adding is allowed
 *
 * The addition is allowed if the current view is neither empty nor read only.
 * In this case \c true is returned, otherwise \c false.
 */
bool MainWindow::isAddAllowed()
{
    QString view = getCurrentView();
    return (!view.isEmpty() && !isReadonly(view));
}

/*!
 * \brief Returns if editing is allowed
 *
 * The editing is allowed only if the current view is neither empty nor read only nor contains transfers
 * and one entry is selected.
 * In this case \c true is returned, otherwise \c false.
 */
bool MainWindow::isEditAllowed()
{
    return (isAddAllowed() &&
            (QString("Anerkennungen").compare(getCurrentView()) != 0) &&
            !getSelectedId().isEmpty());
}

/*!
 * \brief Returns if deleting is allowed
 *
 * The deletion is not allowed if the current view is empty or read only or no entry is selected.
 * Furthermore the deletion of courses or modules is prohibited if they are used within transfers.
 * If deletion is allowed \c true is returned, otherwise \c false.
 */
bool MainWindow::isDeleteAllowed()
{
    QString view = getCurrentView();
    if(view.isEmpty() || getSelectedId().isEmpty() || isReadonly(view)) {
        return false;
    }
    // Course/Module is used in transfers?
    if(QString("Kurse").compare(view) == 0) {
        return (db.countEntries("Anerkennungen", QStringList("KID"), QStringList(getSelectedId())) == 0);
    }
    if(QString("Module").compare(view) == 0) {
        return (db.countEntries("Anerkennungen", QStringList("MID"), QStringList(getSelectedId())) == 0);
    }
    if(QString("Anerkennungen").compare(view) == 0) {
        return true;
    }
    return false;
}

/*!
 * \brief Adds a search condition row
 *
 * A hidden search condition row is taken from the top of the search stack and displayed.
 * If it was the last all add buttons are disabled
 */
void MainWindow::searchConditionAdd()
{
    QLayout *layout = ui->scrollSearchContents->layout();

    // Take the top element from the stack and add it to the layout.
    if(!searchWidgetStack->isEmpty()) {
        QLayoutItem *item = searchWidgetStack->pop();
        // Beware! Last item is a spacer
        QLayoutItem *spacer = layout->takeAt(layout->count()-1);
        layout->addItem(item);
        layout->addItem(spacer);
        item->widget()->show();
    }

    // if stack is empty after taking the top element, disable all add buttons
    if(searchWidgetStack->isEmpty()) {
        for(int i=0; i < max_search; i++) {
            QPushButton *searchAddButtonGen = ui->scrollSearchContents->findChild<QPushButton *>(QString("scrollSearchAdd%1").arg(i));
            if(searchAddButtonGen) searchAddButtonGen->setEnabled(false);
        }
    }

    // If there had been only one row previously, enable its remove button
    // layout =3 : old row + new row + spacer
    if(layout->count() == 3) {
        QPushButton *searchRemoveButtonFirst = ui->scrollSearchContents->findChild<QPushButton *>(QString("scrollSearchRemove%1").arg(layout->itemAt(0)->widget()->objectName().right(1)));
        if(searchRemoveButtonFirst) searchRemoveButtonFirst->setEnabled(true);
    }
}

/*!
 * \brief Removes a search condition row
 *
 * A visible search condition row is cleared and put on top of the search stack and hidden.
 * If only one row remains visible its remove buttons is disabled.
 * If all add buttons were disabled, it enables them now.
 */
void MainWindow::searchConditionRemove()
{
    QLayout *layout = ui->scrollSearchContents->layout();

    QPushButton* button = qobject_cast<QPushButton*>(sender());
    QString id = button->objectName().right(1);

    QWidget *widget = ui->scrollSearchContents->findChild<QWidget *>(QString("scrollSearchWidget%1").arg(id));
    if(widget) {

        // Remove the search condition row from the layout and put on stack
        for(int i = 0; i < layout->count(); i++) {
            if(layout->itemAt(i)->widget() == widget) {
                searchWidgetStack->push(layout->takeAt(i));
                break;
            }
        }
        // Hide the search condition row
        widget->hide();

        // If only one search condition row is visible , disable the remove button
        if(layout->count() == 2) {
            QPushButton *searchRemoveButtonFirst = ui->scrollSearchContents->findChild<QPushButton *>(QString("scrollSearchRemove%1").arg(layout->itemAt(0)->widget()->objectName().right(1)));
            if(searchRemoveButtonFirst) searchRemoveButtonFirst->setEnabled(false);
        }
    }

    // Enable all add buttons again if they were previously disabled
    if(searchWidgetStack->size() == 1) {
        for(int i=0; i < max_search; i++) {
            QPushButton *searchAddButtonGen = ui->scrollSearchContents->findChild<QPushButton *>(QString("scrollSearchAdd%1").arg(i));
            if(searchAddButtonGen) searchAddButtonGen->setEnabled(true);
        }
    }

    // Clear / set to default all contents of the hidden search condition row
    QComboBox *searchFieldBox = ui->scrollSearchContents->findChild<QComboBox *>(QString("scrollSearchField%1").arg(id));
    if(searchFieldBox) searchFieldBox->setCurrentIndex(nameIndex);

    QComboBox *searchRelationBox = ui->scrollSearchContents->findChild<QComboBox *>(QString("scrollSearchRelation%1").arg(id));
    if(searchRelationBox) searchRelationBox->setCurrentIndex(0);

    QLineEdit *searchInputLine = ui->scrollSearchContents->findChild<QLineEdit *>(QString("scrollSearchInput%1").arg(id));
    if(searchInputLine) searchInputLine->clear();

}

/*!
 * \brief The search button is clicked
 *
 * A search is initiated.
 * The visible search conditions are processed and used as restrictions for the call to \l MainWindow::adjustModel().
 * Additionally, the member variable indicating as search has happened is set to \c true.
 */
void MainWindow::on_searchButtonsSearch_clicked()
{
    // How to link the conditions?
    bool conditionOr = (!ui->searchModeAllButton->isChecked()) && (ui->searchModeAnyButton->isChecked());
    QStringList conditionCols, conditionVals, relations;

    // create the restriction rules for adjustModel call based on the inputs
    for(unsigned int i = 0; i < max_search; i++) {

        QComboBox *searchFieldBox = ui->scrollSearchContents->findChild<QComboBox *>(QString("scrollSearchField%1").arg(i));

        // Only process visible search condition rows
        if(searchFieldBox && searchFieldBox->isVisible()) {

            QComboBox *searchRelationBox = ui->scrollSearchContents->findChild<QComboBox *>(QString("scrollSearchRelation%1").arg(i));
            QLineEdit *searchInputLine = ui->scrollSearchContents->findChild<QLineEdit *>(QString("scrollSearchInput%1").arg(i));

            // Escape special SQL characters
            QString input = searchInputLine->text().replace("!", "!!")
                    .replace("%", "!%")
                    .replace("_", "!_")
                    .replace("[", "![");

            // wrap like clauses in placeholders
            QString relation = searchRelationBox->currentData().toString();
            if(relation.contains("LIKE")) {
                input = "%" + input + "%";
            }

            conditionCols << (searchFieldBox->currentData().toString() + relation);
            conditionVals << input;
            if(i>0) {
                relations << (conditionOr ? " OR " : " AND ");
            }
        }
    }

    // set member for search to true
    hasSearched = true;

    // adjust the table view
    adjustModel(getCurrentView(), conditionCols, conditionVals, relations);
}

/*!
 * \brief Search reset button is clicked
 *
 * The view is reset but not made empty
 */
void MainWindow::on_searchButtonsReset_clicked()
{
    resetViewAndSearch(false);
}

/*!
 * \brief Delete button is clicked
 *
 * Opens a dialog to make sure deletion was desired.
 * If it was desired, the selected entry is removed from the database.
 * In the end the table view is created anew via a call to \l MainWindow::adjustModel()
 *
 * This signal does nothing if deletion was not allowed.
 *
 * \sa adjustModel(), isDeleteAllowed()
 */
void MainWindow::on_editButtonsDelete_clicked()
{
    if(!ui->editButtonsDelete->isEnabled()) {
        // Nothing to be done if deletion was not allowed
        return;
    }

    QMessageBox *deleteDialog = new QMessageBox(QMessageBox::Question,
                                                tr("Delete entry"),
                                                tr("Would you like to delete the selected entry?"),
                                                QMessageBox::No | QMessageBox::Yes, this);

    if(deleteDialog->exec() == QMessageBox::Yes) {
        // Remove the selected entry from the database
        QString view = getCurrentView();
        db.deleteEntry(view,getSelectedId());
        adjustModel(view);
    }

    delete deleteDialog;
}

/*!
 * \brief Edit button is clicked
 *
 * Opens a dialog to edit the selected entry.
 * The values in the dialog are prefilled with the ones currently stored in the database.
 * If editing is successful they are stored in the database and the table view is created anew via a call to \l MainWindow::adjustModel()
 *
 * This signal does nothing if editing was not allowed.
 *
 * \sa adjustModel(), isEditAllowed()
 */
void MainWindow::on_editButtonsModify_clicked()
{
    if(!ui->editButtonsModify->isEnabled()) {
        // noting to be done if editing is not allowed
        return;
    }

    QString view = getCurrentView();
    ModifyDialog *dialog = new ModifyDialog(view, this);

    // preset values in dialog with database entry of selected value
    dialog->presetValues(getSelectedQuery());

    if(dialog->exec() == QDialog::Accepted ) {
        // update the entry in database
        QStringList colList, valList;
        colList << ((view == "Kurse") ? "Kursname" : "Modulname") << "ECTS" << ((view == "Kurse") ? "Herkunft" : "PO");
        valList << dialog->getNameValue() << QString::number(dialog->getEctsValue()) << dialog->getOtherValue();
        db.updateEntry(view,colList,valList,getSelectedId());
        adjustModel(view);
    }
    delete dialog;
}

/*!
 * \brief Add button is clicked
 *
 * Opens a dialog to add an entry.
 *
 * The type of the dialog is different if the view is 'Anerkennungn'.
 * If adding is successful the values are stored in the database and the table view is created anew via a call to \l MainWindow::adjustModel()
 *
 * This signal does nothing if adding was not allowed.
 *
 * \sa adjustModel(), isAddAllowed()
 */
void MainWindow::on_editButtonsAdd_clicked()
{
    if(!ui->editButtonsAdd->isEnabled()) {
        // noting to be done if editing is not allowed
        return;
    }

    QString view = getCurrentView();

    if(view == "Anerkennungen") {

        // Different dialog for 'Anerkennungen'
        TransferAddDialog *dialog = new TransferAddDialog(&db, this);

        if(dialog->exec() == QDialog::Accepted) {
            // Insert values into database
            QStringList colList, valList;
            colList << "KID" << "MID";
            valList << dialog->getCid() << dialog->getMid();
            db.insertEntry(view, colList, valList);
            adjustModel(view);
        }
        delete dialog;

    } else {

        ModifyDialog *dialog = new ModifyDialog(view, this, true);

        if(dialog->exec() == QDialog::Accepted ) {
            // Insert values into database
            QStringList colList, valList;
            // Colnames depend on the view
            colList << ((view == "Kurse") ? "Kursname" : "Modulname") << "ECTS" << ((view == "Kurse") ? "Herkunft" : "PO");
            valList << dialog->getNameValue() << QString::number(dialog->getEctsValue()) << dialog->getOtherValue();
            db.insertEntry(view, colList, valList);
            adjustModel(view);

        }

        delete dialog;
    }
}

/*!
 * \brief Returns the current view
 *
 * Convenience function accessing the current view from the view combobox
 */
QString MainWindow::getCurrentView() const
{
    return ui->viewComboBox->currentData().toString();
}

/*!
 * \brief Print the current view
 *
 * Print the current view. \a printer is a poitner to a default constructed QPrinter.
 *
 * If something goes wrong a warning is emitted.
 *
 * \note Currently the layout is hard coded in here. In a future versions the layout settings could be made customisable via the config mananger
 */
void MainWindow::print(QPrinter *printer)
{
    QPainter painter;
    if(!painter.begin(printer)) {
        qWarning() << tr("Unable to start printer");
        return;
    }

    TablePrinter tablePrinter(&painter, printer);

    // Layout configuration
    // Pen for borders
    tablePrinter.setPen(QPen(Qt::gray, 1, Qt::DashLine, Qt::RoundCap));

    // Font for headers
    QFont font1;
    font1.setBold(true);
    font1.setItalic(true);
    tablePrinter.setHeadersFont(font1);
    tablePrinter.setHeaderColor(Qt::black);

    // Font for content
    tablePrinter.setContentFont(QFont());
    tablePrinter.setContentColor(Qt::black);

    // Layout for page
    PrintLayout *printB = new PrintLayout;
    printB->pageNumber = 1;
    printB->setPageName(getCurrentView());
    tablePrinter.setPagePrepare(printB);
    tablePrinter.setCellMargin(5, 5, 5, 5);
    tablePrinter.setPageMargin(40, 40, 40, 40);

    // Set the column stretch according to the current in the table view
    QVector<int> colStretch;
    for(int i = 0; i < ui->viewTable->model()->columnCount(); i++) {
        int columnwidth = 0;
        if(!ui->viewTable->isColumnHidden(i)) {
            columnwidth = ui->viewTable->columnWidth(i);
        }
        colStretch.append(columnwidth);
    }

    // Do the actual printing
    if(!tablePrinter.printTable(ui->viewTable->model(), colStretch)) {
        qCritical() << tr("Error in printing the table:") << tablePrinter.lastError();
    }
    painter.end();
    delete printB;
}

/*!
 * \brief The index in the view combobox changed
 *
 * This function makes sure that the GUI is entirely reset after a change happened.
 * This applies to the search conditions, any buttons, menus and of course the table view.
 *
 * It also saves the current layout of the columns of the previous table
 */
void MainWindow::on_viewComboBox_currentIndexChanged(int index)
{

    saveSizeViewColumns(ui->viewComboBox->itemData(lastTableIndex).toString());
    lastTableIndex = index;
    readonlyId.clear();
    resetViewAndSearch(true);
    visibilityReadonly();
    enableModify();
    enablePrint();
}

/*!
 * \brief The index in the view combobox changed
 *
 * This function adapts the table view when the read only combobox changes
 */
void MainWindow::on_readonlyComboBox_currentIndexChanged(int index)
{

    if(index > -1) {
        QModelIndex qidx = readonlyProxy->mapToSource(ui->readonlyComboBox->model()->index(index,0));
        readonlyId = readonlyProxy->sourceModel()->data(qidx).toString();
        adjustModel(getCurrentView());
        enablePrint();
    }
}

/*!
 * \fn MainWindow::on_actionModuleAll_triggered()
 * \fn MainWindow::on_actionCourseAll_triggered()
 * \fn MainWindow::on_actionModuleCourse_triggered()
 * \fn MainWindow::on_actionCourseModule_triggered()
 * \fn MainWindow::on_actionTransfers_triggered()
 *
 * \brief Signals for the menu entries
 *
 * These functions change the index of the view combobox
 */
void MainWindow::on_actionModuleAll_triggered()
{
    ui->viewComboBox->setCurrentIndex(1);
}

void MainWindow::on_actionCourseAll_triggered()
{
    ui->viewComboBox->setCurrentIndex(0);
}

void MainWindow::on_actionModuleCourse_triggered()
{
    ui->viewComboBox->setCurrentIndex(2);
}

void MainWindow::on_actionCourseModule_triggered()
{
    ui->viewComboBox->setCurrentIndex(3);
}

void MainWindow::on_actionTransfers_triggered()
{
    ui->viewComboBox->setCurrentIndex(4);
}

/*!
 * \brief About menu entry
 *
 * Displays the about dialog
 */
void MainWindow::on_actionAbout_triggered()
{
    AboutDialog *about = new AboutDialog(this);
    about->exec();
}

/*!
 * \brief Export to SQLite menu entry
 *
 * Exports the database to a SQLite file at the user specified location.
 * Mainly performs a copy of the current DB file.
 *
 * \warning The export overwrites files of the same name without a warning to the user.
 */
void MainWindow::on_actionExportSqlite_triggered()
{
    // Dialog to get the destination file name
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Database Export"), QDir::homePath(),
            filefiltersSqlite.join(";;"), &filefiltersSqlite.first());
    // Do nothing if destination file name is empty
    if(fileName.isEmpty()) return;

    // Give it a proper file ending, if user hasn't specified
    int lastsep = qMax(fileName.lastIndexOf("/"), fileName.lastIndexOf("\\"));
    int lastpoint = fileName.lastIndexOf(".");
    if(lastpoint <= lastsep) {
        fileName.append(".sqlite");
    }

    // Check if file is actually writable, if not do nothing and notify user
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Target file is not writable."),file.errorString());
        return;
    }
    file.close();

    // Overwrite any file of same name there!
    if(file.exists()) {file.remove();}

    // Copy the current database file to the destination
    QFile::copy(db.getDBFilePath(),fileName);
    QMessageBox::information(this, tr("Database Export"), tr("Database has been successfully exported to SQLite."));
}

/*!
 * \brief Export to single CSV menu entry
 *
 * Exports the transfers into a single CSV file (in German format)
 * It basically exports the 'Anerkennungen' view.
 *
 * \warning The export overwrites files of the same name without a warning to the user.
 */
void MainWindow::on_actionExportSinglecsv_triggered()
{
    // Dialog to get the destination file name
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Database Export"), QDir::homePath(),
            filefiltersCsv.join(";;"), &filefiltersCsv.first());
    // Do nothing if destination file name is empty
    if(fileName.isEmpty()) return;

    // Give it a proper file ending, if user hasn't specified
    int lastsep = qMax(fileName.lastIndexOf("/"), fileName.lastIndexOf("\\"));
    int lastpoint = fileName.lastIndexOf(".");
    if(lastpoint <= lastsep) {
        fileName.append(".csv");
    }

    QFile file(fileName);

    // write the view of 'Anerkennungen' to CSV
    CSVWriter writer(&db, this);

    QString mytable = "Module M JOIN Anerkennungen A ON M.ID = A.MID JOIN Kurse K ON K.ID = A.KID";
    QString selectcols = "K.Kursname AS 'Kurs-Name', K.ECTS AS 'Kurs-ECTS', K.Herkunft AS 'Kurs-Herkunft', M.Modulname AS 'Modul-Name', M.ECTS AS 'Modul-ECTS', M.PO AS 'Modul-PO'";

    writer.writeCSV(file, mytable, selectcols);

    QMessageBox::information(this, tr("Database Export"), tr("Database has been successfully exported to a single CSV file."));
}

/*!
 * \brief Export to CSV menu entry
 *
 * Exports each tables of the database into a single CSV file (in German format).
 * All the CSV files are then packed into a ZIP archive
 *
 * \warning The export overwrites files of the same name without a warning to the user.
 */
void MainWindow::on_actionExportCsv_triggered()
{
    QStringList filefiltersZip;
    filefiltersZip << tr("Zip-Archive (*.zip)") << tr("All files (*)");

    // Dialog to get the destination file name
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Database Export"), QDir::homePath(),
            filefiltersZip.join(";;"), &filefiltersZip.first());
    // Do nothing if destination file name is empty
    if(fileName.isEmpty()) return;

    // Give it a proper file ending, if user hasn't specified
    int lastsep = qMax(fileName.lastIndexOf("/"), fileName.lastIndexOf("\\"));
    int lastpoint = fileName.lastIndexOf(".");
    if(lastpoint <= lastsep) {
        fileName.append(".zip");
    }

    // Create a temporary directory. If not possible do nothing and return
    QTemporaryDir tmpdir;
    if(!tmpdir.isValid()) {
        qCritical() << tr("Unable to create temporary directory");
        return;
    }
    CSVWriter writer(&db, this);

    QString tmpdirpath = tmpdir.path();
    QStringList tables;
    tables << "Kurse" << "Module" << "Anerkennungen";
    QString table;

    // write each table to a csv file
    for(int i = 0; i< tables.size(); ++i) {
        table = tables.at(i);
        QFile tmpfile(tmpdirpath + "/" + table + ".csv");
        writer.writeCSV(tmpfile, table);
    }

    // pack the csv files into a zip archive
    JlCompress::compressDir(fileName, tmpdirpath);
    QMessageBox::information(this, tr("Database Export"), tr("Database has been successfully exported to a zip-archive of CSV files."));
}


/*!
 * \brief Import SQLite menu entry
 *
 * It imports a selected SQLite database.
 * If the import can happen, the following is done:
 * \list 1
 *   \li the current database is closed,
 *   \li the file then copied from the old location to the database path
 *   \li the new database is opened
 * \endlist
 */
void MainWindow::on_actionRestore_triggered()
{
    // Dialog to get the file name of the database to import
    QString fileName = QFileDialog::getOpenFileName(this, tr("Database Import"), QDir::homePath(),
                                                    filefiltersSqlite.join(";;"),&filefiltersSqlite.first());
    // Do nothing if destination file name is empty
    if(fileName.isEmpty()) return;

    // Check if the database can be copied from the import location
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly)){
        QMessageBox::information(this, tr("Selected file is not readable."),file.errorString());
        return;
    }
    file.close();

    // set the view combobox empty
    ui->viewComboBox->setCurrentIndex(-1);

    // Close the current database
    db.closeDatabase();
    // Copy the database to import over the current database
    QFile dbFile(db.getDBFilePath());
    dbFile.remove();
    QFile::copy(fileName, db.getDBFilePath());
    // Open the imported database
    db.openDatabase();
    QMessageBox::information(this, tr("Database Import"), tr("Database was successfully imported."));
}

/*!
 * \brief Print menu entry
 *
 * Displays a print preview dialog where the printed version of the current table view is shown
 */
void MainWindow::on_actionPrint_triggered()
{
    QPrintPreviewDialog dialog;
    dialog.connect(&dialog, SIGNAL(paintRequested(QPrinter*)), this, SLOT(print(QPrinter*)));
    dialog.exec();
}

/*!
 * \brief Options menu entry
 *
 * Displays the options dialog
 */
void MainWindow::on_actionOptions_triggered()
{
    ConfigManager::getInstance()->execConfigDialog(this);
    ui->viewTable->resizeRowsToContents();
}

/*!
 * \brief Overwrite of close event
 *
 * This function is overwriten to write the windowGeometry and the windowSize to the settings.
 *
 * \since 3.1
 */
void MainWindow::closeEvent(QCloseEvent *event)
{

    ConfigManager *cm = ConfigManager::getInstance();
    cm->writeSetting("windowgeometry", saveGeometry(), "applicationsize");
    cm->writeSetting("windowstate", saveState(), "applicationsize");

    cm->writeSetting("splittersizes", ui->mainsplitter->saveState(), "applicationsize");

    saveSizeViewColumns();
    QMainWindow::closeEvent(event);
}
