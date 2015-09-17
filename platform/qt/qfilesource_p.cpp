#include "qfilesource_p.hpp"

#include "qsqlitecache_p.hpp"

#include <mbgl/storage/http_context_base.hpp>
#include <mbgl/util/mapbox.hpp>
#include <mbgl/storage/request.hpp>
#include <mbgl/storage/response.hpp>
#include <mbgl/platform/log.hpp>

#include <QByteArray>
#include <QDir>
#include <QNetworkReply>
#include <QScopedPointer>
#include <QSslConfiguration>
#include <QNetworkProxyFactory>

// Max number of request we are sending simultaneously to
// QNetworkAccessManager to not overload it with requests.
// Many get canceled before we need to make the actual get().
const int kPendingMax = 4;

namespace mbgl {

// FIXME: Not in use, needed for linking as a library.
std::unique_ptr<HTTPContextBase> HTTPContextBase::createContext(uv_loop_s*) {
    return nullptr;
}

} // namespace mbgl

QFileSourcePrivate::QFileSourcePrivate() {
    QNetworkProxyFactory::setUseSystemConfiguration(true);

#if QT_VERSION >= 0x050000
    m_ssl.setProtocol(QSsl::SecureProtocols);
#else
    // Qt 4 defines SecureProtocols as TLS1 or SSL3, but we don't want SSL3.
    m_ssl.setProtocol(QSsl::TlsV1);
#endif

    m_ssl.setCaCertificates(QSslCertificate::fromPath("ca-bundle.crt"));
    if (m_ssl.caCertificates().isEmpty()) {
        mbgl::Log::Warning(mbgl::Event::HttpRequest, "Could not load list of certificate authorities");
    }

    connect(this, SIGNAL(urlRequested(mbgl::Request*)), this,
            SLOT(handleUrlRequest(mbgl::Request*)), Qt::QueuedConnection);
    connect(this, SIGNAL(urlCanceled(mbgl::Request*)), this, SLOT(handleUrlCancel(mbgl::Request*)),
            Qt::QueuedConnection);

    connect(&m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinish(QNetworkReply*)));
}

void QFileSourcePrivate::setAccessToken(const QString& token) {
    m_token = token.toUtf8().constData();
}

void QFileSourcePrivate::setCacheDatabase(const QString& path) {
    QScopedPointer<QSqliteCachePrivate> cache(new QSqliteCachePrivate(path));

    if (cache->isValid()) {
        m_manager.setCache(cache.take());
    }
}

mbgl::Request* QFileSourcePrivate::request(const mbgl::Resource& resource, Callback cb) {
    // WARNING: Must be thread-safe.

    std::string normalizedUrl;

    switch (resource.kind) {
    case mbgl::Resource::Kind::Style:
        normalizedUrl = mbgl::util::mapbox::normalizeStyleURL(resource.url, m_token);
        break;
    case mbgl::Resource::Kind::Source:
        normalizedUrl = mbgl::util::mapbox::normalizeSourceURL(resource.url, m_token);
        break;
    case mbgl::Resource::Kind::Glyphs:
        normalizedUrl = mbgl::util::mapbox::normalizeGlyphsURL(resource.url, m_token);
        break;
    case mbgl::Resource::Kind::SpriteImage:
    case mbgl::Resource::Kind::SpriteJSON:
        normalizedUrl = mbgl::util::mapbox::normalizeSpriteURL(resource.url, m_token);
        break;
    default:
        normalizedUrl = resource.url;
    }

    mbgl::Request* req = new mbgl::Request({ resource.kind, normalizedUrl }, std::move(cb));
    emit urlRequested(req);

    return req;
}

void QFileSourcePrivate::cancel(mbgl::Request* req) {
    // WARNING: Must be thread-safe.

    req->cancel();
    emit urlCanceled(req);
}

void QFileSourcePrivate::handleUrlRequest(mbgl::Request* req) {
    QUrl url =
        QUrl::fromPercentEncoding(QByteArray(req->resource.url.data(), req->resource.url.size()));

    if (url.scheme() == "asset") {
        url.setUrl("file://" + QDir::currentPath() + "/" + url.host() + url.path());
    }

#if QT_VERSION < 0x050000
    if (m_pending.size() >= kPendingMax && m_pending.constFind(url) == m_pending.end()) {
        m_requestQueue.enqueue(req);
        return;
    }
#endif

    QPair<QNetworkReply*, QVector<mbgl::Request*>>& data = m_pending[url];
    QVector<mbgl::Request*>& requests = data.second;
    requests.append(req);

    if (requests.size() > 1) {
        return;
    }

    QNetworkRequest qreq = QNetworkRequest(url);
    qreq.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    qreq.setRawHeader("User-Agent", "MapboxGL/1.0 [Qt]");
    qreq.setSslConfiguration(m_ssl);

    data.first = m_manager.get(qreq);
}

void QFileSourcePrivate::handleUrlCancel(mbgl::Request* req) {
#if QT_VERSION < 0x050000
    int queueIndex = m_requestQueue.indexOf(req);

    if (queueIndex != -1) {
        m_requestQueue.removeAt(queueIndex);
        req->destruct();
        return;
    }
#endif

    QUrl url =
        QUrl::fromPercentEncoding(QByteArray(req->resource.url.data(), req->resource.url.size()));

    auto it = m_pending.find(url);
    if (it == m_pending.end()) {
        req->destruct();
        return;
    }

    QPair<QNetworkReply*, QVector<mbgl::Request*>>& data = it.value();
    QNetworkReply* reply = data.first;
    QVector<mbgl::Request*>& requests = data.second;

    for (int i = 0; i < requests.size(); ++i) {
        if (req == requests.at(i)) {
            requests.remove(i);
            break;
        }
    }

    req->destruct();

    if (requests.empty()) {
        m_pending.erase(it);
#if QT_VERSION >= 0x050000
        reply->abort();
#else
        // XXX: We should be aborting the reply here
        // but a bug on Qt4 causes the connection of
        // other ongoing requests to drop if we call
        // abort() too often (and we do).
        //
        // reply->abort();
        //
        Q_UNUSED(reply);
#endif
    }
}

void QFileSourcePrivate::replyFinish(QNetworkReply* reply) {
    const QUrl& url = reply->request().url();

    auto it = m_pending.find(url);
    if (it == m_pending.end()) {
        reply->deleteLater();
#if QT_VERSION < 0x050000
        processQueue();
#endif
        return;
    }

    std::shared_ptr<mbgl::Response> response = std::make_shared<mbgl::Response>();
    if (reply->error()) {
        response->message = reply->errorString().toUtf8().constData();
    } else {
        QByteArray bytes = reply->readAll();
        if (bytes.size()) {
            response->status = mbgl::Response::Status::Successful;
            response->data = std::string(bytes.data(), bytes.size());
        }
    }

    QVector<mbgl::Request*>& requests = it.value().second;
    for (auto req : requests) {
        req->notify(response);
    }

    m_pending.erase(it);
    reply->deleteLater();
#if QT_VERSION < 0x050000
    processQueue();
#endif
}

#if QT_VERSION < 0x050000
void QFileSourcePrivate::processQueue()
{
    if (!m_requestQueue.isEmpty()) {
        handleUrlRequest(m_requestQueue.dequeue());
    }
}
#endif
