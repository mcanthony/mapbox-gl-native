#include "qsqlitecache_p.hpp"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#include <mbgl/storage/sqlite_cache.hpp>
#include <mbgl/util/compression.hpp>
#include <mbgl/util/mapbox.hpp>
#include <mbgl/platform/log.hpp>

QSqliteCachePrivate::QSqliteCachePrivate(const QString& path)
    : m_cache(QSqlDatabase::addDatabase("QSQLITE")) {
    m_cache.setDatabaseName(path);

    if (!m_cache.open()) {
        mbgl::Log::Error(mbgl::Event::Database, "Error opening database %s: %s", path.data(),
                         m_cache.lastError().text().data());
    } else {
        QSqlQuery createSchema = m_cache.exec(
            "CREATE TABLE IF NOT EXISTS `http_cache` ("
            "    `url` TEXT PRIMARY KEY NOT NULL,"
            "    `status` INTEGER NOT NULL," // The response status (Successful or Error).
            "    `kind` INTEGER NOT NULL,"   // The kind of file.
            "    `modified` INTEGER,"        // Timestamp when the file was last modified.
            "    `etag` TEXT,"
            "    `expires` INTEGER," // Timestamp when the server says the file expires.
            "    `data` BLOB,"
            "    `compressed` INTEGER NOT NULL DEFAULT 0" // Whether the data is compressed.
            ");");
        if (!createSchema.isActive()) {
            mbgl::Log::Error(mbgl::Event::Database, "Failed to create database schema: %s",
                             createSchema.lastError().text().data());
        }

        QSqlQuery createIndex = m_cache.exec(
            "CREATE INDEX IF NOT EXISTS `http_cache_kind_idx` ON `http_cache` (`kind`);");
        if (!createIndex.isActive()) {
            mbgl::Log::Error(mbgl::Event::Database, "Failed to create database index: %s",
                             createIndex.lastError().text().data());
        }

        m_select = new QSqlQuery(m_cache);
        //                                0          1         2        3         4
        if (!m_select->prepare("SELECT `status`, `modified`, `etag`, `expires`, `data`, "
                               //    5
                               "`compressed` FROM `http_cache` WHERE `url` = ?")) {
            mbgl::Log::Error(mbgl::Event::Database, "Failed to prepare select statement: %s",
                             m_select->lastError().text().data());
        }

        m_insert = new QSqlQuery(m_cache);
        if (!m_insert->prepare( //            0       1        2         3
                "REPLACE INTO `http_cache` (`url`, `status`, `kind`, `modified`, "
                // 4         5        6           7
                "`etag`, `expires`, `data`, `compressed`) VALUES(?, ?, ?, ?, ?, ?, ?, ?)")) {
            mbgl::Log::Error(mbgl::Event::Database, "Failed to prepare insert statement: %s",
                             m_insert->lastError().text().data());
        }

        m_update = new QSqlQuery(m_cache); //                       1               2
        if (!m_update->prepare("UPDATE `http_cache` SET `expires` = ? WHERE `url` = ?")) {
            mbgl::Log::Error(mbgl::Event::Database, "Failed to prepare delete statement: %s",
                             m_update->lastError().text().data());
        }

        m_delete = new QSqlQuery(m_cache); //                          1
        if (!m_delete->prepare("DELETE FROM `http_cache` WHERE `url` = ?")) {
            mbgl::Log::Error(mbgl::Event::Database, "Failed to prepare delete statement: %s",
                             m_delete->lastError().text().data());
        }
    }
}

bool QSqliteCachePrivate::isValid() const {
    return m_cache.isOpen();
}

qint64 QSqliteCachePrivate::cacheSize() const {
    QSqlQuery size = m_cache.exec("SELECT SUM(LENGTH(`data`)) FROM `http_cache`");
    if (!size.isActive() || !size.first()) {
        mbgl::Log::Warning(mbgl::Event::Database, "Failed to determine cache size: %s",
                           size.lastError().text().data());
        return 0;
    } else {
        return size.value(0).toULongLong();
    }
}

QIODevice* QSqliteCachePrivate::data(const QUrl& url) {
    if (m_metaData.url() != url) {
        metaData(url);
    }

    m_buffer->open(QIODevice::ReadOnly);
    return m_buffer.take();
}

QString unifyURL(const QUrl& url) {
    QString normalized(url.toString());

    // XXX: In the cache, we use % encoding for the URLs, but only
    // for whitespace on the fonts names.
    if (normalized.contains("/font")) {
        normalized = QUrl::toPercentEncoding(normalized, ":/?=");
        normalized = normalized.replace("%2C", "%2c");
    }

    std::string str(mbgl::util::mapbox::canonicalURL(
        std::string(normalized.toUtf8().constData(), normalized.size())));

    // FIXME: Too many data conversions and copies.
    return QString::fromUtf8(str.data(), str.size());
}

QNetworkCacheMetaData QSqliteCachePrivate::metaData(const QUrl& url) {
    m_metaData = QNetworkCacheMetaData();
    m_metaData.setUrl(url);

    m_select->bindValue(0, unifyURL(url));

    if (m_select->exec()) {
        if (!m_select->first() || !m_select->value(0).toBool()) {
            return m_metaData;
        }
    } else {
        mbgl::Log::Warning(mbgl::Event::Database, "Failed to fetch cached data: %s",
                           m_select->lastError().text().data());
        return m_metaData;
    }

    m_buffer.reset(new QBuffer);
    QByteArray cacheData(m_select->value(4).toByteArray());

    if (m_select->value(5).toBool()) { // Compressed.
        m_buffer->setData(qUncompress(cacheData));
    } else {
        m_buffer->setData(cacheData);
    }

    const auto expires = m_select->value(3).toULongLong();
    if (expires > 0) {
        m_metaData.setExpirationDate(QDateTime::fromMSecsSinceEpoch(expires * 1000));
    }
    const auto modified = m_select->value(1).toULongLong();
    if (modified > 0) {
        m_metaData.setLastModified(QDateTime::fromMSecsSinceEpoch(modified * 1000));
    }
    const auto etag = m_select->value(2).toByteArray();
    if (!etag.isEmpty()) {
        QNetworkCacheMetaData::RawHeaderList headers;
        headers.append({ "ETag", etag });
        m_metaData.setRawHeaders(headers);
    }
    m_metaData.setUrl(url);
    return m_metaData;
}

uint64_t convertTime(const QDateTime& time) {
    if (time.isNull()) {
        return 0;
    } else {
        return time.toMSecsSinceEpoch() / 1000;
    }
}

class QSqliteCachePrivateInsert : public QBuffer {
public:
    inline QSqliteCachePrivateInsert(const QNetworkCacheMetaData& metaData) : m_metaData(metaData) {
    }
    const QNetworkCacheMetaData m_metaData;

private:
    Q_DISABLE_COPY(QSqliteCachePrivateInsert)
};

QIODevice* QSqliteCachePrivate::prepare(const QNetworkCacheMetaData& metaData) {
    // There can be multiple cache insert operations going on at the same time. Therefore, we must
    // create a separate buffer for everyone and remember the data (using QSqliteCachePrivateInsert)
    QScopedPointer<QSqliteCachePrivateInsert> buffer;
    buffer.reset(new QSqliteCachePrivateInsert(metaData));
    buffer->open(QIODevice::ReadWrite);
    return buffer.take();
}

void QSqliteCachePrivate::insert(QIODevice* device) {
    QScopedPointer<QSqliteCachePrivateInsert> buffer(
        dynamic_cast<QSqliteCachePrivateInsert*>(device));
    auto& metaData = buffer->m_metaData;

    m_insert->bindValue(0 /* url */, unifyURL(metaData.url()));

    const auto status =
        metaData.attributes().value(QNetworkRequest::HttpStatusCodeAttribute, -1).toInt();
    m_insert->bindValue(1 /* status */, status == 200 ? 1 : 0);

    bool etag = false;
    m_insert->bindValue(2 /* kind */, 3);
    m_insert->bindValue(3 /* modified */, convertTime(metaData.lastModified()));
    m_insert->bindValue(4 /* etag */, "test");
    m_insert->bindValue(5 /* expires */, convertTime(metaData.expirationDate()));

    QPair<QByteArray, QByteArray> line;
    foreach (line, metaData.rawHeaders()) {
        if ("etag" == QString(line.first).toLower()) {
            m_insert->bindValue(4 /* etag */, QString(line.second));
            etag = true;
        }
    }

    if (!etag) {
        m_insert->bindValue(4 /* etag */, "");
    }
    // TODO: Compress data that looks easily compressible
    buffer->seek(0);
    const QByteArray& data = buffer->data();
    uint8_t sig[8];
    if (8 == buffer->peek(reinterpret_cast<char*>(sig), 8) &&
        ((sig[0] == 0x89 && sig[1] == 0x50 && sig[2] == 0x4e && sig[3] == 0x47 &&
          sig[4] == 0x0d && sig[5] == 0x0a && sig[6] == 0x1a && sig[7] == 0x0a) ||
         (sig[0] == 0xff && sig[1] == 0xd8 && sig[2] == 0xff) ||
         (sig[0] == 0x47 && sig[1] == 0x49 && sig[2] == 0x46 && sig[3] == 0x38))) {
        // The data has the signature of a compressed file, so we don't need to recompress.
        m_insert->bindValue(6 /* data */, data);
        m_insert->bindValue(7 /* compressed */, 0);
    } else {
        m_insert->bindValue(6 /* data */, qCompress(data));
        m_insert->bindValue(7 /* compressed */, 1);
    }

    if (!m_insert->exec()) {
        mbgl::Log::Warning(mbgl::Event::Database, "Failed to cache data: %s",
                           m_insert->lastError().text().data());
    }
}

bool QSqliteCachePrivate::remove(const QUrl& url) {
    m_delete->bindValue(0 /* url */, unifyURL(url));
    if (!m_delete->exec()) {
        mbgl::Log::Warning(mbgl::Event::Database, "Failed to delete cache entry: %s",
                           m_delete->lastError().text().data());
        return false;
    } else {
        return m_delete->numRowsAffected() > 0;
    }
}

void QSqliteCachePrivate::updateMetaData(const QNetworkCacheMetaData& metaData) {
    m_update->bindValue(0 /* expires */, convertTime(metaData.expirationDate()));
    m_update->bindValue(1 /* url */, unifyURL(metaData.url()));
    if (!m_update->exec()) {
        mbgl::Log::Warning(mbgl::Event::Database, "Failed to update cache entry: %s",
                           m_update->lastError().text().data());
    }
}

void QSqliteCachePrivate::clear() {
    QSqlQuery clear = m_cache.exec("DELETE FROM `http_cache`");
    if (!clear.isActive()) {
        mbgl::Log::Warning(mbgl::Event::Database, "Failed to clear cache: %s",
                           clear.lastError().text().data());
    }
}
