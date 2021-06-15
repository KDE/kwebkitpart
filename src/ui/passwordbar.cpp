/*
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "passwordbar.h"

#include "kwebkitpart_debug.h"
#include "settings/webkitsettings.h"

#include <KColorScheme>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QAction>
#include <QPalette>


PasswordBar::PasswordBar(QWidget *parent)
            :KMessageWidget(parent)
{
    setCloseButtonVisible(false);
    setMessageType(KMessageWidget::Information);

    QAction* action = new QAction(i18nc("@action:remember password", "&Remember"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onRememberButtonClicked()));
    addAction(action);

    action = new QAction(i18nc("@action:never for this site", "Ne&ver for this site"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onNeverButtonClicked()));
    addAction(action);

    action = new QAction(i18nc("@action:not now", "N&ot now"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onNotNowButtonClicked()));
    addAction(action);
}

PasswordBar::~PasswordBar()
{
}

QUrl PasswordBar::url() const
{
    return m_url;
}

QString PasswordBar::requestKey() const
{
    return m_requestKey;
}

void PasswordBar::setUrl (const QUrl& url)
{
    m_url = url;
}

void PasswordBar::setRequestKey (const QString& key)
{
    m_requestKey = key;
}

void PasswordBar::onNotNowButtonClicked()
{
    animatedHide();
    emit saveFormDataRejected (m_requestKey);
    emit done();
    clear();
}

void PasswordBar::onNeverButtonClicked()
{
    WebKitSettings::self()->addNonPasswordStorableSite(m_url.host());
    onNotNowButtonClicked();
}

void PasswordBar::onRememberButtonClicked()
{
    animatedHide();
    emit saveFormDataAccepted(m_requestKey);
    emit done();
    clear();
}

void PasswordBar::clear()
{
    m_requestKey.clear();
    m_url.clear();
}
