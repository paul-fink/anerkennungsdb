/*
 * printlayout.cpp
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

#include "printlayout.h"

/*!
 * \class PrintLayout
 *
 * \brief Defines the basic page layout
 *
 * \since 3.0
 */
int PrintLayout::pageNumber = 0;

/*!
 * \overload
 *
 * \brief Prepares the page for printing
 *
 * The page number is printetd in the center of the footer and the pagename at the top left
 */
void PrintLayout::preparePage(QPainter *painter)
{
    painter->setPen(QPen(QColor(0, 0, 0), 1));
    painter->drawText(10, 10, pageName); //upper left
    painter->translate(10, painter->viewport().height() - 10); //center in footer
    painter->drawText(0, 0, QObject::tr("Page %1").arg(pageNumber));
    pageNumber += 1;
}

/*!
 * \brief Returns the page name
 */
QString PrintLayout::getPageName() const
{
    return pageName;
}

/*!
 * \brief Sets the page name to \a value
 */
void PrintLayout::setPageName(const QString &value)
{
    pageName = value;
}

