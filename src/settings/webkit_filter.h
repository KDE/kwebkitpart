/*
    SPDX-FileCopyrightText: 2005 Ivor Hewitt <ivor@kde.org>
    SPDX-FileCopyrightText: 2008 Maksim Orlovich <maksim@kde.org>
    SPDX-FileCopyrightText: 2008 Vyacheslav Tokarev <tsjoker@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef WEBKIT_FILTER_P_H
#define WEBKIT_FILTER_P_H

#include <QString>
#include <QRegExp>
#include <QVector>
#include <kwebkitpart.h>

class StringsMatcher;

namespace KDEPrivate
{
// This represents a set of filters that may match URLs.
// Currently it supports a subset of AddBlock Plus functionality.
class FilterSet {
public:
    FilterSet();
    ~FilterSet();

    // Parses and registers a filter. This will also strip @@ for exclusion rules, skip comments, etc.
    // The user does have to split black and white lists into separate sets, however
    void addFilter(const QString& filter);

    bool isUrlMatched(const QString& url);
    QString urlMatchedBy(const QString& url);

    void clear();

private:
    QVector<QRegExp> reFilters;
    StringsMatcher* stringFiltersMatcher;
};

}

#endif // WEBKIT_FILTER_P_H

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
