# Playlist Conversion Feature

## Overview
Added the ability to convert songs directly from the player's playlist to MP3 format.

## Features

### 1. Convert Current Track (Button)
- **Location**: Playlist panel buttons
- **Button**: "→ MP3"
- **Function**: Converts the currently playing/selected track to MP3
- **Tooltip**: "Convert current track to MP3 (Right-click playlist for more options)"

### 2. Context Menu (Right-click on Playlist)
Right-clicking on any song in the playlist opens a context menu with:

#### When clicking on a song:
- **Play**: Play the selected song
- **Convert to MP3**: Convert the selected song to MP3 (disabled if already MP3)

#### Additional options:
- **Convert All to MP3 (N files)**: Batch convert all non-MP3 files in the playlist
  - Shows count of files to be converted
  - Disabled if all files are already MP3

### 3. Batch Conversion
When using "Convert All to MP3":
1. Confirmation dialog shows the number of files to convert
2. Each file is converted sequentially using the ConversionDialog
3. Summary dialog displays:
   - Number of successful conversions
   - Number of failed conversions
   - List of failed files (if any)

## Implementation Details

### New Methods
- `onConvertSelectedSong()`: Converts the right-clicked song
- `onConvertAllPlaylist()`: Batch converts all non-MP3 files
- `showPlaylistContextMenu(const QPoint &pos)`: Displays context menu
- `convertFilesToMP3(const QStringList &files)`: Handles batch conversion
- `getNonMP3FilesFromPlaylist()`: Filters non-MP3 files from playlist

### User Experience
- MP3 files are automatically excluded from conversion options
- Context menu dynamically enables/disables options based on file types
- Clear feedback through message boxes for all operations
- Batch conversion includes progress tracking per file

## Usage

### Convert Single Song:
1. Right-click on any song in the playlist
2. Select "Convert to MP3"
3. Choose output location and settings
4. Click "Convert"

### Convert All Songs:
1. Right-click anywhere in the playlist
2. Select "Convert All to MP3 (N files)"
3. Confirm the batch operation
4. Each file will be converted sequentially
5. View the summary when complete

## Technical Notes
- Only non-MP3 files are included in conversion operations
- Existing ConversionDialog is reused for individual file conversions
- Batch operations show individual dialogs for each file (allows per-file settings)
- File format detection uses case-insensitive extension checking (.mp3, .MP3, etc.)
