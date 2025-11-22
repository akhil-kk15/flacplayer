#ifndef DISCOGSCLIENT_H
#define DISCOGSCLIENT_H

//

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

struct DiscogsRelease
{
    QString id;
    QString title;
    QString artist;
    QString year;
    QString label;
    QString country;
    QString format;
    QString thumb;
    
    QStringList genres;
    QStringList styles;
    
    struct Track {
        QString position;
        QString title;
        QString duration;
    };
    QVector<Track> tracks;
};

class DiscogsClient : public QObject
{
    Q_OBJECT

public:
    explicit DiscogsClient(QObject *parent = nullptr);
    ~DiscogsClient();
    
    // Search methods
    void searchRelease(const QString &artist, const QString &album);
    void searchByBarcode(const QString &barcode);
    void getRelease(const QString &releaseId);
    
    // Configuration
    void setUserAgent(const QString &userAgent);
    void setApiToken(const QString &token);

signals:
    void searchResultsReady(const QVector<DiscogsRelease> &releases);
    void releaseDetailsReady(const DiscogsRelease &release);
    void errorOccurred(const QString &error);

private slots:
    void onSearchFinished();
    void onReleaseFinished();

private:
    void makeRequest(const QString &url, bool isReleaseRequest = false);
    DiscogsRelease parseRelease(const QJsonObject &obj, bool detailed = false);
    
private:
    QNetworkAccessManager *m_networkManager;
    QString m_userAgent;
    QString m_apiToken;
    QString m_baseUrl;
};

#endif // DISCOGSCLIENT_H
;