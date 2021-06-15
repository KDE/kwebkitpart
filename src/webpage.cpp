/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 - 2010 Urs Wolfer <uwolfer @ kde.org>
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

#include "webpage.h"

#include "kwebkitpart_debug.h"
#include "kwebkitpart.h"
#include "webview.h"
#include "networkaccessmanager.h"
#include "settings/webkitsettings.h"
#include "webpluginfactory.h"

#include <KDialogJobUiDelegate>
#include <KIO/CommandLauncherJob>
#include <KIconLoader>
#include <KMessageBox>
#include <KShell>
#include <KProtocolInfo>
#include <KStringHandler>
#include <KUrlAuthorized>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <KIO/Job>
#include <KParts/HtmlExtension>

#include <QApplication>
#include <QNetworkReply>
#include <QWebSecurityOrigin>
#include <QDesktopWidget>
#include <QMimeType>
#include <QMimeDatabase>
#include <QLocale>
#include <QFileDialog>
#include <QUrlQuery>

#define QL1S(x)  QLatin1String(x)
#define QL1C(x)  QLatin1Char(x)

static bool isBlankUrl(const QUrl& url)
{
    return (url.isEmpty() || url.url() == QL1S("about:blank"));
}

WebPage::WebPage(KWebKitPart *part, QWidget *parent)
        :KWebPage(parent, (KWebPage::KPartsIntegration|KWebPage::KWalletIntegration)),
         m_kioErrorCode(0),
         m_ignoreError(false),
         m_noJSOpenWindowCheck(false),
         m_part(part)
{
    // FIXME: Need a better way to handle request filtering than to inherit
    // KIO::Integration::AccessManager...
    KDEPrivate::MyNetworkAccessManager *manager = new KDEPrivate::MyNetworkAccessManager(this);
    manager->setEmitReadyReadOnMetaDataChange(true);
    manager->setCache(nullptr);
    QWidget* window = parent ? parent->window() : nullptr;
    if (window) {
        manager->setWindow(window);
    }
    setNetworkAccessManager(manager);

    setPluginFactory(new WebPluginFactory(part, this));

    setSessionMetaData(QL1S("ssl_activate_warnings"), QL1S("TRUE"));

    // Set font sizes accordingly...
    if (view())
        WebKitSettings::self()->computeFontSizes(view()->logicalDpiY());

    setForwardUnsupportedContent(true);

    // Add all KDE's local protocols to QWebSecurityOrigin
    Q_FOREACH (const QString& protocol, KProtocolInfo::protocols()) {
        // file is already a known local scheme and about must not be added
        // to this list since there is about:blank.
        if (protocol == QL1S("about") || protocol == QL1S("file"))
            continue;

        if (KProtocolInfo::protocolClass(protocol) != QL1S(":local"))
            continue;

        QWebSecurityOrigin::addLocalScheme(protocol);
    }

    connect(this, SIGNAL(geometryChangeRequested(QRect)),
            this, SLOT(slotGeometryChangeRequested(QRect)));
    connect(this, SIGNAL(downloadRequested(QNetworkRequest)),
            this, SLOT(downloadRequest(QNetworkRequest)));
    connect(this, SIGNAL(unsupportedContent(QNetworkReply*)),
            this, SLOT(slotUnsupportedContent(QNetworkReply*)));
    connect(this, SIGNAL(featurePermissionRequested(QWebFrame*,QWebPage::Feature)),
            this, SLOT(slotFeaturePermissionRequested(QWebFrame*,QWebPage::Feature)));
    connect(networkAccessManager(), SIGNAL(finished(QNetworkReply*)),
            this, SLOT(slotRequestFinished(QNetworkReply*)));
}

WebPage::~WebPage()
{
    //qCDebug(KWEBKITPART_LOG) << this;
}

const WebSslInfo& WebPage::sslInfo() const
{
    return m_sslInfo;
}

void WebPage::setSslInfo (const WebSslInfo& info)
{
    m_sslInfo = info;
}

static void checkForDownloadManager(QWidget* widget, QString& cmd)
{
    cmd.clear();
    KConfigGroup cfg (KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals), "HTML Settings");
    const QString fileName (cfg.readPathEntry("DownloadManager", QString()));
    if (fileName.isEmpty())
        return;

    const QString exeName = QStandardPaths::findExecutable(fileName);
    if (exeName.isEmpty()) {
        KMessageBox::detailedSorry(widget,
                                   i18n("The download manager (%1) could not be found in your installation.", fileName),
                                   i18n("Try to reinstall it and make sure that it is available in $PATH. \n\nThe integration will be disabled."));
        cfg.writePathEntry("DownloadManager", QString());
        cfg.sync();
        return;
    }

    cmd = exeName;
}

void WebPage::downloadRequest(const QNetworkRequest &request)
{
    const QUrl url(request.url());

    // Integration with a download manager...
    if (!url.isLocalFile()) {
        QString managerExe;
        checkForDownloadManager(view(), managerExe);
        if (!managerExe.isEmpty()) {
            //qCDebug(KWEBKITPART_LOG) << "Calling command" << cmd;
            KIO::CommandLauncherJob *job = new KIO::CommandLauncherJob(managerExe, {url.toString()});
            job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, view()));
            job->start();
            return;
        }
    }

    KWebPage::downloadRequest(request);
}

static QString warningIconData()
{
    QString data;
    QFile f (KIconLoader::global()->iconPath("dialog-warning", -KIconLoader::SizeHuge));

    if (f.open(QIODevice::ReadOnly)) {
        QByteArray fileData = f.readAll();
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFileNameAndData(f.fileName(), fileData);
        data += QL1S("data:");
        data += mime.name();
        data += QL1S(";base64,");
        data += fileData.toBase64();
        f.close();
    }

    return data;
}

QString WebPage::errorPage(int code, const QString& text, const QUrl& reqUrl) const
{
    QString errorName, techName, description;
    QStringList causes, solutions;

    QByteArray raw = KIO::rawErrorDetail( code, text, &reqUrl );
    QDataStream stream(raw);

    stream >> errorName >> techName >> description >> causes >> solutions;

    QFile file (QStandardPaths::locate(QStandardPaths::GenericDataLocation, QL1S("kwebkitpart/error.html")));
    if ( !file.open( QIODevice::ReadOnly ) )
        return i18n("<html><body><h3>Unable to display error message</h3>"
                    "<p>The error template file <em>error.html</em> could not be "
                    "found.</p></body></html>");

    QString html( QL1S(file.readAll()) );

    html.replace( QL1S( "TITLE" ), i18n( "Error: %1", errorName ) );
    html.replace( QL1S( "DIRECTION" ), QApplication::isRightToLeft() ? "rtl" : "ltr" );
    html.replace( QL1S( "ICON_PATH" ), warningIconData());

    QString doc (QL1S( "<h1>" ));
    doc += i18n( "The requested operation could not be completed" );
    doc += QL1S( "</h1><h2>" );
    doc += errorName;
    doc += QL1S( "</h2>" );

    if ( !techName.isNull() ) {
        doc += QL1S( "<h2>" );
        doc += i18n( "Technical Reason: %1", techName );
        doc += QL1S( "</h2>" );
    }

    doc += QL1S( "<h3>" );
    doc += i18n( "Details of the Request:" );
    doc += QL1S( "</h3><ul><li>" );
    // escape URL twice: once for i18n, and once for HTML.
    doc += i18n( "URL: %1", reqUrl.toDisplayString().toHtmlEscaped().toHtmlEscaped() );
    doc += QL1S( "</li><li>" );

    const QString protocol (reqUrl.scheme());
    if ( !protocol.isNull() ) {
        // escape protocol twice: once for i18n, and once for HTML.
        doc += i18n( "Protocol: %1", protocol.toHtmlEscaped().toHtmlEscaped() );
        doc += QL1S( "</li><li>" );
    }

    doc += i18n( "Date and Time: %1",
                 QLocale().toString(QDateTime::currentDateTime(), QLocale::LongFormat) );
    doc += QL1S( "</li><li>" );
    // escape text twice: once for i18n, and once for HTML.
    doc += i18n( "Additional Information: %1", text.toHtmlEscaped().toHtmlEscaped() );
    doc += QL1S( "</li></ul><h3>" );
    doc += i18n( "Description:" );
    doc += QL1S( "</h3><p>" );
    doc += description.toHtmlEscaped();
    doc += QL1S( "</p>" );

    if ( causes.count() ) {
        doc += QL1S( "<h3>" );
        doc += i18n( "Possible Causes:" );
        doc += QL1S( "</h3><ul><li>" );
        doc += causes.join( "</li><li>" );
        doc += QL1S( "</li></ul>" );
    }

    if ( solutions.count() ) {
        doc += QL1S( "<h3>" );
        doc += i18n( "Possible Solutions:" );
        doc += QL1S( "</h3><ul><li>" );
        doc += solutions.join( "</li><li>" );
        doc += QL1S( "</li></ul>" );
    }

    html.replace( QL1S("TEXT"), doc );

    return html;
}

bool WebPage::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
    switch (extension) {
    case QWebPage::ErrorPageExtension: {
        if (!m_ignoreError) {
          const QWebPage::ErrorPageExtensionOption *extOption = static_cast<const QWebPage::ErrorPageExtensionOption*>(option);
          QWebPage::ErrorPageExtensionReturn *extOutput = static_cast<QWebPage::ErrorPageExtensionReturn*>(output);
          if (extOutput && extOption && extOption->domain != QWebPage::WebKit) {
            extOutput->content = errorPage(m_kioErrorCode, extOption->errorString, extOption->url).toUtf8();
            extOutput->baseUrl = extOption->url;
            return true;
          }
        }
        break;
    }
    case QWebPage::ChooseMultipleFilesExtension: {
        const QWebPage::ChooseMultipleFilesExtensionOption* extOption = static_cast<const QWebPage::ChooseMultipleFilesExtensionOption*> (option);
        QWebPage::ChooseMultipleFilesExtensionReturn *extOutput = static_cast<QWebPage::ChooseMultipleFilesExtensionReturn*>(output);
        if (extOutput && extOption && currentFrame() == extOption->parentFrame) {
            if (extOption->suggestedFileNames.isEmpty())
                extOutput->fileNames = QFileDialog::getOpenFileNames(view(),
                                                                     i18n("Choose files to upload"),
                                                                     QString(), QString());
            else
                extOutput->fileNames = QFileDialog::getOpenFileNames(view(),
                                                                     i18n("Choose files to upload"),
                                                                     extOption->suggestedFileNames.first(),
                                                                     QString());
            return true;
        }
        break;
    }
    default:
        break;
    }

    return KWebPage::extension(extension, option, output);
}

bool WebPage::supportsExtension(Extension extension) const
{
    //qCDebug(KWEBKITPART_LOG) << extension << m_ignoreError;
    switch (extension) {
    case QWebPage::ErrorPageExtension:
        return (!m_ignoreError);
    case QWebPage::ChooseMultipleFilesExtension:
        return true;
    default:
        break;
    }

    return KWebPage::supportsExtension(extension);
}

QWebPage *WebPage::createWindow(WebWindowType type)
{
    // qCDebug(KWEBKITPART_LOG) << "window type:" << type;
    // Crete an instance of NewWindowPage class to capture all the
    // information we need to create a new window. See documentation of
    // the class for more information...
    NewWindowPage* page = new NewWindowPage(type, part(), m_noJSOpenWindowCheck);
    m_noJSOpenWindowCheck = false;
    return page;
}

// Returns true if the scheme and domain of the two urls match...
static bool domainSchemeMatch(const QUrl& u1, const QUrl& u2)
{
    if (u1.scheme() != u2.scheme())
        return false;

    QStringList u1List = u1.host().split(QL1C('.'), QString::SkipEmptyParts);
    QStringList u2List = u2.host().split(QL1C('.'), QString::SkipEmptyParts);

    if (qMin(u1List.count(), u2List.count()) < 2)
        return false;  // better safe than sorry...

    while (u1List.count() > 2)
        u1List.removeFirst();

    while (u2List.count() > 2)
        u2List.removeFirst();

    return (u1List == u2List);
}

static void resetPluginsLoadedOnDemandFor(QWebPluginFactory* _factory)
{
    WebPluginFactory* factory = qobject_cast<WebPluginFactory*>(_factory);
    if (factory) {
        factory->resetPluginOnDemandList();
    }
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    QUrl reqUrl (request.url());

    // Handle "mailto:" url here...
    if (handleMailToUrl(reqUrl, type))
        return false;

    const bool isMainFrameRequest = (frame == mainFrame());
    const bool isTypedUrl = property("NavigationTypeUrlEntered").toBool();

    /*
      NOTE: We use a dynamic QObject property called "NavigationTypeUrlEntered"
      to distinguish between requests generated by user entering a url vs those
      that were generated programatically through javascript (AJAX requests).
    */
    if (isMainFrameRequest && isTypedUrl)
      setProperty("NavigationTypeUrlEntered", QVariant());

    if (frame) {
        // inPage requests are those generarted within the current page through
        // link clicks, javascript queries, and button clicks (form submission).
        bool inPageRequest = true;
        switch (type) {
        case QWebPage::NavigationTypeFormSubmitted:
            if (!checkFormData(request))
                return false;
            break;
        case QWebPage::NavigationTypeFormResubmitted:
            if (!checkFormData(request))
                return false;
            if (KMessageBox::warningContinueCancel(view(),
                            i18n("<qt><p>To display the requested web page again, "
                                  "the browser needs to resend information you have "
                                  "previously submitted.</p>"
                                  "<p>If you were shopping online and made a purchase, "
                                  "click the Cancel button to prevent a duplicate purchase."
                                  "Otherwise, click the Continue button to display the web"
                                  "page again.</p>"),
                            i18n("Resubmit Information")) == KMessageBox::Cancel) {
                return false;
            }
            break;
        case QWebPage::NavigationTypeBackOrForward:
            // If history navigation is locked, ignore all such requests...
            if (property("HistoryNavigationLocked").toBool()) {
                setProperty("HistoryNavigationLocked", QVariant());
                qCDebug(KWEBKITPART_LOG) << "Rejected history navigation because 'HistoryNavigationLocked' property is set!";
                return false;
            }
            //qCDebug(KWEBKITPART_LOG) << "Navigating to item (" << history()->currentItemIndex()
            //         << "of" << history()->count() << "):" << history()->currentItem().url();
            inPageRequest = false;
            if (!isBlankUrl(reqUrl)) {
                resetPluginsLoadedOnDemandFor(pluginFactory());
            }
            break;
        case QWebPage::NavigationTypeReload:
            setRequestMetaData(QL1S("cache"), QL1S("reload"));
            inPageRequest = false;
            if (!isBlankUrl(reqUrl)) {
                resetPluginsLoadedOnDemandFor(pluginFactory());
            }
            break;
        case QWebPage::NavigationTypeOther:
            inPageRequest = !isTypedUrl;
            if (isTypedUrl && !isBlankUrl(reqUrl)) {
              resetPluginsLoadedOnDemandFor(pluginFactory());
            }
            break;
        default:
            break;
        }

        if (inPageRequest) {
            if (!checkLinkSecurity(request, type))
                return false;

            if (m_sslInfo.isValid())
                setRequestMetaData(QL1S("ssl_was_in_use"), QL1S("TRUE"));
        }

        // Set the "main_frame_request" meta-data to aid SSL verification in KIO.
        setRequestMetaData(QL1S("main_frame_request"), (isMainFrameRequest ? QL1S("TRUE") : QL1S("FALSE")));

        // Insert the request into the queue...
        reqUrl.setUserInfo(QString());
        m_requestQueue << reqUrl;
    } else {
        // If request came from javascript, set m_noJSOpenWindowCheck to true.
        m_noJSOpenWindowCheck = (!isTypedUrl && type != QWebPage::NavigationTypeOther);
    }

    // Honor the enabling/disabling of plugins per host.
    settings()->setAttribute(QWebSettings::PluginsEnabled, WebKitSettings::self()->isPluginsEnabled(reqUrl.host()));
    return KWebPage::acceptNavigationRequest(frame, request, type);
}

QString WebPage::userAgentForUrl(const QUrl& url) const
{
    QString userAgent = KWebPage::userAgentForUrl(url);

    // Remove the useless "U" if it is present.
    const int index = userAgent.indexOf(QL1S(" U;"), -1, Qt::CaseInsensitive);
    if (index > -1)
        userAgent.remove(index, 3);

    return userAgent.trimmed();
}

static int errorCodeFromReply(QNetworkReply* reply)
{
    // First check if there is a KIO error code sent back and use that,
    // if not attempt to convert QNetworkReply's NetworkError to KIO::Error.
    QVariant attr = reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::KioError));
    if (attr.isValid() && attr.type() == QVariant::Int)
        return attr.toInt();

    switch (reply->error()) {
        case QNetworkReply::ConnectionRefusedError:
            return KIO::ERR_COULD_NOT_CONNECT;
        case QNetworkReply::HostNotFoundError:
            return KIO::ERR_UNKNOWN_HOST;
        case QNetworkReply::TimeoutError:
            return KIO::ERR_SERVER_TIMEOUT;
        case QNetworkReply::OperationCanceledError:
            return KIO::ERR_USER_CANCELED;
        case QNetworkReply::ProxyNotFoundError:
            return KIO::ERR_UNKNOWN_PROXY_HOST;
        case QNetworkReply::ContentAccessDenied:
            return KIO::ERR_ACCESS_DENIED;
        case QNetworkReply::ContentOperationNotPermittedError:
            return KIO::ERR_WRITE_ACCESS_DENIED;
        case QNetworkReply::ContentNotFoundError:
            return KIO::ERR_NO_CONTENT;
        case QNetworkReply::AuthenticationRequiredError:
            return KIO::ERR_COULD_NOT_AUTHENTICATE;
        case QNetworkReply::ProtocolUnknownError:
            return KIO::ERR_UNSUPPORTED_PROTOCOL;
        case QNetworkReply::ProtocolInvalidOperationError:
            return KIO::ERR_UNSUPPORTED_ACTION;
        case QNetworkReply::UnknownNetworkError:
            return KIO::ERR_UNKNOWN;
        case QNetworkReply::NoError:
        default:
            break;
    }

    return 0;
}

KWebKitPart* WebPage::part() const
{
    return m_part.data();
}

void WebPage::setPart(KWebKitPart* part)
{
    m_part = part;
}

void WebPage::slotRequestFinished(QNetworkReply *reply)
{
    Q_ASSERT(reply);

    QUrl requestUrl (reply->request().url());
    requestUrl.setUserInfo(QString());

    // Disregards requests that are not in the request queue...
    if (!m_requestQueue.removeOne(requestUrl))
        return;

    QWebFrame* frame = qobject_cast<QWebFrame *>(reply->request().originatingObject());
    if (!frame)
        return;

    const bool shouldResetSslInfo = (m_sslInfo.isValid() && !domainSchemeMatch(requestUrl, m_sslInfo.url()));
    // Only deal with non-redirect responses...
    const QVariant redirectVar = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    const bool isMainFrameRequest = (frame == mainFrame());

    if (isMainFrameRequest && redirectVar.isValid()) {
        m_sslInfo.restoreFrom(reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData)),
                              reply->url(), shouldResetSslInfo);
        return;
    }

    const int errCode = errorCodeFromReply(reply);
    qCDebug(KWEBKITPART_LOG) << frame << "is main frame request?" << isMainFrameRequest << requestUrl;
    // Handle any error...
    switch (errCode) {
        case 0:
        case KIO::ERR_NO_CONTENT:
            if (isMainFrameRequest) {
                m_sslInfo.restoreFrom(reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData)),
                                      reply->url(), shouldResetSslInfo);
                setPageJScriptPolicy(reply->url());
            }
            break;
        case KIO::ERR_ABORTED:
        case KIO::ERR_USER_CANCELED: // Do nothing if request is cancelled/aborted
            //qCDebug(KWEBKITPART_LOG) << "User aborted request!";
            m_ignoreError = true;
            emit loadAborted(QUrl());
            return;
        // Handle the user clicking on a link that refers to a directory
        // Since KIO cannot automatically convert a GET request to a LISTDIR one.
        case KIO::ERR_IS_DIRECTORY:
            m_ignoreError = true;
            emit loadAborted(reply->url());
            return;
        default:
            // Make sure the saveFrameStateRequested signal is emitted so
            // the page can restored properly.
            if (isMainFrameRequest)
                emit saveFrameStateRequested(frame, nullptr);

            m_ignoreError = (reply->attribute(QNetworkRequest::User).toInt() == QNetworkReply::ContentAccessDenied);
            m_kioErrorCode = errCode;
            break;
    }

    if (isMainFrameRequest) {
        const WebPageSecurity security = (m_sslInfo.isValid() ? PageEncrypted : PageUnencrypted);
        emit m_part->browserExtension()->setPageSecurity(security);
    }
}

void WebPage::slotUnsupportedContent(QNetworkReply* reply)
{
    //qCDebug(KWEBKITPART_LOG) << reply->url();
    QString mimeType;
    KIO::MetaData metaData;

    KIO::AccessManager::putReplyOnHold(reply);
    QString downloadCmd;
    checkForDownloadManager(view(), downloadCmd);
    if (!downloadCmd.isEmpty()) {
        reply->setProperty("DownloadManagerExe", downloadCmd);
    }

    if (KWebPage::handleReply(reply, &mimeType, &metaData)) {
        reply->deleteLater();
        if (qobject_cast<NewWindowPage*>(this) && isBlankUrl(m_part->url())) {
            m_part->closeUrl();
            if (m_part->arguments().metaData().contains(QL1S("new-window"))) {
                m_part->widget()->topLevelWidget()->close();
            } else {
                delete m_part;
            }
        }
        return;
    }

    //qCDebug(KWEBKITPART_LOG) << "mimetype=" << mimeType << "metadata:" << metaData;

    if (reply->request().originatingObject() == this->mainFrame()) {
        KParts::OpenUrlArguments args;
        args.setMimeType(mimeType);
        args.metaData() = metaData;
        emit m_part->browserExtension()->openUrlRequest(reply->url(), args, KParts::BrowserArguments());
        return;
    }
    reply->deleteLater();
}

void WebPage::slotFeaturePermissionRequested(QWebFrame* frame, QWebPage::Feature feature)
{
    if (frame == mainFrame()) {
        part()->slotShowFeaturePermissionBar(feature);
        return;
    }
    switch(feature) {
    case QWebPage::Notifications:
        // FIXME: We should have a setting to tell if this is enabled, but so far it is always enabled.
        setFeaturePermission(frame, feature, QWebPage::PermissionGrantedByUser);
        break;
    case QWebPage::Geolocation:
        if (KMessageBox::warningContinueCancel(nullptr, i18n("This site is attempting to "
                                                       "access information about your "
                                                       "physical location.\n"
                                                       "Do you want to allow it access?"),
                                            i18n("Network Transmission"),
                                            KGuiItem(i18n("Allow access")),
                                            KStandardGuiItem::cancel(),
                                            "WarnGeolocation") == KMessageBox::Cancel) {
            setFeaturePermission(frame, feature, QWebPage::PermissionDeniedByUser);
        } else {
            setFeaturePermission(frame, feature, QWebPage::PermissionGrantedByUser);
        }
        break;
    default:
        setFeaturePermission(frame, feature, QWebPage::PermissionUnknown);
        break;
    }
}

void WebPage::slotGeometryChangeRequested(const QRect & rect)
{
    const QString host = mainFrame()->url().host();

    // NOTE: If a new window was created from another window which is in
    // maximized mode and its width and/or height were not specified at the
    // time of its creation, which is always the case in QWebPage::createWindow,
    // then any move operation will seem not to work. That is because the new
    // window will be in maximized mode where moving it will not be possible...
    if (WebKitSettings::self()->windowMovePolicy(host) == KParts::HtmlSettingsInterface::JSWindowMoveAllow &&
        (view()->x() != rect.x() || view()->y() != rect.y()))
        emit m_part->browserExtension()->moveTopLevelWidget(rect.x(), rect.y());

    const int height = rect.height();
    const int width = rect.width();

    // parts of following code are based on kjs_window.cpp
    // Security check: within desktop limits and bigger than 100x100 (per spec)
    if (width < 100 || height < 100) {
        qCWarning(KWEBKITPART_LOG) << "Window resize refused, window would be too small (" << width << "," << height << ")";
        return;
    }

    QRect sg = QApplication::desktop()->screenGeometry(view());

    if (width > sg.width() || height > sg.height()) {
        qCWarning(KWEBKITPART_LOG) << "Window resize refused, window would be too big (" << width << "," << height << ")";
        return;
    }

    if (WebKitSettings::self()->windowResizePolicy(host) == KParts::HtmlSettingsInterface::JSWindowResizeAllow) {
        //qCDebug(KWEBKITPART_LOG) << "resizing to " << width << "x" << height;
        emit m_part->browserExtension()->resizeTopLevelWidget(width, height);
    }

    // If the window is out of the desktop, move it up/left
    // (maybe we should use workarea instead of sg, otherwise the window ends up below kicker)
    const int right = view()->x() + view()->frameGeometry().width();
    const int bottom = view()->y() + view()->frameGeometry().height();
    int moveByX = 0, moveByY = 0;
    if (right > sg.right())
        moveByX = - right + sg.right(); // always <0
    if (bottom > sg.bottom())
        moveByY = - bottom + sg.bottom(); // always <0

    if ((moveByX || moveByY) && WebKitSettings::self()->windowMovePolicy(host) == KParts::HtmlSettingsInterface::JSWindowMoveAllow)
        emit m_part->browserExtension()->moveTopLevelWidget(view()->x() + moveByX, view()->y() + moveByY);
}

bool WebPage::checkLinkSecurity(const QNetworkRequest &req, NavigationType type) const
{
    // Check whether the request is authorized or not...
    if (!KUrlAuthorized::authorizeUrlAction("redirect", mainFrame()->url(), req.url())) {

        //qCDebug(KWEBKITPART_LOG) << "*** Failed security check: base-url=" << mainFrame()->url() << ", dest-url=" << req.url();
        QString buttonText, title, message;

        int response = KMessageBox::Cancel;
        QUrl linkUrl (req.url());

        if (type == QWebPage::NavigationTypeLinkClicked) {
            message = i18n("<qt>This untrusted page links to<br/><b>%1</b>."
                           "<br/>Do you want to follow the link?</qt>", linkUrl.url());
            title = i18n("Security Warning");
            buttonText = i18nc("follow link despite of security warning", "Follow");
        } else {
            title = i18n("Security Alert");
            message = i18n("<qt>Access by untrusted page to<br/><b>%1</b><br/> denied.</qt>",
                           linkUrl.toDisplayString().toHtmlEscaped());
        }

        if (buttonText.isEmpty()) {
            KMessageBox::error( nullptr, message, title);
        } else {
            // Dangerous flag makes the Cancel button the default
            response = KMessageBox::warningContinueCancel(nullptr, message, title,
                                                          KGuiItem(buttonText),
                                                          KStandardGuiItem::cancel(),
                                                          QString(), // no don't ask again info
                                                          KMessageBox::Notify | KMessageBox::Dangerous);
        }

        return (response == KMessageBox::Continue);
    }

    return true;
}

bool WebPage::checkFormData(const QNetworkRequest &req) const
{
    const QString scheme (req.url().scheme());

    if (m_sslInfo.isValid() &&
        !scheme.compare(QL1S("https")) && !scheme.compare(QL1S("mailto")) &&
        (KMessageBox::warningContinueCancel(nullptr,
                                           i18n("Warning: This is a secure form "
                                                "but it is attempting to send "
                                                "your data back unencrypted.\n"
                                                "A third party may be able to "
                                                "intercept and view this "
                                                "information.\nAre you sure you "
                                                "want to send the data unencrypted?"),
                                           i18n("Network Transmission"),
                                           KGuiItem(i18n("&Send Unencrypted")))  == KMessageBox::Cancel)) {

        return false;
    }


    if (scheme.compare(QL1S("mailto")) == 0 &&
        (KMessageBox::warningContinueCancel(nullptr, i18n("This site is attempting to "
                                                    "submit form data via email.\n"
                                                    "Do you want to continue?"),
                                            i18n("Network Transmission"),
                                            KGuiItem(i18n("&Send Email")),
                                            KStandardGuiItem::cancel(),
                                            "WarnTriedEmailSubmit") == KMessageBox::Cancel)) {
        return false;
    }

    return true;
}

// Sanitizes the "mailto:" url, e.g. strips out any "attach" parameters.
static QUrl sanitizeMailToUrl(const QUrl &url, QStringList& files) {
    QUrl sanitizedUrl;

    // NOTE: This is necessary to ensure we can properly use QUrl's query
    // related APIs to process 'mailto:' urls of form 'mailto:foo@bar.com'.
    if (url.hasQuery())
      sanitizedUrl = url;
    else
      sanitizedUrl = QUrl(url.scheme() + QL1S(":?") + url.path());

    QListIterator<QPair<QString, QString> > it (QUrlQuery(sanitizedUrl).queryItems());
    QUrlQuery newQuery;

    while (it.hasNext()) {
        QPair<QString, QString> queryItem = it.next();
        if (queryItem.first.contains(QL1C('@')) && queryItem.second.isEmpty()) {
            // ### DF: this hack breaks mailto:faure@kde.org, kmail doesn't expect mailto:?to=faure@kde.org
            queryItem.second = queryItem.first;
            queryItem.first = "to";
        } else if (QString::compare(queryItem.first, QL1S("attach"), Qt::CaseInsensitive) == 0) {
            files << queryItem.second;
            continue;
        }
        newQuery.addQueryItem(queryItem.first, queryItem.second);
    }

    sanitizedUrl.setQuery(newQuery);			// replace the query component
    return sanitizedUrl;
}

bool WebPage::handleMailToUrl (const QUrl &url, NavigationType type) const
{
    if (QString::compare(url.scheme(), QL1S("mailto"), Qt::CaseInsensitive) == 0) {
        QStringList files;
        QUrl mailtoUrl (sanitizeMailToUrl(url, files));

        switch (type) {
            case QWebPage::NavigationTypeLinkClicked:
                if (!files.isEmpty() && KMessageBox::warningContinueCancelList(nullptr,
                                                                               i18n("<qt>Do you want to allow this site to attach "
                                                                                    "the following files to the email message?</qt>"),
                                                                               files, i18n("Email Attachment Confirmation"),
                                                                               KGuiItem(i18n("&Allow attachments")),
                                                                               KGuiItem(i18n("&Ignore attachments")), QL1S("WarnEmailAttachment")) == KMessageBox::Continue) {

                   // Re-add the attachments...
                    QUrlQuery newQuery(mailtoUrl);
                    QStringListIterator filesIt (files);
                    while (filesIt.hasNext()) {
                        newQuery.addQueryItem(QL1S("attach"), filesIt.next());
                    }
                    mailtoUrl.setQuery(newQuery);
                }
                break;
            case QWebPage::NavigationTypeFormSubmitted:
            case QWebPage::NavigationTypeFormResubmitted:
                if (!files.isEmpty()) {
                    KMessageBox::information(nullptr, i18n("This site attempted to attach a file from your "
                                                     "computer in the form submission. The attachment "
                                                     "was removed for your protection."),
                                             i18n("Attachment Removed"), "InfoTriedAttach");
                }
                break;
            default:
                 break;
        }

        //qCDebug(KWEBKITPART_LOG) << "Emitting openUrlRequest with " << mailtoUrl;
        emit m_part->browserExtension()->openUrlRequest(mailtoUrl);
        return true;
    }

    return false;
}

void WebPage::setPageJScriptPolicy(const QUrl &url)
{
    const QString hostname (url.host());
    settings()->setAttribute(QWebSettings::JavascriptEnabled,
                             WebKitSettings::self()->isJavaScriptEnabled(hostname));

    const KParts::HtmlSettingsInterface::JSWindowOpenPolicy policy = WebKitSettings::self()->windowOpenPolicy(hostname);
    settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows,
                             (policy != KParts::HtmlSettingsInterface::JSWindowOpenDeny &&
                              policy != KParts::HtmlSettingsInterface::JSWindowOpenSmart));
}





/************************************* Begin NewWindowPage ******************************************/

NewWindowPage::NewWindowPage(WebWindowType type, KWebKitPart* part, bool disableJSOpenwindowCheck, QWidget* parent)
              :WebPage(part, parent) , m_type(type) , m_createNewWindow(true)
              , m_disableJSOpenwindowCheck(disableJSOpenwindowCheck)
{
    Q_ASSERT_X (part, "NewWindowPage", "Must specify a valid KPart");

    connect(this, SIGNAL(menuBarVisibilityChangeRequested(bool)),
            this, SLOT(slotMenuBarVisibilityChangeRequested(bool)));
    connect(this, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            this, SLOT(slotToolBarVisibilityChangeRequested(bool)));
    connect(this, SIGNAL(statusBarVisibilityChangeRequested(bool)),
            this, SLOT(slotStatusBarVisibilityChangeRequested(bool)));
    connect(mainFrame(), SIGNAL(loadFinished(bool)), this, SLOT(slotLoadFinished(bool)));
}

NewWindowPage::~NewWindowPage()
{
    //qCDebug(KWEBKITPART_LOG) << this;
}

bool NewWindowPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    // qCDebug(KWEBKITPART_LOG) << "url:" << request.url() << ",type:" << type << ",frame:" << frame;
    if (m_createNewWindow) {
        const QUrl reqUrl (request.url());

        if (!m_disableJSOpenwindowCheck) {
            const KParts::HtmlSettingsInterface::JSWindowOpenPolicy policy = WebKitSettings::self()->windowOpenPolicy(reqUrl.host());
            switch (policy) {
            case KParts::HtmlSettingsInterface::JSWindowOpenDeny:
                // TODO: Implement support for dealing with blocked pop up windows.
                this->deleteLater();
                return false;
            case KParts::HtmlSettingsInterface::JSWindowOpenAsk: {
                const QString message = (reqUrl.isEmpty() ?
                                          i18n("This site is requesting to open a new popup window.\n"
                                               "Do you want to allow this?") :
                                          i18n("<qt>This site is requesting to open a popup window to"
                                               "<p>%1</p><br/>Do you want to allow this?</qt>",
                                               KStringHandler::rsqueeze(reqUrl.toDisplayString().toHtmlEscaped(), 100)));
                if (KMessageBox::questionYesNo(view(), message,
                                               i18n("Javascript Popup Confirmation"),
                                               KGuiItem(i18n("Allow")),
                                               KGuiItem(i18n("Do Not Allow"))) == KMessageBox::No) {
                    // TODO: Implement support for dealing with blocked pop up windows.
                    this->deleteLater();
                    return false;
                }
               break;
            }
            default:
                break;
            }
        }

        if (!part() && frame != mainFrame() && type != QWebPage::NavigationTypeOther)
            return false;

        // Browser args...
        KParts::BrowserArguments bargs;
        bargs.frameName = mainFrame()->frameName();
        if (m_type == WebModalDialog)
            bargs.setForcesNewWindow(true);

        // OpenUrl args...
        KParts::OpenUrlArguments uargs;
        uargs.setMimeType(QL1S("text/html"));
        uargs.setActionRequestedByUser(m_disableJSOpenwindowCheck);

        // Window args...
        KParts::WindowArgs wargs (m_windowArgs);

        KParts::ReadOnlyPart* newWindowPart =nullptr;
        part()->browserExtension()->createNewWindow(QUrl(), uargs, bargs, wargs, &newWindowPart);
        qCDebug(KWEBKITPART_LOG) << "Created new window" << newWindowPart;

        if (!newWindowPart) {
            return false;
        } else if (newWindowPart->widget()->topLevelWidget() != part()->widget()->topLevelWidget()) {
            KParts::OpenUrlArguments args;
            args.metaData().insert(QL1S("new-window"), QL1S("true"));
            newWindowPart->setArguments(args);
        }

        // Get the webview...
        KWebKitPart* webkitPart = qobject_cast<KWebKitPart*>(newWindowPart);
        WebView* webView = webkitPart ? qobject_cast<WebView*>(webkitPart->view()) : nullptr;

        // If the newly created window is NOT a webkitpart...
        if (!webView) {
            newWindowPart->openUrl(reqUrl);
            this->deleteLater();
            return false;
        }
        // Reparent this page to the new webview to prevent memory leaks.
        setParent(webView);
        // Replace the webpage of the new webview with this one. Nice trick...
        webView->setPage(this);
        // Set the new part as the one this page will use going forward.
        setPart(webkitPart);
        // Connect all the signals from this page to the slots in the new part.
        webkitPart->connectWebPageSignals(this);
        //Set the create new window flag to false...
        m_createNewWindow = false;
    }

    return WebPage::acceptNavigationRequest(frame, request, type);
}

void NewWindowPage::slotGeometryChangeRequested(const QRect & rect)
{
    if (!rect.isValid())
        return;

    if (!m_createNewWindow) {
        WebPage::slotGeometryChangeRequested(rect);
        return;
    }

    m_windowArgs.setX(rect.x());
    m_windowArgs.setY(rect.y());
    m_windowArgs.setWidth(qMax(rect.width(), 100));
    m_windowArgs.setHeight(qMax(rect.height(), 100));
}

void NewWindowPage::slotMenuBarVisibilityChangeRequested(bool visible)
{
    //qCDebug(KWEBKITPART_LOG) << visible;
    m_windowArgs.setMenuBarVisible(visible);
}

void NewWindowPage::slotStatusBarVisibilityChangeRequested(bool visible)
{
    //qCDebug(KWEBKITPART_LOG) << visible;
    m_windowArgs.setStatusBarVisible(visible);
}

void NewWindowPage::slotToolBarVisibilityChangeRequested(bool visible)
{
    //qCDebug(KWEBKITPART_LOG) << visible;
    m_windowArgs.setToolBarsVisible(visible);
}

void NewWindowPage::slotLoadFinished(bool ok)
{
    Q_UNUSED(ok)
    //qCDebug(KWEBKITPART_LOG) << ok;
    if (!m_createNewWindow)
        return;

    // Browser args...
    KParts::BrowserArguments bargs;
    bargs.frameName = mainFrame()->frameName();
    if (m_type == WebModalDialog)
        bargs.setForcesNewWindow(true);

    // OpenUrl args...
    KParts::OpenUrlArguments uargs;
    uargs.setMimeType(QL1S("text/html"));
    uargs.setActionRequestedByUser(m_disableJSOpenwindowCheck);

    // Window args...
    KParts::WindowArgs wargs (m_windowArgs);

    KParts::ReadOnlyPart* newWindowPart =nullptr;
    part()->browserExtension()->createNewWindow(QUrl(), uargs, bargs, wargs, &newWindowPart);

    qCDebug(KWEBKITPART_LOG) << "Created new window" << newWindowPart;

    // Get the webview...
    KWebKitPart* webkitPart = newWindowPart ? qobject_cast<KWebKitPart*>(newWindowPart) : nullptr;
    WebView* webView = webkitPart ? qobject_cast<WebView*>(webkitPart->view()) : nullptr;

    if (webView) {
        // if a new window is created, set a new window meta-data flag.
        if (newWindowPart->widget()->topLevelWidget() != part()->widget()->topLevelWidget()) {
            KParts::OpenUrlArguments args;
            args.metaData().insert(QL1S("new-window"), QL1S("true"));
            newWindowPart->setArguments(args);
        }
        // Reparent this page to the new webview to prevent memory leaks.
        setParent(webView);
        // Replace the webpage of the new webview with this one. Nice trick...
        webView->setPage(this);
        // Set the new part as the one this page will use going forward.
        setPart(webkitPart);
        // Connect all the signals from this page to the slots in the new part.
        webkitPart->connectWebPageSignals(this);
    }

    //Set the create new window flag to false...
    m_createNewWindow = false;
}

/****************************** End NewWindowPage *************************************************/
