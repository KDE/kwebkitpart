/*
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef NETWORKACCESSMANAGER_H
#define NETWORKACCESSMANAGER_H

#include <KIO/AccessManager>

#include <QMultiHash>

class QWebFrame;

namespace KDEPrivate {

 /**
  * Re-implemented for internal reasons. API remains unaffected.
  */
class MyNetworkAccessManager : public KIO::AccessManager
{
    Q_OBJECT

public:
    explicit MyNetworkAccessManager(QObject *parent = nullptr);

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = nullptr) override;

private Q_SLOTS:
    void slotFinished(bool);
    void slotMetaDataChanged();

private:
    QMultiHash<QWebFrame*, QUrl> m_blockedRequests;
};

}

#endif // NETWORKACCESSMANAGER_P_H
