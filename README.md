# 🎵 FLAC Player

A modern, cross-platform media player specifically designed for lossless audio formats, built with Qt.

##  Overview

FLAC Player is a desktop application focused on providing the best playback experience for high-quality, lossless audio formats. The player combines simplicity with powerful metadata management capabilities, making it perfect for audiophiles and music enthusiasts who value audio quality.

##  Features

### ✅ Implemented Features

#### Audio Playback
-  **Multi-format Support**: FLAC, WAV, MP3, and other audio formats via Qt Multimedia
-  **Full Playback Controls**: Play, pause, skip (next/previous track)
-  **Volume Control**: Adjustable volume with mute toggle functionality
-  **Seek Control**: Progress bar with seek functionality
-  **Position & Duration Display**: Real-time tracking of playback position

#### Playlist Management
-  **Queue System**: Add multiple tracks to playback queue
-  **Shuffle Mode**: Randomize track order
-  **Repeat Modes**: Support for Off, Repeat All, and Repeat One
-  **Track Queue View**: Visual display of queued tracks
-  **Drag & Drop**: Add files by dragging them to the player (planned)

#### Metadata Management
-  **Custom FLAC Metadata Editor**: Direct binary-level FLAC file manipulation
-  **Read/Write Vorbis Comments**: Title, Artist, Album, Year, Genre, Track Number, etc.
-  **Album Art Management**: 
  - Extract embedded album artwork from FLAC files
  - Add/update album art (PNG, JPEG, BMP formats)
  - Remove album art
-  **Technical Info Display**: Sample rate, channels, bits per sample
-  **Metadata Editor Dialog**: User-friendly interface for editing track information

#### User Interface
-  **Qt-based Modern GUI**: Clean, intuitive interface
-  **Dynamic Gradient Background**: Mouse-responsive gradient effect
-  **Album Art Display**: Shows current track's album artwork
-  **Now Playing Info**: Displays track title, artist, and album
-  **Cross-platform Support**: Linux, Windows, macOS

### 🚧 Planned Features
-  **Library Management**: Database-driven music library organization
-  **Advanced Playlist Features**: Save/load playlists, playlist editing
-  **Audio Visualization**: Real-time frequency spectrum display
-  **Equalizer**: Customizable audio equalization
-  **Online Metadata**: Fetch metadata from MusicBrainz/Last.fm
-  **Keyboard Shortcuts**: Global hotkeys for playback control
-  **Themes**: Customizable UI themes

##  Technology Stack

- **UI Framework**: Qt 5/6 (Widgets, Multimedia)
- **Language**: C++17
- **Build System**: CMake
- **Audio Backend**: Qt Multimedia (QMediaPlayer, QAudioOutput)
- **Metadata Handling**: Custom binary FLAC parser (no external dependencies)
- **File I/O**: Qt Core (QFile, QByteArray)

##  Architecture

### Core Components

1. **MainWindow** (`mainwindow.h/cpp`)
   - Main application window and UI controller
   - Handles user interactions and playback control
   - Manages playlist queue and shuffle/repeat logic
   - Implements dynamic gradient background effect

2. **AudioManager** (`audiomanager.h/cpp`)
   - **MetadataEditor**: Custom FLAC metadata reader/writer
   - Binary-level FLAC file format manipulation
   - Vorbis Comment and Picture block handling
   - Album art extraction and embedding

3. **MetadataEditorDialog** (`audiomanager.h/cpp`)
   - Dialog interface for editing track metadata
   - Form fields for all standard tags
   - Album art preview and management
   - Save/Cancel functionality

### Key Features Implementation

#### Custom FLAC Metadata System
The application implements a custom FLAC metadata parser that:
- Reads FLAC file structure (header, metadata blocks, audio frames)
- Parses STREAMINFO, VORBIS_COMMENT, and PICTURE blocks
- Writes updated metadata while preserving audio data
- Handles big-endian/little-endian conversions
- No dependency on TagLib or libflac

#### Playlist & Queue Management
- In-memory track queue (QStringList)
- Shuffle using Qt's QRandomGenerator
- Three repeat modes (Off, All, One)
- Automatic next track on media end

##  Installation

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

##  Usage

### Basic Operations

1. **Opening Files**
   - Click `File → Open` or press Ctrl+O
   - Select one or more audio files (FLAC, WAV, MP3, etc.)
   - Files are added to the playback queue

2. **Playback Controls**
   - **Play/Pause**: Click the play button or press Space
   - **Next/Previous**: Navigate through tracks
   - **Seek**: Drag the progress slider
   - **Volume**: Adjust volume slider or click speaker icon to mute

3. **Playlist Management**
   - **Shuffle**: Click shuffle button to randomize track order
   - **Repeat**: Cycle through Off → All → One modes
   - **Queue**: Click "Track Queue" to view upcoming tracks

4. **Metadata Editing**
   - Click `File → Edit Metadata` while a track is playing
   - Edit fields: Title, Artist, Album, Year, Genre, Track Number
   - **Album Art**: 
     - Click "Load Image" to add/update artwork
     - Click "Remove" to delete embedded art
   - Click "Save" to write changes to file

### Keyboard Shortcuts
- `Ctrl+O`: Open files
- `Space`: Play/Pause (when implemented)
- For detailed Qt development information, see [meta.md](meta.md) for metadata system details.

##  Project Structure

```
flacplayer/
├── CMakeLists.txt              # Build configuration
├── main.cpp                    # Application entry point
├── mainwindow.h/cpp/ui         # Main window (UI, playback, queue)
├── audiomanager.h/cpp          # Metadata editor & FLAC parser
├── metadataeditor.ui           # Metadata editor dialog UI
├── flacplayer_en_GB.ts         # Translation file
├── resources.qrc               # Qt resources (icons, etc.)
├── assets/                     # Application assets
├── meta.md                     # Metadata implementation docs
├── README.md                   # This file
└── build/                      # Build output directory
```

### File Descriptions

- **mainwindow.***: Main application window with playback controls, queue management, and UI effects
- **audiomanager.***: FLAC metadata reading/writing, binary format parsing, album art handling
- **metadataeditor.ui**: Qt Designer form for metadata editing dialog
- **meta.md**: Detailed documentation of FLAC metadata implementation

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

### Code Organization

#### MainWindow Class
**Responsibilities:**
- UI event handling and user interactions
- Playlist queue management (add, shuffle, next/prev)
- Playback state management (play, pause, stop)
- Repeat mode logic (Off, All, One)
- Dynamic gradient rendering on mouse move
- Track information display updates

**Key Methods:**
- `on_actionOpen_triggered()`: File dialog and queue population
- `on_playPause_clicked()`: Toggle playback state
- `on_nextTrack_clicked()`: Advance to next track with repeat logic
- `updateTrackInfo()`: Update UI with current track metadata
- `paintEvent()`: Render dynamic gradient background

#### MetadataEditor Class
**Responsibilities:**
- Read/write FLAC file binary format
- Parse metadata blocks (STREAMINFO, VORBIS_COMMENT, PICTURE)
- Extract and embed album artwork
- Handle big-endian/little-endian conversions

**Key Methods:**
- `readMetadata()`: Parse FLAC file and extract all metadata
- `writeMetadata()`: Write updated metadata blocks to file
- `updateField()`: Update a single metadata field
- `updateAlbumArt()`: Add or replace embedded album art
- `readMetadataBlock()`: Parse individual metadata blocks

#### MetadataEditorDialog Class
**Responsibilities:**
- Provide UI for editing track metadata
- Load current track information
- Validate and save changes
- Album art preview and management

### Technical Details

#### FLAC Metadata Format
The custom parser handles:
- **File Header**: 4-byte "fLaC" signature (0x664C6143)
- **Metadata Blocks**: Variable-length blocks with headers
- **Vorbis Comments**: Key=Value pairs (little-endian lengths)
- **Picture Blocks**: Embedded images with MIME type (big-endian)

See [meta.md](meta.md) for detailed binary format documentation.

### Debugging

- The application outputs debug information to the terminal
- Use `qDebug()` for logging in Qt applications
- Qt Creator provides integrated debugging support
- Check `build/` directory for compilation artifacts

## Contributing

Contributions are welcome! Here's how you can help:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Contribution Ideas
- Add support for more audio formats (ALAC, APE, OGG)
- Implement music library with scanning functionality
- Add online metadata fetching (MusicBrainz API)
- Create audio visualization (spectrum analyzer)
- Improve UI/UX design and add themes
- Add playlist import/export (M3U, PLS)
- Implement global keyboard shortcuts
- Add audio effects (equalizer, normalization)
- Write unit tests for metadata parser
- Improve error handling and logging
- Add localization/translations

##  Project Status

**Current Version**: 0.2 (Alpha)

**Status**: Active Development

### What's Working
✅ Audio playback with full controls (play, pause, next, previous)  
✅ Volume and seek functionality  
✅ Queue-based playlist system  
✅ Shuffle and repeat modes  
✅ FLAC metadata reading and writing  
✅ Album art extraction and embedding  
✅ Metadata editor dialog  
✅ Track information display  

### Known Issues
⚠️ No playlist persistence (queue cleared on app close)  
⚠️ Limited error handling for corrupted files  
⚠️ UI could be more polished  

### Upcoming Milestones
- [ ] Persistent playlist storage
- [ ] Music library with database
- [ ] Search and filter functionality
- [ ] Audio visualization
- [ ] Global keyboard shortcuts
- [ ] Settings and preferences

##  License

This project is open source. Please add a LICENSE file to specify the terms of use.

##  Acknowledgments

- Built with [Qt Framework](https://www.qt.io/)
- Inspired by the need for a dedicated lossless audio player

##  Contact

For questions, suggestions, or issues, please open an issue on GitHub.

---

**Note**: This is a work in progress. Features and documentation will be updated as the project evolves.

## Quick build & run

If you already have Qt and CMake installed, this is the shortest way to build and run the app from the repository root:

```bash
# configure build directory
cmake -S . -B build

# build
cmake --build build -j4

# run the built binary
./build/flacplayer
```

If you use VS Code, use the local `.vscode/tasks.json` to run the configure/build tasks and `.vscode/launch.json` to debug (both are created locally and ignored by Git).

