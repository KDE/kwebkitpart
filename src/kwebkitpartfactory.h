/*
    SPDX-FileCopyrightText: 2008 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBKITPARTFACTORY
#define WEBKITPARTFACTORY

#include <KPluginFactory>

#include <QHash>

class QWidget;

class KWebKitFactory : public KPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.KPluginFactory" FILE "kwebkitpart.json")
    Q_INTERFACES(KPluginFactory)
public:
    ~KWebKitFactory() override;
    QObject *create(const char* iface, QWidget *parentWidget, QObject *parent, const QVariantList& args, const QString &keyword) override;

private Q_SLOTS:
    void slotDestroyed(QObject* object);
    void slotSaveHistory(QObject* widget, const QByteArray&);

private:
    QHash<QObject*, QByteArray> m_historyBufContainer;
};

#endif // WEBKITPARTFACTORY
