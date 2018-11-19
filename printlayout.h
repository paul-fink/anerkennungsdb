/*
 * printlayout.h
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

#ifndef PRINTLAYOUT_H
#define PRINTLAYOUT_H

#include "tableprinter.h"

class PrintLayout : public PagePrepare {
public:
    virtual void preparePage(QPainter *painter);
    static int pageNumber;

    QString getPageName() const;
    void setPageName(const QString &value);

private:
    QString pageName;
};

#endif // PRINTLAYOUT_H
