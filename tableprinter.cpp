/****************************************************************************
**
** Copyright (c) 2016, Anton Onishchenko
**               2018, Paul Fink (modifications)
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without modification,
** are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
** list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice, this
** list of conditions and the following disclaimer in the documentation and/or other
** materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its contributors may
** be used to endorse or promote products derived from this software without
** specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
**  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
** ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#include "tableprinter.h"

/*!
 * \class TablePrinter
 *
 * \brief Class to print the contents of a QAbstractItemModel
 *
 * \since 3.0
 */

/*!
 * \brief Constructs the TablePrinter instance
 *
 * It sets the pointers to the supplied QPrinter \c printer and QPainter \c painter objects.
 * It sets all its members to default values:
 *
 * \table
 * \header
 *   \li Member
 *   \li Description
 *   \li Default
 * \row
 *   \li \c topMargin, \c bottomMargin, \c leftMargin, \c rightMargin
 *   \li Margins of each cell
 *   \li 5
 * \row
 *   \li \c headerHeight, \c bottomHeight
 *   \li Height of header / footer
 *   \li 0
 * \row
 *   \li \c leftBlank, \c rightBlank
 *   \li left / right page margin
 *   \li 0
 * \row
 *   \li \c maxRowHeight
 *   \li maximal height of a single row
 *   \li 1000
 * \row
 *   \li \c headerPrint
 *   \li Print header
 *   \li true
 * \row
 *   \li \c headerRepeat
 *   \li Repeat header as first line if table is split over multiple pages (is ignored when \c{headerPrint == false}
 *   \li true
 * \row
 *   \li \c pen
 *   \li Pen to use for the lines
 *   \li pen stored in \c painter (\l QPainter::pen())
 * \row
 *   \li \c headersFont, \c contentFont
 *   \li Font for table header / contet
 *   \li font stored in \c painter(\l QPainter::font())
 * \row
 *   \li \c headerColor, \c contentColor
 *   \li Fnt colour for the header / content
 *   \li colour of the the pen stored in painter (\l QPainter::pen(), \l QPen::color())
 * \row
 *   \li \c prepare
 *   \li \c PagePrepare object
 *   \li NULL
 * \row
 *   \li \c error
 *   \li Error message
 *   \li No error (translatable)
 * \endtable
 */
TablePrinter::TablePrinter(QPainter* painter, QPrinter* printer) :
    painter(painter),
    printer(printer) {
    topMargin = 5;
    bottomMargin = 5;
    leftMargin = 5;
    rightMargin = 5;
    headerHeight = 0;
    bottomHeight = 0;
    leftBlank = 0;
    rightBlank = 0;
    maxRowHeight = 1000;
    headerPrint = true;
    headerRepeat = true;
    pen = painter->pen();
    headersFont = painter->font();
    contentFont = painter->font();
    headerColor = painter->pen().color();
    contentColor = painter->pen().color();
    prepare = NULL;
    error = QObject::tr("No error");
}

/*!
 * \fn TablePrinter::printTable(const QAbstractItemModel* model, const QVector<int> columnStretch, const QVector<QString> headers = QVector<QString>()
 * \brief Print the contents of the model
 *
 * This is the workhose function which prints all contents in \a model to paper.
 *
 * By \a columnStretch the strech for each column in the model is supplied.
 * Setting a stretch to 0 for a column omits it from printing.
 * The length of \a columnStretch must correspond to the number of columns in \a model.
 *
 * Optionally, a vector with values headers can be supplied in \a headers.
 * The length of \a headers must correspond to the number of columns in \a model.
 * If not supplied the headers are taken from \c model.
 *
 * This function returns \c true if no error occured. Otherwise the internal error message is updated and \c false is returned.
 */
bool TablePrinter::printTable(const QAbstractItemModel* model, const QVector<int> columnStretch,
                              const QVector<QString> headers) {

    //--------------------------------- error checking -------------------------------------

    int columnCount = model->columnCount();
    int count = columnStretch.count();
    if(count != columnCount) {
        error = QObject::tr("Different columns count in model and in columnStretch");
        return false;
    }
    count = headers.count();
    if(count != columnCount && count != 0) {
        error = QObject::tr("Different columns count in model and in headers");
        return false;
    }
    if(!printer->isValid()) {
        error = QObject::tr("Printer is not valid");
        return false;
    }
    if(!painter->isActive()) {
        error = QObject::tr("Painter is not active");
        return false;
    }
    double tableWidth = painter->viewport().width() - leftBlank - rightBlank;
    double tableHeight = painter->viewport().height() - headerHeight - bottomHeight;
    if(tableWidth <= 0 || tableHeight <= 0) {
        error = QObject::tr("Margins larger than print area");
        return false;
    }
    int totalStretch = 0;
    for (int i = 0; i < columnStretch.count(); i++) {
        if(columnStretch[i] < 0) {
            error = QObject::tr("Negative column stretch for column: %1 (%2)").arg(i).arg(columnStretch[i]);
            return false;
        }
        totalStretch += columnStretch[i];
    }
    if(totalStretch <= 0) {
        error = QObject::tr("No column to print");
        return false;
    }
    QVector<double> columnWidth;
    for (int i = 0; i < columnStretch.count(); i++) {
        columnWidth.append(tableWidth / totalStretch * columnStretch[i]);
    }

    //----------------------------------------------------------------------------

    painter->save(); // before table print

    // to know row height before printing
    // at first print to test image
    QPainter testSize;
    QImage* image = new QImage(10, 10, QImage::Format_RGB32);
    image->setDotsPerMeterX(printer->logicalDpiX() * 100 / 2.54); // 2.54 cm = 1 inch
    image->setDotsPerMeterY(printer->logicalDpiY() * 100 / 2.54);
    testSize.begin(image);

    if(prepare) {
        painter->save();
        painter->translate(-painter->transform().dx(), -painter->transform().dy());
        prepare->preparePage(painter);
        painter->restore();
    }

    painter->setPen(pen);
    painter->setFont(contentFont);
    testSize.setFont(contentFont);
    painter->translate(-painter->transform().dx() + leftBlank, -painter->transform().dy() + headerHeight);
    painter->save();

    painter->drawLine(0, 0, tableWidth, 0); // first horizontal line

    float max_y = 0;

    int maxHeaderheight = 0; // max row height for header
    // Vector of all (default) column headers
    QVector<QString> printheader;

    if(headerPrint) {// Only do something if headers are desired

        painter->setFont(headersFont);
        testSize.setFont(headersFont);


        if(headers.isEmpty()) {
            for(int i = 0; i < columnCount; i++) { // for each column
                printheader.append(model->headerData(i,Qt::Horizontal).toString());
            }
        } else {
            printheader = headers;
        }

        // First all calculations for the header


        for(int i = 0; i < columnCount; i++) { // for each column
            QString str = printheader.at(i);
            QRect rect(0, 0, columnWidth[i] - rightMargin - leftMargin, maxRowHeight);
            QRect realRect;
            testSize.drawText(rect, Qt::AlignLeft | Qt::TextWordWrap, str, &realRect);
            if (realRect.height() > maxHeaderheight && columnStretch[i] != 0) {
                realRect.height() > maxRowHeight ? maxHeaderheight = maxRowHeight : maxHeaderheight = realRect.height();
            }
        }
        painter->save();

        // print out the header
        painter->setPen(QPen(headerColor));

        for(int i = 0; i < columnCount; i++) { // for each column
            QString str = printheader.at(i);
            QRect rec(leftMargin, topMargin, columnWidth[i] - rightMargin - leftMargin, maxHeaderheight);
            painter->drawText(rec, Qt::AlignLeft | Qt::TextWordWrap, str);
            painter->translate(columnWidth[i], 0);
        }
        painter->restore();

        painter->drawLine(0, maxHeaderheight + topMargin + bottomMargin, tableWidth,
                      maxHeaderheight + topMargin + bottomMargin); // last horizontal line
        painter->translate(0, maxHeaderheight + topMargin + bottomMargin);
        max_y = painter->transform().dy();
    }

    // now iterate over the rows
    for(int j = 0; j < model->rowCount(); j++) { // for each row

        painter->setFont(contentFont);
        testSize.setFont(contentFont);

        // --------------------------- row height counting ----------------------------

        int maxHeight = 0; // max row Height
        for(int i = 0; i < columnCount; i++) { // for each column
            QString str = model->data(model->index(j,i), Qt::DisplayRole).toString();
            QRect rect(0, 0, columnWidth[i] - rightMargin - leftMargin, maxRowHeight);
            QRect realRect;
            testSize.drawText(rect, Qt::AlignLeft | Qt::TextWordWrap, str, &realRect);
            if (realRect.height() > maxHeight && columnStretch[i] != 0) {
                 realRect.height() > maxRowHeight ? maxHeight = maxRowHeight : maxHeight = realRect.height();
            }
        }

        if(painter->transform().dy() + maxHeight + topMargin + bottomMargin > painter->viewport().height() - bottomHeight) {
            // row to add has to large row height => Finish this page and start a new one
            int y = painter->transform().dy();
            painter->restore();
            painter->save();
            for(int i = 0; i < columnCount; i++) { // vertical lines
                painter->drawLine(0, 0, 0,
                                  - painter->transform().dy() + y);
                painter->translate(columnWidth[i], 0);
            }
            painter->drawLine(0, 0, 0, - painter->transform().dy() + y); // last vertical line
            painter->restore();
            printer->newPage();
            if(prepare) {
                painter->save();
                painter->translate(-painter->transform().dx(), -painter->transform().dy());
                prepare->preparePage(painter);
                painter->restore();
            }
            painter->translate(-painter->transform().dx() + leftBlank, -painter->transform().dy() + headerHeight);
            painter->save();
            painter->drawLine(0, 0, tableWidth, 0); // first horizontal line

            // Print the header at first (if option is set and printing headers is enabled at all)
            if(headerPrint && headerRepeat) {
                painter->setFont(headersFont);
                testSize.setFont(headersFont);

                painter->save();
                painter->setPen(QPen(headerColor));

                for(int i = 0; i < columnCount; i++) { // for each column
                    QString str = printheader.at(i);
                    QRect rec(leftMargin, topMargin, columnWidth[i] - rightMargin - leftMargin, maxHeaderheight);
                    painter->drawText(rec, Qt::AlignLeft | Qt::TextWordWrap, str);
                    painter->translate(columnWidth[i], 0);
                }
                painter->restore();

                painter->drawLine(0, maxHeaderheight + topMargin + bottomMargin, tableWidth,
                                  maxHeaderheight + topMargin + bottomMargin); // last horizontal line
                painter->translate(0, maxHeaderheight + topMargin + bottomMargin);
                max_y = painter->transform().dy();

                painter->setFont(contentFont);
                testSize.setFont(contentFont);
            }
        }

        //------------------------------ content printing -------------------------------------------

        painter->save();
        painter->setPen(QPen(contentColor));
        for(int i = 0; i < columnCount; i++) { // for each column
            QString str = model->data(model->index(j,i), Qt::DisplayRole).toString();
            QRect rec(leftMargin, topMargin, columnWidth[i] - rightMargin - leftMargin, maxHeight);
            painter->drawText(rec, Qt::AlignLeft | Qt::TextWordWrap, str);
            painter->translate(columnWidth[i], 0);
        }
        painter->restore();

        painter->drawLine(0, maxHeight + topMargin + bottomMargin, tableWidth,
                          maxHeight + topMargin + bottomMargin); // last horizontal line
        painter->translate(0, maxHeight + topMargin + bottomMargin);
        max_y = painter->transform().dy();
    }
    int y = painter->transform().dy();
    painter->restore();
    painter->save();
    for(int i = 0; i < columnCount; i++) { // vertical lines
        painter->drawLine(0, 0, 0,
                          - painter->transform().dy() + y);
        painter->translate(columnWidth[i], 0);
    }
    painter->drawLine(0, 0, 0, - painter->transform().dy() + y); // last vertical line
    painter->restore();

    testSize.end();
    delete image;

    painter->restore(); // before table print

    painter->translate(0, max_y);

    return true;
}

/*!
 * \brief Returns the last error
 */
QString TablePrinter::lastError() {
    return error;
}

/*!
 * \brief Sets the cell margins
 * \param left
 * \param right
 * \param top
 * \param bottom
 */
void TablePrinter::setCellMargin(int left, int right, int top, int bottom) {
    topMargin = top;
    bottomMargin = bottom;
    leftMargin = left;
    rightMargin = right;
}

/*!
 * \brief Sets the page margins
 * \param left
 * \param right
 * \param top
 * \param bottom
 */
void TablePrinter::setPageMargin(int left, int right, int top, int bottom) {
    headerHeight = top;
    bottomHeight = bottom;
    leftBlank = left;
    rightBlank = right;
}

/*!
 * \brief Sets the PagePrepare object
 * \param prep
 */
void TablePrinter::setPagePrepare(PagePrepare *prep) {
    prepare = prep;
}

void TablePrinter::setPen(QPen p) {
    pen = p;
}

/*!
 * \fn TablePrinter::setHeadersFont(QFont f)
 * \fn TablePrinter::setContentFont(QFont f)
 *
 * \brief Font settings
 *
 * These functions set the header / content font to \a f.
 */
void TablePrinter::setHeadersFont(QFont f) {
    headersFont = f;
}

void TablePrinter::setContentFont(QFont f) {
    contentFont = f;
}

/*!
 * \fn TablePrinter::setHeaderColor(QColor color)
 * \fn TablePrinter::setContentColor(QColor color)
 *
 * \brief Colour settings
 *
 * These functions set the header / content colour to \a color.
 */
void TablePrinter::setHeaderColor(QColor color) {
    headerColor = color;
}

void TablePrinter::setContentColor(QColor color) {
    contentColor = color;
}

/*!
 * \brief Set row height
 *
 * This function sets the maximal row height to \a height.
 */
void TablePrinter::setMaxRowHeight(int height) {
    maxRowHeight = height;
}

/*!
 * \brief Set header to be repeated on new page
 *
 * This function set the option if the header should be repeated
 * in case the table is split over multiple pages to \a repeatheader.
 */
void TablePrinter::setHeaderRepeat(bool repeatheader) {
    headerRepeat = repeatheader;
}

/*!
 * \brief Set header to be printed
 *
 * This function set the option if the header should be printed at all.
 */
void TablePrinter::setHeaderPrint(bool printheader) {
    headerPrint = printheader;
}
