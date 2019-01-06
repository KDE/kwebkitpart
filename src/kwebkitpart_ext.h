/*
 * This file is part of the KDE project.
 *
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

#ifndef WEBKITPART_EXT_H
#define WEBKITPART_EXT_H

#include <QPointer>

#include <KParts/BrowserExtension>
#include <KParts/TextExtension>
#include <KParts/HtmlExtension>
#include <kparts/htmlsettingsinterface.h>
#include <kparts/selectorinterface.h>
#include <kparts/scriptableextension.h>

class QUrl;
class KWebKitPart;
class WebView;
class KSaveFile;
class QWebFrame;

class WebKitBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT

public:
    WebKitBrowserExtension(KWebKitPart *parent, const QByteArray& cachedHistoryData);
    ~WebKitBrowserExtension() override;

    int xOffset() override;
    int yOffset() override;
    void saveState(QDataStream &) override;
    void restoreState(QDataStream &) override;
    void saveHistory();

Q_SIGNALS:
    void saveUrl(const QUrl &);
    void saveHistory(QObject*, const QByteArray&);

public Q_SLOTS:
    void cut();
    void copy();
    void paste();
    void print();

    void slotSaveDocument();
    void slotSaveFrame();
    void searchProvider();
    void reparseConfiguration();
    void disableScrolling();

    void zoomIn();
    void zoomOut();
    void zoomNormal();
    void toogleZoomTextOnly();
    void toogleZoomToDPI();
    void slotSelectAll();

    void slotFrameInWindow();
    void slotFrameInTab();
    void slotFrameInTop();
    void slotReloadFrame();
    void slotBlockIFrame();

    void slotSaveImageAs();
    void slotSendImage();
    void slotCopyImageURL();
    void slotCopyImage();
    void slotViewImage();
    void slotBlockImage();
    void slotBlockHost();

    void slotCopyLinkURL();
    void slotCopyLinkText();
    void slotSaveLinkAs();
    void slotCopyEmailAddress();

    void slotViewDocumentSource();
    void slotViewFrameSource();

    void updateEditActions();
    void updateActions();

    void slotPlayMedia();
    void slotMuteMedia();
    void slotLoopMedia();
    void slotShowMediaControls();
    void slotSaveMedia();
    void slotCopyMedia();
    void slotTextDirectionChanged();
    void slotCheckSpelling();
    void slotSpellCheckSelection();
    void slotSpellCheckDone(const QString&);
    void spellCheckerCorrected(const QString&, int, const QString&);
    void spellCheckerMisspelling(const QString&, int);
    void slotPrintRequested(QWebFrame*);
    void slotPrintPreview();

    void slotOpenSelection();
    void slotLinkInTop();

private:
    WebView* view();
    QPointer<KWebKitPart> m_part;
    QPointer<WebView> m_view;
    quint32 m_spellTextSelectionStart;
    quint32 m_spellTextSelectionEnd;
    QByteArray m_historyData;
};

/**
 * @internal
 * Implements the TextExtension interface
 */
class KWebKitTextExtension : public KParts::TextExtension
{
    Q_OBJECT
public:
    explicit KWebKitTextExtension(KWebKitPart* part);

    bool hasSelection() const override;
    QString selectedText(Format format) const override;
    QString completeText(Format format) const override;

private:
    KWebKitPart* part() const;
};

/**
 * @internal
 * Implements the HtmlExtension interface
 */
class KWebKitHtmlExtension : public KParts::HtmlExtension,
                             public KParts::SelectorInterface,
                             public KParts::HtmlSettingsInterface
{
    Q_OBJECT
    Q_INTERFACES(KParts::SelectorInterface)
    Q_INTERFACES(KParts::HtmlSettingsInterface)

public:
    explicit KWebKitHtmlExtension(KWebKitPart* part);

    // HtmlExtension
    QUrl baseUrl() const override;
    bool hasSelection() const override;

    // SelectorInterface
    QueryMethods supportedQueryMethods() const override;
    Element querySelector(const QString& query, KParts::SelectorInterface::QueryMethod method) const override;
    QList<Element> querySelectorAll(const QString& query, KParts::SelectorInterface::QueryMethod method) const override;

    // HtmlSettingsInterface
    QVariant htmlSettingsProperty(HtmlSettingsType type) const override;
    bool setHtmlSettingsProperty(HtmlSettingsType type, const QVariant& value) override;

private:
    KWebKitPart* part() const;
};

class KWebKitScriptableExtension : public KParts::ScriptableExtension
{
  Q_OBJECT

public:
    explicit KWebKitScriptableExtension(KWebKitPart* part);

    QVariant rootObject() override;

    QVariant get(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName) override;

    bool put(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName, const QVariant& value) override;

    bool setException(ScriptableExtension* callerPrincipal, const QString& message) override;

    QVariant evaluateScript(ScriptableExtension* callerPrincipal,
                                    quint64 contextObjectId,
                                    const QString& code,
                                    ScriptLanguage language = ECMAScript) override;

    bool isScriptLanguageSupported(ScriptLanguage lang) const override;

private:
     QVariant encloserForKid(KParts::ScriptableExtension* kid) override;
     KWebKitPart* part();
};

#endif // WEBKITPART_EXT_H
