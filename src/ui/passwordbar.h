/*
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PASSWORDBAR_H
#define PASSWORDBAR_H

#include <KMessageWidget>

#include <QUrl>


class PasswordBar : public KMessageWidget
{
    Q_OBJECT
public:
    explicit PasswordBar(QWidget *parent = nullptr);
    ~PasswordBar() override;

    QUrl url() const;
    QString requestKey() const;

    void setUrl(const QUrl&);
    void setRequestKey(const QString&);

Q_SIGNALS:
    void saveFormDataRejected(const QString &key);
    void saveFormDataAccepted(const QString &key);
    void done();

private Q_SLOTS:
    void onNotNowButtonClicked();
    void onNeverButtonClicked();
    void onRememberButtonClicked();

private:
    void clear();

    QUrl m_url;
    QString m_requestKey;
};

#endif // PASSWORDBAR_H
