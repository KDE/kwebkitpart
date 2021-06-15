/*
    SPDX-FileCopyrightText: 2011 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBHISTORYINTERFACE_H
#define WEBHISTORYINTERFACE_H


#include <QWebHistoryInterface>


class WebHistoryInterface : public QWebHistoryInterface
{
public:
    explicit WebHistoryInterface(QObject* parent = nullptr);
    void addHistoryEntry (const QString & url) override;
    bool historyContains (const QString & url) const override;
};

#endif //WEBHISTORYINTERFACE_H
