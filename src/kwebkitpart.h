/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#ifndef KWEBKITPART_H
#define KWEBKITPART_H

#include <KDE/KParts/ReadOnlyPart>

namespace KParts {
  class BrowserExtension;
  class StatusBarExtension;
}

namespace KDEPrivate {
  class PasswordBar;
  class SearchBar;
}

class QWebView;
class QWebFrame;
class QWebHistoryItem;
class WebView;
class WebPage;
class KUrlLabel;
class WebKitBrowserExtension;

/**
 * A KPart wrapper for the QtWebKit's browser rendering engine.
 *
 * This class attempts to provide the same type of integration into KPart
 * plugin applications, such as Konqueror, in much the same way as KHTML.
 *
 * Unlink the KHTML part however, access into the internals of the rendering
 * engine are provided through existing QtWebKit class ; @see QWebView.
 *
 */
class KWebKitPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
    Q_PROPERTY( bool modified READ isModified )
public:
    explicit KWebKitPart(QWidget *parentWidget = 0, QObject *parent = 0,
                         const QStringList &/*args*/ = QStringList());
    ~KWebKitPart();

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::openUrl
     */
    virtual bool openUrl(const KUrl &);

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::closeUrl
     */
    virtual bool closeUrl();

    /**
     * Returns a pointer to the render widget used to display a web page.
     *
     * @see QWebView.
     */
    virtual QWebView *view();

    /**
     * Checks whether the page contains unsubmitted form changes.
     *
     * @return @p true if form changes exist.
     */
    bool isModified() const;

    /**
     * Connects the appropriate signals from the given page to the slots
     * in this class.
     */
    void connectWebPageSignals(WebPage* page);

protected:
    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::guiActivateEvent
     */
    virtual void guiActivateEvent(KParts::GUIActivateEvent *);

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::openFile
     */
    virtual bool openFile();

private Q_SLOTS:
    void slotShowSecurity();
    void slotShowSearchBar();
    void slotLoadStarted();
    void slotLoadFinished(bool);
    void slotLoadAborted(const KUrl &);

    void slotSearchForText(const QString &text, bool backward);
    void slotLinkHovered(const QString &, const QString&, const QString &);
    void slotSaveFrameState(QWebFrame *frame, QWebHistoryItem *item);
    void slotRestoreFrameState(QWebFrame *frame);
    void slotLinkMiddleOrCtrlClicked(const KUrl&);
    void slotSelectionClipboardUrlPasted(const KUrl&, const QString&);

    void slotUrlChanged(const QUrl &);
    void slotWalletClosed();
    void slotShowWalletMenu();
    void slotLaunchWalletManager();
    void slotDeleteNonPasswordStorableSite();
    void slotRemoveCachedPasswords();
    void slotSetTextEncoding(QTextCodec*);
    void slotSetStatusBarText(const QString& text);
    void slotWindowCloseRequested();
    void slotPrintRequested(QWebFrame*);

private:
    WebPage* page();
    void initActions();

    bool m_emitOpenUrlNotify;
    bool m_pageRestored;
    bool m_hasCachedFormData;
    bool m_loadStarted;
    KUrlLabel* m_statusBarWalletLabel;
    KDEPrivate::SearchBar* m_searchBar;
    KDEPrivate::PasswordBar* m_passwordBar;
    WebKitBrowserExtension* m_browserExtension;
    KParts::StatusBarExtension* m_statusBarExtension;
    WebView* m_webView;
};

#endif // WEBKITPART_H
