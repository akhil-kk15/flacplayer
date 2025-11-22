#include "discogsclient.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QUrl>
#include <QDebug>

DiscogsClient::DiscogsClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_userAgent("FLACPlayer/1.0")
    , m_baseUrl("https://api.discogs.com")
{
}

DiscogsClient::~DiscogsClient()
{
}

void DiscogsClient::setUserAgent(const QString &userAgent)
{
    m_userAgent = userAgent;
}

void DiscogsClient::setApiToken(const QString &token)
{
    m_apiToken = token;
}

void DiscogsClient::searchRelease(const QString &artist, const QString &album)
{
    QUrl url(m_baseUrl + "/database/search");
    QUrlQuery query;
    
    QString searchQuery;
    if (!artist.isEmpty() && !album.isEmpty()) {
        searchQuery = QString("artist:\"%1\" release_title:\"%2\"").arg(artist, album);
    } else if (!album.isEmpty()) {
        searchQuery = album;
    } else if (!artist.isEmpty()) {
        searchQuery = artist;
    } else {
        emit errorOccurred("Search query cannot be empty");
        return;
    }
    
    query.addQueryItem("q", searchQuery);
    query.addQueryItem("type", "release");
    query.addQueryItem("per_page", "20");
    
    if (!m_apiToken.isEmpty()) {
        query.addQueryItem("token", m_apiToken);
    }
    
    url.setQuery(query);
    
    qDebug() << "======================================";
    qDebug() << "DISCOGS API: Starting search request";
    qDebug() << "Search Query:" << searchQuery;
    qDebug() << "Full URL:" << url.toString();
    qDebug() << "======================================";
    makeRequest(url.toString(), false);
}

void DiscogsClient::searchByBarcode(const QString &barcode)
{
    QUrl url(m_baseUrl + "/database/search");
    QUrlQuery query;
    
    query.addQueryItem("barcode", barcode);
    query.addQueryItem("type", "release");
    
    if (!m_apiToken.isEmpty()) {
        query.addQueryItem("token", m_apiToken);
    }
    
    url.setQuery(query);
    
    qDebug() << "======================================";
    qDebug() << "DISCOGS API: Barcode search";
    qDebug() << "Barcode:" << barcode;
    qDebug() << "======================================";
    makeRequest(url.toString(), false);
}

void DiscogsClient::getRelease(const QString &releaseId)
{
    QString url = m_baseUrl + "/releases/" + releaseId;
    
    if (!m_apiToken.isEmpty()) {
        url += "?token=" + m_apiToken;
    }
    
    qDebug() << "======================================";
    qDebug() << "DISCOGS API: Getting release details";
    qDebug() << "Release ID:" << releaseId;
    qDebug() << "URL:" << url;
    qDebug() << "======================================";
    makeRequest(url, true);
}

void DiscogsClient::makeRequest(const QString &url, bool isReleaseRequest)
{
    qDebug() << "DISCOGS API: Creating network request";
    qDebug() << "  User-Agent:" << m_userAgent;
    qDebug() << "  Request Type:" << (isReleaseRequest ? "Release Details" : "Search");
    
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", m_userAgent.toUtf8());
    
    QNetworkReply *reply = m_networkManager->get(request);
    qDebug() << "DISCOGS API: Request sent, waiting for response...";
    reply->setProperty("isReleaseRequest", isReleaseRequest);
    
    if (isReleaseRequest) {
        connect(reply, &QNetworkReply::finished, this, &DiscogsClient::onReleaseFinished);
    } else {
        connect(reply, &QNetworkReply::finished, this, &DiscogsClient::onSearchFinished);
    }
}

void DiscogsClient::onSearchFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    reply->deleteLater();
    
    qDebug() << "======================================";
    qDebug() << "DISCOGS API: Search response received";
    qDebug() << "  HTTP Status Code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "  HTTP Status Reason:" << reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "  ERROR:" << reply->errorString();
        qDebug() << "======================================";
        emit errorOccurred("Network error: " + reply->errorString());
        return;
    }
    
    QByteArray data = reply->readAll();
    qDebug() << "  Response size:" << data.size() << "bytes";
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "  ERROR: Invalid JSON response";
        qDebug() << "  Raw response:" << data.left(500);
        qDebug() << "======================================";
        emit errorOccurred("Invalid JSON response");
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray results = root["results"].toArray();
    
    qDebug() << "  JSON parsed successfully";
    qDebug() << "  Results array size:" << results.size();
    
    QVector<DiscogsRelease> releases;
    
    for (int i = 0; i < results.size(); ++i) {
        const QJsonValue &value = results[i];
        QJsonObject obj = value.toObject();
        DiscogsRelease release = parseRelease(obj, false);
        qDebug() << "    [" << i << "]" << release.artist << "-" << release.title << "(" << release.year << ")";
        releases.append(release);
    }
    
    qDebug() << "DISCOGS API: Successfully parsed" << releases.size() << "releases";
    qDebug() << "======================================";
    emit searchResultsReady(releases);
}

void DiscogsClient::onReleaseFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    reply->deleteLater();
    
    qDebug() << "======================================";
    qDebug() << "DISCOGS API: Release details response received";
    qDebug() << "  HTTP Status Code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "  ERROR:" << reply->errorString();
        qDebug() << "======================================";
        emit errorOccurred("Network error: " + reply->errorString());
        return;
    }
    
    QByteArray data = reply->readAll();
    qDebug() << "  Response size:" << data.size() << "bytes";
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "  ERROR: Invalid JSON response";
        qDebug() << "  Raw response:" << data.left(500);
        qDebug() << "======================================";
        emit errorOccurred("Invalid JSON response");
        return;
    }
    
    QJsonObject root = doc.object();
    DiscogsRelease release = parseRelease(root, true);
    
    qDebug() << "  Release Details:";
    qDebug() << "    Title:" << release.title;
    qDebug() << "    Artist:" << release.artist;
    qDebug() << "    Year:" << release.year;
    qDebug() << "    Label:" << release.label;
    qDebug() << "    Country:" << release.country;
    qDebug() << "    Format:" << release.format;
    qDebug() << "    Genres:" << release.genres.join(", ");
    qDebug() << "    Styles:" << release.styles.join(", ");
    qDebug() << "    Tracks:" << release.tracks.size();
    qDebug() << "DISCOGS API: Release details parsed successfully";
    qDebug() << "======================================";
    
    emit releaseDetailsReady(release);
}

DiscogsRelease DiscogsClient::parseRelease(const QJsonObject &obj, bool detailed)
{
    DiscogsRelease release;
    
    release.id = QString::number(obj["id"].toInt());
    release.title = obj["title"].toString();
    release.year = QString::number(obj["year"].toInt());
    release.thumb = obj["thumb"].toString();
    
    // Parse artists
    if (obj.contains("artists")) {
        QJsonArray artists = obj["artists"].toArray();
        QStringList artistNames;
        for (const QJsonValue &artist : artists) {
            artistNames.append(artist.toObject()["name"].toString());
        }
        release.artist = artistNames.join(", ");
    } else if (obj.contains("artist")) {
        release.artist = obj["artist"].toString();
    }
    
    if (detailed) {
        // Detailed information
        release.country = obj["country"].toString();
        
        // Labels
        QJsonArray labels = obj["labels"].toArray();
        QStringList labelNames;
        for (const QJsonValue &label : labels) {
            labelNames.append(label.toObject()["name"].toString());
        }
        release.label = labelNames.join(", ");
        
        // Formats
        QJsonArray formats = obj["formats"].toArray();
        QStringList formatNames;
        for (const QJsonValue &format : formats) {
            formatNames.append(format.toObject()["name"].toString());
        }
        release.format = formatNames.join(", ");
        
        // Genres
        QJsonArray genres = obj["genres"].toArray();
        for (const QJsonValue &genre : genres) {
            release.genres.append(genre.toString());
        }
        
        // Styles
        QJsonArray styles = obj["styles"].toArray();
        for (const QJsonValue &style : styles) {
            release.styles.append(style.toString());
        }
        
        // Tracklist
        QJsonArray tracklist = obj["tracklist"].toArray();
        for (const QJsonValue &trackVal : tracklist) {
            QJsonObject trackObj = trackVal.toObject();
            DiscogsRelease::Track track;
            track.position = trackObj["position"].toString();
            track.title = trackObj["title"].toString();
            track.duration = trackObj["duration"].toString();
            release.tracks.append(track);
        }
    }
    
    return release;
}
