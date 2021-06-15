/*
    SPDX-FileCopyrightText: 2012 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBPLUGINFACTORY_H
#define WEBPLUGINFACTORY_H

#include <KWebPluginFactory>

#include <QWidget>
#include <QPointer>

class KWebKitPart;
class QPoint;

class FakePluginWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool swapping READ swapping)

public:
    FakePluginWidget(uint id, const QUrl& url, const QString& mimeType, QWidget* parent = nullptr);
    bool swapping() const { return m_swapping; }

Q_SIGNALS:
    void pluginLoaded(uint);

private Q_SLOTS:
    void loadAll();
    void load(bool loadAll = false);
    void showContextMenu(const QPoint&);
    void updateScrollPoisition(int, int, const QRect&);

private:
    bool m_swapping;
    bool m_updateScrollPosition;
    QString m_mimeType;
    uint m_id;
};

class WebPluginFactory : public KWebPluginFactory
{
    Q_OBJECT
public:
    explicit WebPluginFactory (KWebKitPart* part, QObject* parent = nullptr);
    QObject* create (const QString&, const QUrl&, const QStringList&, const QStringList&) const override;
    void resetPluginOnDemandList();

private Q_SLOTS:
    void loadedPlugin(uint);

private:
    QPointer<KWebKitPart> mPart;
    mutable QList<uint> mPluginsLoadedOnDemand;
};

#endif
