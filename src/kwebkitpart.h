/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef KWEBKITPART_H
#define KWEBKITPART_H

#include <QWebPage>

#include <KParts/ReadOnlyPart>

namespace KParts {
  class BrowserExtension;
  class StatusBarExtension;
}

class QWebView;
class QWebFrame;
class QWebHistoryItem;
class WebView;
class WebPage;
class SearchBar;
class PasswordBar;
class FeaturePermissionBar;
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
    explicit KWebKitPart(QWidget* parentWidget, QObject* parent,
                         const KPluginMetaData& metaData,
                         const QByteArray& cachedHistory = QByteArray(),
                         const QStringList& = QStringList());
    ~KWebKitPart() override;

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::openUrl
     */
    bool openUrl(const QUrl &) override;

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::closeUrl
     */
    bool closeUrl() override;

    /**
     * Returns a pointer to the render widget used to display a web page.
     *
     * @see QWebView.
     */
    QWebView *view() const;

    /**
     * Checks whether the page contains unsubmitted form changes.
     *
     * @return @p true if form changes exist.
     */
    bool isModified() const;

    /**
     * Returns if the page is currently in caret browsing mode.
     */
    bool isCaretMode() const;

    /**
     * Connects the appropriate signals from the given page to the slots
     * in this class.
     */
    void connectWebPageSignals(WebPage* page);

    void slotShowFeaturePermissionBar(QWebPage::Feature);
protected:
    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::guiActivateEvent
     */
    void guiActivateEvent(KParts::GUIActivateEvent *) override;

    /**
     * Re-implemented for internal reasons. API remains unaffected.
     *
     * @see KParts::ReadOnlyPart::openFile
     */
    bool openFile() override;

private Q_SLOTS:
    void slotShowSecurity();
    void slotShowSearchBar();
    void slotLoadStarted();
    void slotLoadAborted(const QUrl &);
    void slotLoadFinished(bool);
    void slotFrameLoadFinished(bool);
    void slotMainFrameLoadFinished(bool);

    void slotSearchForText(const QString &text, bool backward);
    void slotLinkHovered(const QString &, const QString&, const QString &);
    void slotSaveFrameState(QWebFrame *frame, QWebHistoryItem *item);
    void slotRestoreFrameState(QWebFrame *frame);
    void slotLinkMiddleOrCtrlClicked(const QUrl&);
    void slotSelectionClipboardUrlPasted(const QUrl&, const QString&);

    void slotUrlChanged(const QUrl &);
    void slotWalletClosed();
    void slotShowWalletMenu();
    void slotLaunchWalletManager();
    void slotDeleteNonPasswordStorableSite();
    void slotRemoveCachedPasswords();
    void slotSetTextEncoding(QTextCodec*);
    void slotSetStatusBarText(const QString& text);
    void slotWindowCloseRequested();
    void slotSaveFormDataRequested(const QString &, const QUrl &);
    void slotSaveFormDataDone();
    void slotFillFormRequestCompleted(bool);
    void slotFrameCreated(QWebFrame*);
    void slotToggleCaretMode();

    void slotFeaturePermissionGranted(QWebPage::Feature);
    void slotFeaturePermissionDenied(QWebPage::Feature);

private:
    WebPage* page();
    const WebPage* page() const;
    void initActions();
    void updateActions();
    void addWalletStatusBarIcon();

    bool m_emitOpenUrlNotify;
    bool m_hasCachedFormData;
    bool m_doLoadFinishedActions;
    KUrlLabel* m_statusBarWalletLabel;
    SearchBar* m_searchBar;
    PasswordBar* m_passwordBar;
    FeaturePermissionBar* m_featurePermissionBar;
    WebKitBrowserExtension* m_browserExtension;
    KParts::StatusBarExtension* m_statusBarExtension;
    WebView* m_webView;
};

#endif // WEBKITPART_H
