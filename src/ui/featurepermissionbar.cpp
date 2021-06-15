/*
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>
    SPDX-FileCopyrightText: 2013 Allan Sandfeld Jensen <sandfeld@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "featurepermissionbar.h"

#include <KLocalizedString>

#include <QAction>


FeaturePermissionBar::FeaturePermissionBar(QWidget *parent)
                     :KMessageWidget(parent)
{
    setCloseButtonVisible(false);
    setMessageType(KMessageWidget::Information);

    QAction* action = new QAction(i18nc("@action:deny access", "&Deny access"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onDeniedButtonClicked()));
    addAction(action);

    action = new QAction(i18nc("@action:grant access", "&Grant access"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(onGrantedButtonClicked()));
    addAction(action);

    // FIXME: Add option to allow and remember for this site.
}

FeaturePermissionBar::~FeaturePermissionBar()
{
}

QWebPage::Feature FeaturePermissionBar::feature() const
{
    return m_feature;
}

void FeaturePermissionBar::setFeature (QWebPage::Feature feature)
{
    m_feature = feature;
}

void FeaturePermissionBar::onDeniedButtonClicked()
{
    animatedHide();
    emit permissionDenied(m_feature);
    emit done();
}

void FeaturePermissionBar::onGrantedButtonClicked()
{
    animatedHide();
    emit permissionGranted(m_feature);
    emit done();
}
