#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QString>
#include <stdexcept>
 //dynamic array based playlist implementation ,  when full it should double its capacity
class Playlist {
public:
  //empty constructor for an empty playlist 
    Playlist() : m_data(nullptr), m_size(0), m_capacity(0) {}
    //distructor to free allocated memory
    ~Playlist() {
        delete[] m_data;
    }
    //copy constructor
    Playlist(const Playlist& other) : m_data(nullptr), m_size(0), m_capacity(0) {
        if (other.m_size > 0) {
            reserve(other.m_size);
            for (int i = 0; i < other.m_size; ++i) {
                m_data[i] = other.m_data[i];
            }
            m_size = other.m_size;
        }
    }
    
    // Copy assignment operator
   
    Playlist& operator=(const Playlist& other) {
        if (this != &other) {
            delete[] m_data;
            m_data = nullptr;
            m_size = 0;
            m_capacity = 0;
            
            if (other.m_size > 0) {
                reserve(other.m_size);
                for (int i = 0; i < other.m_size; ++i) {
                    m_data[i] = other.m_data[i];
                }
                m_size = other.m_size;
            }
        }
        return *this;
    }
    
  // add a new file path to the playlist
    void append(const QString& path) {
        if (m_size >= m_capacity) {
            // Double capacity (or start with 4 if empty)
            int newCapacity = (m_capacity == 0) ? 4 : m_capacity * 2;
            reserve(newCapacity);
        }
        m_data[m_size++] = path;
    }
    
  /// Check if playlist is empty
    bool isEmpty() const {
        return m_size == 0;
    }
    
/// Get current size of playlist
    int size() const {
        return m_size;
    }
    
     //accessing elements by index with bounds checking
    const QString& operator[](int index) const {
        if (index < 0 || index >= m_size) {
            throw std::out_of_range("Playlist index out of range");
        }
        return m_data[index];
    }
//non-const version of above
     QString& operator[](int index) {
        if (index < 0 || index >= m_size) {
            throw std::out_of_range("Playlist index out of range");
        }
        return m_data[index];
    }
    
    //path search function
    int indexOf(const QString& path) const {
        for (int i = 0; i < m_size; ++i) {
            if (m_data[i] == path) {
                return i;
            }
        }
        return -1;
    }
    

    //iterator support for std::shuffle
    QString* begin() {
        return m_data;
    }
    
    //end iterator pointitng to one past the last element, clearing the playlist
    QString* end() {
        return m_data + m_size;
    }
    void clear() {
        m_size = 0;
    }
    
private:
    //reserve function to allocate more memory
    void reserve(int newCapacity) {
        if (newCapacity <= m_capacity) {
            return;
        }
        
        QString* newData = new QString[newCapacity];
        
        // Copy existing elements
        for (int i = 0; i < m_size; ++i) {
            newData[i] = m_data[i];
        }
        
        delete[] m_data;
        m_data = newData;
        m_capacity = newCapacity;
    }
    
    QString* m_data;       //Dynamic array of file paths
    int m_size;            //Current number of elements
    int m_capacity;        // Allocated capacity
};

#endif // PLAYLIST_H
