#include "playlist.h"
#include <QDebug>
#include <QFileInfo>
#include <QRandomGenerator>
#include <algorithm>

Playlist::Playlist(QObject *parent)
    : QObject(parent)
    , m_currentIndex(-1)
    , m_shuffleEnabled(false)
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
    
    // Fisher-Yates shuffle algorithm
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
