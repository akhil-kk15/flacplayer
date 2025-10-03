# 🎵 FLAC Player

A modern, cross-platform media player specifically designed for lossless audio formats, built with Qt.

## 📋 Overview

FLAC Player is a desktop application focused on providing the best playback experience for high-quality, lossless audio formats. The player aims to combine simplicity with powerful metadata management capabilities, making it perfect for audiophiles and music enthusiasts who value audio quality.

## ✨ Features (Planned & Current)

### Current Features
- ✅ Qt-based graphical user interface
- ✅ Basic media player framework
- ✅ Cross-platform support (Linux, Windows, macOS)
- ✅ Modern C++17 codebase

### Planned Features
- 🎯 **Lossless Format Support**: FLAC, ALAC, WAV, APE, and other high-quality audio formats
- 🎨 **Metadata Management**: 
  - Automatic metadata fetching from online databases
  - Album art download and management
  - Manual metadata and album art editing
- 📚 **Library Management**: Organize and browse your music collection
- 🎼 **Playlist Support**: Create and manage custom playlists
- 🎛️ **Audio Controls**: Play, pause, skip, volume, and seek controls
- 📊 **Audio Visualization**: Real-time visualization of audio playback

## 🛠️ Technology Stack

- **UI Framework**: Qt 5/6 (Widgets, Multimedia)
- **Language**: C++17
- **Build System**: CMake
- **Planned Libraries**: Boost (for additional functionality)

## 📦 Installation

### Prerequisites

- Qt 5 or Qt 6 (with Widgets and Multimedia modules)
- CMake 3.16 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)

### Building from Source

1. **Clone the repository**
   ```bash
   git clone https://github.com/akhil-kk15/flacplayer.git
   cd flacplayer
   ```

2. **Create build directory**
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake**
   ```bash
   cmake ..
   ```

4. **Build the project**
   ```bash
   make -j$(nproc)
   # On Windows with Visual Studio:
   # cmake --build . --config Release
   ```

5. **Run the application**
   ```bash
   ./flacplayer
   # On Windows:
   # .\Release\flacplayer.exe
   ```

## 🚀 Usage

The application is currently in early development. Upon launching, you will see:
- A main window with basic UI controls
- Test buttons to verify the Qt framework setup
- Debug information to help understand the application structure

For detailed Qt development information, see [QT_LEARNING_GUIDE.md](QT_LEARNING_GUIDE.md).

## 🏗️ Project Structure

```
flacplayer/
├── CMakeLists.txt           # Build configuration
├── main.cpp                 # Application entry point
├── mainwindow.h/cpp         # Main window implementation
├── mainwindow.ui            # Qt Designer UI file
├── flacplayer_en_GB.ts      # Translation file
├── QT_LEARNING_GUIDE.md     # Qt development guide
└── README.md                # This file
```

## 🔧 Development

### Development Environment Setup

1. **Install Qt Creator** (recommended for GUI development)
   - Download from [qt.io](https://www.qt.io/download)
   - Or use your preferred IDE with Qt support

2. **Install Qt Libraries**
   - **Ubuntu/Debian**: `sudo apt install qt5-default qtmultimedia5-dev`
   - **Fedora**: `sudo dnf install qt5-qtbase-devel qt5-qtmultimedia-devel`
   - **macOS**: `brew install qt@5`
   - **Windows**: Use the Qt installer from qt.io

3. **Build and test your changes**
   ```bash
   cd build
   make -j$(nproc)
   ./flacplayer
   ```

### Debugging

- The application outputs debug information to the terminal
- Use `qDebug()` for logging in Qt applications
- Qt Creator provides integrated debugging support

See [QT_LEARNING_GUIDE.md](QT_LEARNING_GUIDE.md) for comprehensive Qt development tips.

## 🤝 Contributing

Contributions are welcome! Here's how you can help:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Contribution Ideas
- Implement FLAC file format support
- Add metadata fetching from MusicBrainz or similar services
- Create album art management features
- Improve UI/UX design
- Add playlist functionality
- Write tests
- Improve documentation

## 📝 Project Status

**Current Version**: 0.1 (Early Development)

This project is in its **early development phase**. The current focus is on establishing the basic Qt framework and UI structure. Core audio playback and metadata features are planned for upcoming releases.

## 📄 License

This project is open source. Please add a LICENSE file to specify the terms of use.

## 🙏 Acknowledgments

- Built with [Qt Framework](https://www.qt.io/)
- Inspired by the need for a dedicated lossless audio player

## 📞 Contact

For questions, suggestions, or issues, please open an issue on GitHub.

---

**Note**: This is a work in progress. Features and documentation will be updated as the project evolves.
