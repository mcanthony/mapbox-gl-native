#ifndef QFILESOURCE
#define QFILESOURCE

#include <mbgl/storage/file_source.hpp>

#include <QMap>
#include <QNetworkAccessManager>
#include <QSslConfiguration>
#include <QObject>
#include <QPair>
#include <QString>
#include <QUrl>
#include <QVector>

class QFileSourcePrivate : public QObject, public mbgl::FileSource {
    Q_OBJECT

public:
    QFileSourcePrivate();
    ~QFileSourcePrivate() override = default;

    void setAccessToken(const QString& token);
    void setCacheDatabase(const QString& path);

    // FileSource implementation.
    mbgl::Request* request(const mbgl::Resource&, Callback) override;
    void cancel(mbgl::Request*) override;

signals:
    void urlRequested(mbgl::Request*);
    void urlCanceled(mbgl::Request*);

public slots:
    void handleUrlRequest(mbgl::Request*);
    void handleUrlCancel(mbgl::Request*);

    void replyFinish(QNetworkReply* reply);

private:
    QMap<QUrl, QPair<QNetworkReply*, QVector<mbgl::Request*>>> m_pending;
    QNetworkAccessManager m_manager;
    QSslConfiguration m_ssl;

    std::string m_token;
};

#endif
