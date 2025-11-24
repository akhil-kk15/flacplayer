#include "playlist.h"
#include <QDebug>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <algorithm>

Playlist::Playlist(QObject *parent)
    : QObject(parent)
    , m_currentIndex(-1) //no audio file initially
    , m_shuffleEnabled(false)
    , m_name("Untitled Playlist") //default name for playlist
{
}

Playlist::~Playlist()
{
}

void Playlist::addFile(const QString &filePath)
{
    if (!filePath.isEmpty()) {
        m_files.append(filePath);
        
        // If this is the first file, set it as current
        if (m_files.count() == 1) {
            m_currentIndex = 0;
            emit currentIndexChanged(m_currentIndex);
        }
        
        emit playlistChanged();
        
        // Regenerate shuffle order if shuffle is enabled
        if (m_shuffleEnabled) {
            generateShuffleOrder();
        }
        
        qDebug() << "Added file to playlist:" << filePath;
    }
}

void Playlist::addFiles(const QStringList &filePaths)
{
    if (!filePaths.isEmpty()) {
        bool wasEmpty = m_files.isEmpty();
        m_files.append(filePaths);
        
        // If playlist was empty, set first file as current
        if (wasEmpty && !m_files.isEmpty()) {
            m_currentIndex = 0;
            emit currentIndexChanged(m_currentIndex);
        }
        
        emit playlistChanged();
        
        // Regenerate shuffle order if shuffle is enabled
        if (m_shuffleEnabled) {
            generateShuffleOrder();
        }
        
        qDebug() << "Added" << filePaths.count() << "files to playlist";
    }
}

void Playlist::removeFile(int index)
{
    if (index >= 0 && index < m_files.count()) {
        m_files.removeAt(index);
        
        // Adjust current index if needed
        if (m_currentIndex >= m_files.count()) {
            m_currentIndex = m_files.count() - 1;
            emit currentIndexChanged(m_currentIndex);
        } else if (m_currentIndex == index && m_currentIndex > 0) {
            m_currentIndex--;
            emit currentIndexChanged(m_currentIndex);
        }
        
        // Regenerate shuffle order if shuffle is enabled
        if (m_shuffleEnabled) {
            generateShuffleOrder();
        }
        
        emit playlistChanged();
    }
}

void Playlist::clear()
{
    m_files.clear();
    m_currentIndex = -1;
    m_shuffleOrder.clear();
    emit playlistChanged();
    emit currentIndexChanged(m_currentIndex);
}

bool Playlist::hasNext() const
{
    if (m_shuffleEnabled && !m_shuffleOrder.isEmpty()) {
        // Find current position in shuffle order
        for (int i = 0; i < m_shuffleOrder.count() - 1; ++i) {
            if (m_shuffleOrder[i] == m_currentIndex) {
                return true;
            }
        }
        return false;
    }
    return m_currentIndex >= 0 && m_currentIndex < m_files.count() - 1;
}

bool Playlist::hasPrevious() const
{
    if (m_shuffleEnabled && !m_shuffleOrder.isEmpty()) {
        // Find current position in shuffle order
        for (int i = 1; i < m_shuffleOrder.count(); ++i) {
            if (m_shuffleOrder[i] == m_currentIndex) {
                return true;
            }
        }
        return false;
    }
    return m_currentIndex > 0;
}

QString Playlist::next()
{
    if (hasNext()) {
        if (m_shuffleEnabled && !m_shuffleOrder.isEmpty()) {
            // Find current position in shuffle order
            int shufflePos = -1;
            for (int i = 0; i < m_shuffleOrder.count(); ++i) {
                if (m_shuffleOrder[i] == m_currentIndex) {
                    shufflePos = i;
                    break;
                }
            }
            if (shufflePos >= 0 && shufflePos < m_shuffleOrder.count() - 1) {
                m_currentIndex = m_shuffleOrder[shufflePos + 1];
            }
        } else {
            m_currentIndex++;
        }
        emit currentIndexChanged(m_currentIndex);
        return m_files[m_currentIndex];
    }
    return QString();
}

QString Playlist::previous()
{
    if (hasPrevious()) {
        if (m_shuffleEnabled && !m_shuffleOrder.isEmpty()) {
            // Find current position in shuffle order
            int shufflePos = -1;
            for (int i = 0; i < m_shuffleOrder.count(); ++i) {
                if (m_shuffleOrder[i] == m_currentIndex) {
                    shufflePos = i;
                    break;
                }
            }
            if (shufflePos > 0) {
                m_currentIndex = m_shuffleOrder[shufflePos - 1];
            }
        } else {
            m_currentIndex--;
        }
        emit currentIndexChanged(m_currentIndex);
        return m_files[m_currentIndex];
    }
    return QString();
}

QString Playlist::current() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_files.count()) {
        return m_files[m_currentIndex];
    }
    return QString();
}

int Playlist::count() const
{
    return m_files.count();
}

int Playlist::currentIndex() const
{
    return m_currentIndex;
}

QStringList Playlist::getFiles() const
{
    return m_files;
}

QString Playlist::getFileAt(int index) const
{
    if (index >= 0 && index < m_files.count()) {
        return m_files[index];
    }
    return QString();
}

void Playlist::setCurrentIndex(int index)
{
    if (index >= 0 && index < m_files.count() && index != m_currentIndex) {
        m_currentIndex = index;
        emit currentIndexChanged(m_currentIndex);
    }
}

void Playlist::setShuffle(bool enabled)
{
    if (m_shuffleEnabled != enabled) {
        m_shuffleEnabled = enabled;
        
        if (enabled) {
            generateShuffleOrder();
            qDebug() << "Shuffle enabled";
        } else {
            m_shuffleOrder.clear();
            qDebug() << "Shuffle disabled";
        }
        
        emit shuffleChanged(enabled);
    }
}

bool Playlist::isShuffled() const
{
    return m_shuffleEnabled;
}

void Playlist::generateShuffleOrder()
{
    m_shuffleOrder.clear();
    
    if (m_files.isEmpty()) {
        return;
    }
    
    // Create sequential order
    for (int i = 0; i < m_files.count(); ++i) {
        m_shuffleOrder.append(i);
    }
    
// Fisher-Yates shuffle algorithm for shuffling the songs,
//randomly picking an index from 0-i and swapping the i and j 

    for (int i = m_shuffleOrder.count() - 1; i > 0; --i) {
        int j = QRandomGenerator::global()->bounded(i + 1);
        m_shuffleOrder.swapItemsAt(i, j);
    }
    
    // If current track is set, move it to the front of shuffle order
    if (m_currentIndex >= 0) {
        for (int i = 0; i < m_shuffleOrder.count(); ++i) {
            if (m_shuffleOrder[i] == m_currentIndex) {
                if (i != 0) {
                    m_shuffleOrder.move(i, 0);
                }
                break;
            }
        }
    }
    
    qDebug() << "Generated shuffle order:" << m_shuffleOrder;
}

int Playlist::getShuffledIndex(int logicalIndex) const
{
    if (logicalIndex < 0 || logicalIndex >= m_shuffleOrder.count()) {
        return -1;
    }
    return m_shuffleOrder.indexOf(logicalIndex);
}

int Playlist::getLogicalIndex(int shuffledIndex) const
{
    if (shuffledIndex < 0 || shuffledIndex >= m_shuffleOrder.count()) {
        return -1;
    }
    return m_shuffleOrder[shuffledIndex];
}

bool Playlist::saveToFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    
    // Write playlist header
    out << "#EXTM3U\n";
    out << "#PLAYLIST:" << m_name << "\n";
    
    // Write each file with metadata
    for (const QString &filePath : m_files) {
        QFileInfo fileInfo(filePath);
        out << "#EXTINF:-1," << fileInfo.completeBaseName() << "\n";
        out << filePath << "\n";
    }
    
    file.close();
    qDebug() << "Playlist saved to:" << filePath;
    return true;
}

bool Playlist::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file for reading:" << filePath;
        return false;
    }
    
    QTextStream in(&file);
    QStringList newFiles;
    QString playlistName;
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        if (line.isEmpty()) {
            continue;
        }
        
        // Parse playlist name
        if (line.startsWith("#PLAYLIST:")) {
            playlistName = line.mid(10).trimmed();
            continue;
        }
        
        // Skip comments and metadata (except file paths)
        if (line.startsWith("#")) {
            continue;
        }
        
        // This should be a file path
        QFileInfo fileInfo(line);
        if (fileInfo.exists() && fileInfo.isFile()) {
            newFiles.append(line);
        } else {
            qDebug() << "File not found, skipping:" << line;
        }
    }
    
    file.close();
    
    if (newFiles.isEmpty()) {
        qDebug() << "No valid files found in playlist";
        return false;
    }
    
    // Clear current playlist and load new files
    clear();
    addFiles(newFiles);
    
    if (!playlistName.isEmpty()) {
        m_name = playlistName;
    } else {
        QFileInfo pFileInfo(filePath);
        m_name = pFileInfo.completeBaseName();
    }
    
    qDebug() << "Playlist loaded:" << m_name << "with" << newFiles.count() << "files";
    return true;
}

QString Playlist::getName() const
{
    return m_name;
}

void Playlist::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        qDebug() << "Playlist name changed to:" << m_name;
    }
}
