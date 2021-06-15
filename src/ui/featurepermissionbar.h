/*
    SPDX-FileCopyrightText: 2013 Allan Sandfeld Jensen <sandfeld@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FEATUREPERMISSIONBAR_H
#define FEATUREPERMISSIONBAR_H

#include <KMessageWidget>

#include <QWebPage>


class FeaturePermissionBar : public KMessageWidget
{
    Q_OBJECT
public:
    explicit FeaturePermissionBar(QWidget *parent = nullptr);
    ~FeaturePermissionBar() override;

    QWebPage::Feature feature() const;

    void setFeature(QWebPage::Feature);

Q_SIGNALS:
    void permissionGranted(QWebPage::Feature);
    void permissionDenied(QWebPage::Feature);
    void done();

private Q_SLOTS:
    void onDeniedButtonClicked();
    void onGrantedButtonClicked();

private:
    QWebPage::Feature m_feature;
};

#endif // FEATUREPERMISSIONBAR_H
