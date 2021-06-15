/*
    SPDX-FileCopyrightText: 2008 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "kwebkitpartfactory.h"
#include "kwebkitpart_debug.h"
#include "kwebkitpart_ext.h"
#include "kwebkitpart.h"

#include <KPluginMetaData>

KWebKitFactory::~KWebKitFactory()
{
    // qCDebug(KWEBKITPART_LOG) << this;
}

QObject *KWebKitFactory::create(const char* iface, QWidget *parentWidget, QObject *parent, const QVariantList &args, const QString& keyword)
{
    Q_UNUSED(iface);
    Q_UNUSED(keyword);
    Q_UNUSED(args);

    qCDebug(KWEBKITPART_LOG) << parentWidget << parent;
    connect(parentWidget, SIGNAL(destroyed(QObject*)), this, SLOT(slotDestroyed(QObject*)));

    // NOTE: The code below is what makes it possible to properly integrate QtWebKit's
    // history management with any KParts based application.
    QByteArray histData (m_historyBufContainer.value(parentWidget));
    if (!histData.isEmpty()) histData = qUncompress(histData);
    KWebKitPart* part = new KWebKitPart(parentWidget, parent, metaData(), histData);
    WebKitBrowserExtension* ext = qobject_cast<WebKitBrowserExtension*>(part->browserExtension());
    if (ext) {
        connect(ext, SIGNAL(saveHistory(QObject*,QByteArray)), this, SLOT(slotSaveHistory(QObject*,QByteArray)));
    }
    return part;
}

void KWebKitFactory::slotSaveHistory(QObject* widget, const QByteArray& buffer)
{
    // qCDebug(KWEBKITPART_LOG) << "Caching history data from" << widget;
    m_historyBufContainer.insert(widget, buffer);
}

void KWebKitFactory::slotDestroyed(QObject* object)
{
    // qCDebug(KWEBKITPART_LOG) << "Removing cached history data of" << object;
    m_historyBufContainer.remove(object);
}
