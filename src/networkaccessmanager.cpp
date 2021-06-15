/*
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "networkaccessmanager.h"
#include "settings/webkitsettings.h"

#include "kwebkitpart_debug.h"
#include <KLocalizedString>
#include <KProtocolInfo>
#include <KRun>

#include <QTimer>
#include <QWidget>
#include <QNetworkReply>
#include <QWebFrame>
#include <QWebElementCollection>
#include <QMimeType>
#include <QMimeDatabase>


#define QL1S(x) QLatin1String(x)
#define HIDABLE_ELEMENTS   QL1S("audio,img,embed,object,iframe,frame,video")

/* Null network reply */
class NullNetworkReply : public QNetworkReply
{
public:
    explicit NullNetworkReply(const QNetworkRequest &req, QObject* parent = nullptr)
        :QNetworkReply(parent)
    {
        setRequest(req);
        setUrl(req.url());
        setHeader(QNetworkRequest::ContentLengthHeader, 0);
        setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
        setError(QNetworkReply::ContentAccessDenied, i18n("Blocked by ad filter"));
        setAttribute(QNetworkRequest::User, QNetworkReply::ContentAccessDenied);
        QTimer::singleShot(0, this, SIGNAL(finished()));
    }

    void abort() override {}
    qint64 bytesAvailable() const override { return 0; }

protected:
    qint64 readData(char*, qint64) override {return -1;}
};

namespace KDEPrivate {

MyNetworkAccessManager::MyNetworkAccessManager(QObject *parent)
                       : KIO::AccessManager(parent)
{
}

static bool isMixedContent(const QUrl& baseUrl, const QUrl& reqUrl)
{
    const QString baseProtocol (baseUrl.scheme());
    const bool hasSecureScheme = (baseProtocol.compare(QL1S("https")) == 0 ||
                                  baseProtocol.compare(QL1S("webdavs")) == 0);
    return (hasSecureScheme && baseProtocol != reqUrl.scheme());
}

static bool blockRequest(QNetworkAccessManager::Operation op, const QUrl& requestUrl)
{
   if (op != QNetworkAccessManager::GetOperation)
       return false;

   if (!WebKitSettings::self()->isAdFilterEnabled())
       return false;

   if (!WebKitSettings::self()->isAdFiltered(requestUrl.toString()))
       return false;

   qCDebug(KWEBKITPART_LOG) << "*** REQUEST BLOCKED: URL" << requestUrl << "RULE" << WebKitSettings::self()->adFilteredBy(requestUrl.toString());
   return true;
}


QNetworkReply *MyNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    QWebFrame* frame = qobject_cast<QWebFrame*>(req.originatingObject());

    if (!blockRequest(op, req.url())) {
        if (KProtocolInfo::isHelperProtocol(req.url())) {
            (void) new KRun(req.url(), qobject_cast<QWidget*>(req.originatingObject()));
            return new NullNetworkReply(req, this);
        }
        QNetworkReply* reply = KIO::AccessManager::createRequest(op, req, outgoingData);
        if (frame && isMixedContent(frame->baseUrl(), req.url())) {
            connect(reply, SIGNAL(metaDataChanged()), this, SLOT(slotMetaDataChanged()));
        }
        return reply;
    }

    if (frame) {
        if (!m_blockedRequests.contains(frame))
            connect(frame, SIGNAL(loadFinished(bool)), this, SLOT(slotFinished(bool)));
        m_blockedRequests.insert(frame, req.url());
    }

    return new NullNetworkReply(req, this);
}

static void hideBlockedElements(const QUrl& url, QWebElementCollection& collection)
{
    for (QWebElementCollection::iterator it = collection.begin(); it != collection.end(); ++it) {
        const QUrl baseUrl ((*it).webFrame()->baseUrl());
        QString src = (*it).attribute(QL1S("src"));
        if (src.isEmpty())
            src = (*it).evaluateJavaScript(QL1S("this.src")).toString();
        if (src.isEmpty())
            continue;
        const QUrl resolvedUrl(baseUrl.resolved(QUrl(src)));
        if (url == resolvedUrl) {
            //qCDebug(KWEBKITPART_LOG) << "*** HIDING ELEMENT: " << (*it).tagName() << resolvedUrl;
            (*it).removeFromDocument();
        }
    }
}

void MyNetworkAccessManager::slotFinished(bool ok)
{
    if (!ok)
        return;

    if(!WebKitSettings::self()->isAdFilterEnabled())
        return;

    if(!WebKitSettings::self()->isHideAdsEnabled())
        return;

    QWebFrame* frame = qobject_cast<QWebFrame*>(sender());
    if (!frame)
        return;

    QList<QUrl> urls = m_blockedRequests.values(frame);
    if (urls.isEmpty())
        return;

   QWebElementCollection collection = frame->findAllElements(HIDABLE_ELEMENTS);
   if (frame->parentFrame())
        collection += frame->parentFrame()->findAllElements(HIDABLE_ELEMENTS);

    Q_FOREACH(const QUrl& url, urls)
        hideBlockedElements(url, collection);
}

static bool isActiveContent(const QString& contentType)
{
    QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForName(contentType);
    return (mime.isValid() && mime.inherits(QLatin1String("application/javascript")));
}

void MyNetworkAccessManager::slotMetaDataChanged()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*> (sender());
    if (reply) {
        const bool activeContent = isActiveContent(reply->header(QNetworkRequest::ContentTypeHeader).toString());
        if ((activeContent && !WebKitSettings::self()->alowActiveMixedContent()) ||
            (!activeContent && !WebKitSettings::self()->allowMixedContentDisplay())) {
            reply->abort();
            emit QMetaObject::invokeMethod(reply, "finished", Qt::AutoConnection);
        }
    }
}
}
