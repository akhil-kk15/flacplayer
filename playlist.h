#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

class Playlist : public QObject
{
    Q_OBJECT

public:
    explicit Playlist(QObject *parent = nullptr);
    ~Playlist();

    // Playlist management
    void addFile(const QString &filePath);
    void addFiles(const QStringList &filePaths);
    void removeFile(int index);
    void clear();
    
    // Navigation
    bool hasNext() const;
    bool hasPrevious() const;
    QString next();
    QString previous();
    QString current() const;
    
    // Getters
    int count() const;
    int currentIndex() const;
    QStringList getFiles() const;
    QString getFileAt(int index) const;
    
    // Setters
    void setCurrentIndex(int index);
    
    // Shuffle
    void setShuffle(bool enabled);
    bool isShuffled() const;

signals:
    void playlistChanged();
    void currentIndexChanged(int index);
    void shuffleChanged(bool enabled);

private:
    void generateShuffleOrder();
    int getShuffledIndex(int logicalIndex) const;
    int getLogicalIndex(int shuffledIndex) const;
    
private:
    QStringList m_files;
    int m_currentIndex;
    bool m_shuffleEnabled;
    QVector<int> m_shuffleOrder;  // Maps logical index to shuffled index
};

#endif // PLAYLIST_H
