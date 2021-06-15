/*
    SPDX-FileCopyrightText: 2007 Trolltech ASA
    SPDX-FileCopyrightText: 2008 Urs Wolfer <uwolfer@kde.org>
    SPDX-FileCopyrightText: 2008 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QPointer>
#include <KParts/BrowserExtension>
#include <QWebFrame>
#include <KWebView>

class QUrl;
class KWebKitPart;
class QWebInspector;
class QLabel;

class WebView : public KWebView
{
    Q_OBJECT
public:
    WebView(KWebKitPart* part, QWidget* parent);
    ~WebView() override;

    /**
     * Same as QWebPage::load, but with KParts style arguments instead.
     *
     * @see KParts::OpenUrlArguments, KParts::BrowserArguments.
     *
     * @param url     the url to load.
     * @param args    reference to a OpenUrlArguments object.
     * @param bargs   reference to a BrowserArguments object.
     */
    void loadUrl(const QUrl& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& bargs);

    QWebHitTestResult contextMenuResult() const;

protected:
    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::contextMenuEvent
     * @internal
     */
    void contextMenuEvent(QContextMenuEvent*) override;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::keyPressEvent
     * @internal
     */
    void keyPressEvent(QKeyEvent*) override;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::keyReleaseEvent
     * @internal
     */
    void keyReleaseEvent(QKeyEvent*) override;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::mouseReleaseEvent
     * @internal
     */
    void mouseReleaseEvent(QMouseEvent*) override;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QObject::timerEvent
     * @internal
     */
    void timerEvent(QTimerEvent*) override;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::wheelEvent
     * @internal
     */
    void wheelEvent(QWheelEvent*) override;

private Q_SLOTS:
    void slotStopAutoScroll();
    void hideAccessKeys();

private:
    void editableContentActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void selectActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void linkActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void partActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &);
    void multimediaActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void addSearchActions(QList<QAction*>& selectActions, QWebView*);

    void showAccessKeys();
    bool checkForAccessKey(QKeyEvent *event);
    void makeAccessKeyLabel(const QChar &accessKey, const QWebElement &element);

    KActionCollection* m_actionCollection;
    QWebHitTestResult m_result;
    QPointer<KWebKitPart> m_part;
    QWebInspector* m_webInspector;

    qint32 m_autoScrollTimerId;
    qint32 m_verticalAutoScrollSpeed;
    qint32 m_horizontalAutoScrollSpeed;

    enum AccessKeyState {
        NotActivated,
        PreActivated,
        Activated
    };
    AccessKeyState m_accessKeyActivated;
    QList<QLabel*> m_accessKeyLabels;
    QHash<QChar, QWebElement> m_accessKeyNodes;
    QHash<QString, QChar> m_duplicateLinkElements;
};

#endif // WEBVIEW_H
