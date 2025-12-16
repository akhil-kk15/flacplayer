
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "metadataeditor.h"
#include <QMessageBox>
#include <QStatusBar>
#include <QFileDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QElapsedTimer>
#include <QDialog>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QMediaMetaData>
#include <QPixmap>
#include <QImage>
#include <QRegularExpression>


// * Initializes the UI, sets up button icons, configures media playback components,
 // connects signals/slots, and enables mouse tracking for gradient effect.
 
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // Configure window properties
    setWindowTitle("Flac Player v2.0");
    setFixedSize(970, 650);  // Fixed size, non-resizable
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);  // Disable maximize button
    
    statusBar()->setSizeGripEnabled(false);  // Disable resize grip in status bar
    
    // Initialize media playback components (Qt6 requires separate audio output)
    MPlayer = new QMediaPlayer();
    audioOutput = new QAudioOutput();
    MPlayer->setAudioOutput(audioOutput);


    // Set button icons from resources (clear text to show icons only)
    ui->playPause->setIcon(QIcon(":/icons/assets/play.png"));
    ui->playPause->setIconSize(QSize(40, 40));
    ui->playPause->setText("");  // Clear button text
    
    ui->nextTrack->setIcon(QIcon(":/icons/assets/next.png"));
    ui->nextTrack->setIconSize(QSize(40, 40));
    ui->nextTrack->setText("");
    
    ui->previousTrack->setIcon(QIcon(":/icons/assets/previous.png"));
    ui->previousTrack->setIconSize(QSize(40, 40));
    ui->previousTrack->setText("");
    
    ui->Shuffle->setIcon(QIcon(":/icons/assets/shuffle.png"));
    ui->Shuffle->setIconSize(QSize(40, 40));
    ui->Shuffle->setText("");
    
    ui->trackQueue->setIcon(QIcon(":/icons/assets/playlist.png"));
    ui->trackQueue->setIconSize(QSize(40, 40));
    ui->trackQueue->setText("");
    
    ui->muteButton->setIcon(QIcon(":/icons/assets/unmuted.png"));
    ui->muteButton->setIconSize(QSize(30, 30));
    ui->muteButton->setText("");

    //volume slider initial setup
    ui->volumeSlider->setRange(0, 100);
    ui->volumeSlider->setValue(30);

    // Connect media player signals for position and duration changes
    connect(MPlayer, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(MPlayer, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(MPlayer, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::onMediaStatusChanged);
    connect(MPlayer, &QMediaPlayer::metaDataChanged, this, &MainWindow::displayMetadata);

    // Install event filter on next/previous buttons to detect hold vs click
    ui->nextTrack->installEventFilter(this);
    ui->previousTrack->installEventFilter(this);

    // Connect button signals
    connect(ui->muteButton, &QPushButton::clicked, this, &MainWindow::onMuteToggle);

    // Set initial UI state
    ui->labelFileName->setText("Add files through the menu to begin playback");
    ui->trackName_2->setText("No next track");
    ui->seekSlider->setEnabled(false);
    
    // Enable mouse tracking for gradient effect on all widgets
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    
    if (centralWidget()) {
        centralWidget()->setMouseTracking(true);
        centralWidget()->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        
        // Enable mouse tracking on all child widgets to prevent stuttering
        QList<QWidget*> allWidgets = centralWidget()->findChildren<QWidget*>();
        for (QWidget* widget : allWidgets) {
            widget->setMouseTracking(true);
            widget->installEventFilter(this);
        }
    }
    
    // Initialize frame timer for performance tracking
    frameTimer.start();
    lastUpdateTime = 0;
    
    statusBar()->showMessage("Ready - Click buttons to test UI", 3000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//file management slots 
void MainWindow::on_actionOpen_triggered()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this, 
        tr("Open Audio Files"), 
        "", 
        tr("FLAC Files (*.flac);;All Audio Files (*.flac *.m4a *.wav);;All Files (*)")
    );

    if (fileNames.isEmpty()) {
        return; // User cancelled
    }

    // Add files to playlist
    for (const QString &fileName : fileNames) {
        playlist.append(fileName);
        // qDebug() << "[MainWindow] Added to playlist:" << fileName;
    }
    
    // If this is the first file, load it
    if (currentTrackIndex == -1) {
        currentTrackIndex = 0;
        loadTrack(currentTrackIndex);
    }
    
    updateNextTrackDisplay();
    statusBar()->showMessage(QString("Added %1 file(s) to queue").arg(fileNames.size()), 2000);
}


/**
 * @brief Show track queue dialog with list of all tracks
 * 
 * Displays a modal dialog showing all tracks in the playlist with the current track highlighted.
 * Users can double-click a track to jump to it.
 */
void MainWindow::on_trackQueue_clicked()
{
    if (playlist.isEmpty()) {
        QMessageBox::information(this, "Queue", "No tracks in queue.\n\nUse File > Open to add tracks.");
        return;
    }
    
    // Create modal dialog
    QDialog *queueDialog = new QDialog(this);
    queueDialog->setWindowTitle("Track Queue");
    queueDialog->resize(500, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(queueDialog);
    
    // Add list widget showing all tracks
    QListWidget *trackList = new QListWidget(queueDialog);
    for (int i = 0; i < playlist.size(); ++i) {
        QFileInfo fileInfo(playlist[i]);
        QListWidgetItem *item = new QListWidgetItem(
            QString("%1. %2").arg(i + 1).arg(fileInfo.fileName())
        );
        
        // Highlight current track
        if (i == currentTrackIndex) {
            item->setBackground(QColor(100, 150, 255, 100));
            item->setForeground(Qt::white);
        }
        
        trackList->addItem(item);
    }
    
    // Double-click to play that track
    connect(trackList, &QListWidget::itemDoubleClicked, 
            [this, trackList, queueDialog](QListWidgetItem *item) {
        int index = trackList->row(item);
        loadTrack(index);
        MPlayer->play();
        isPlaying = true;
        ui->playPause->setIcon(QIcon(":/icons/assets/pause.png"));
        queueDialog->accept();
    });
    
    layout->addWidget(new QLabel(
        QString("Total tracks: %1 | Current: %2")
        .arg(playlist.size())
        .arg(currentTrackIndex + 1)
    ));
    layout->addWidget(trackList);
    
    // Add close button
    QPushButton *closeButton = new QPushButton("Close", queueDialog);
    connect(closeButton, &QPushButton::clicked, queueDialog, &QDialog::accept);
    layout->addWidget(closeButton);
    
    queueDialog->exec();
    delete queueDialog;
}

/**
 * @brief Open metadata editor for current track
 * 
 * Opens a dialog that allows editing of the current track's metadata
 * including title, artist, album, year, and album art.
 */
void MainWindow::on_actionEditMetadata_triggered()
{
    // qDebug() << "[MainWindow] Edit Metadata triggered";
    if (currentTrackIndex < 0 || currentTrackIndex >= playlist.size()) {
        // qDebug() << "[MainWindow] No track loaded";
        QMessageBox::information(this, "Edit Metadata", 
            "No track currently loaded.\n\nLoad a track first, then use Tools > Edit Metadata.");
        return;
    }
    
    QString currentFile = playlist[currentTrackIndex];
    // qDebug() << "[MainWindow] Current file:" << currentFile;
    
    // Check if it's a FLAC file
    if (!currentFile.toLower().endsWith(".flac")) {
        QMessageBox::warning(this, "Edit Metadata", 
            "Metadata editing is currently only supported for FLAC files.\n\nCurrent file: " + 
            QFileInfo(currentFile).fileName());
        return;
    }
    
    // Open metadata editor dialog
    // qDebug() << "[MainWindow] Creating MetadataEditorDialog...";
    MetadataEditorDialog dialog(currentFile, this);
    // qDebug() << "[MainWindow] Dialog created, executing...";
    if (dialog.exec() == QDialog::Accepted) {
        // qDebug() << "[MainWindow] Dialog accepted, refreshing metadata...";
        // Reload metadata display after editing
        statusBar()->showMessage("Metadata updated - reloading track info...", 2000);
        
        // Force metadata refresh by reloading the track
        qint64 currentPosition = MPlayer->position();
        bool wasPlaying = isPlaying;
        
        loadTrack(currentTrackIndex);
        MPlayer->setPosition(currentPosition);
        
        if (wasPlaying) {
            MPlayer->play();
        }
    }
}

//song loading and metadata display
void MainWindow::loadTrack(int index)
{
    if (index >= 0 && index < playlist.size()) {
        currentTrackIndex = index;
        QString fileName = playlist[index];
        MPlayer->setSource(QUrl::fromLocalFile(fileName));
        
        QFileInfo fileinfo(fileName);
        ui->labelFileName->setText(fileinfo.fileName());
        ui->seekSlider->setEnabled(true);
        ui->seekSlider->setValue(0);
        
        updateNextTrackDisplay();
    }
}

//next song  display update, if there are no song left in the queue it will show no next track
void MainWindow::updateNextTrackDisplay()
{
    int nextIndex = currentTrackIndex + 1;
    if (nextIndex < playlist.size()) {
        QFileInfo nextFile(playlist[nextIndex]);
        ui->trackName_2->setText(QString("Next: %1").arg(nextFile.fileName()));
    } else {
        ui->trackName_2->setText("No next track");
    }
}



//metadata extraction and display using QMediaMetaData
void MainWindow::displayMetadata()
{
    // Get metadata from the media player
    QMediaMetaData metadata = MPlayer->metaData();
    
    // Extract and display track title
    QString trackTitle = "Unknown Track";
    if (metadata.value(QMediaMetaData::Title).isValid()) {
        trackTitle = metadata.stringValue(QMediaMetaData::Title);
    } else if (currentTrackIndex >= 0 && currentTrackIndex < playlist.size()) {
        // Fallback to filename without extension
        QFileInfo fileInfo(playlist[currentTrackIndex]);
        trackTitle = fileInfo.completeBaseName();
    }
    ui->trackName->setText(trackTitle);
    
    // Extract and display album artist
    QString artist = "Unknown Artist";
    if (metadata.value(QMediaMetaData::AlbumArtist).isValid()) {
        artist = metadata.stringValue(QMediaMetaData::AlbumArtist);
    } else if (metadata.value(QMediaMetaData::ContributingArtist).isValid()) {
        artist = metadata.stringValue(QMediaMetaData::ContributingArtist);
    }
    ui->albumArtist->setText(artist);
    
    // Extract and display album name
    QString album = "Unknown Album";
    if (metadata.value(QMediaMetaData::AlbumTitle).isValid()) {
        album = metadata.stringValue(QMediaMetaData::AlbumTitle);
    }
    ui->albumName->setText(album);
    
    // Extract and display year
    QString year = "----";
    if (metadata.value(QMediaMetaData::Date).isValid()) {
        QVariant dateVariant = metadata.value(QMediaMetaData::Date);
        // Try to convert to year
        if (dateVariant.typeId() == QMetaType::QDate) {
            year = QString::number(dateVariant.toDate().year());
        } else if (dateVariant.typeId() == QMetaType::QDateTime) {
            year = QString::number(dateVariant.toDateTime().date().year());
        } else {
            QString dateStr = dateVariant.toString();
            // Try to extract year from string (first 4 digits)
            QRegularExpression yearRegex("(\\d{4})");
            QRegularExpressionMatch match = yearRegex.match(dateStr);
            if (match.hasMatch()) {
                year = match.captured(1);
            }
        }
    }
    ui->albumYear->setText(year);
    
    // Extract and display album art
    if (metadata.value(QMediaMetaData::ThumbnailImage).isValid()) {
        QImage coverImage = metadata.value(QMediaMetaData::ThumbnailImage).value<QImage>();
        if (!coverImage.isNull()) {
            // Scale image to fit the label while maintaining aspect ratio
            QPixmap coverPixmap = QPixmap::fromImage(coverImage);
            QSize labelSize = ui->albumArtLabel->size();  // Assuming label_2 is for album art
            QPixmap scaledPixmap = coverPixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ui->albumArtLabel->setPixmap(scaledPixmap);
            ui->albumArtLabel->setAlignment(Qt::AlignCenter);
        } else {
            // No album art available - show placeholder text
            ui->albumArtLabel->clear();
            ui->albumArtLabel->setText("No Album Art");
            ui->albumArtLabel->setAlignment(Qt::AlignCenter);
        }
    } else if (metadata.value(QMediaMetaData::CoverArtImage).isValid()) {
        // Try alternative metadata key for cover art
        QImage coverImage = metadata.value(QMediaMetaData::CoverArtImage).value<QImage>();
        if (!coverImage.isNull()) {
            QPixmap coverPixmap = QPixmap::fromImage(coverImage);
            QSize labelSize = ui->albumArtLabel->size();
            QPixmap scaledPixmap = coverPixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            ui->albumArtLabel->setPixmap(scaledPixmap);
            ui->albumArtLabel->setAlignment(Qt::AlignCenter);
        } else {
            ui->albumArtLabel->clear();
            ui->albumArtLabel->setText("No Album Art");
            ui->albumArtLabel->setAlignment(Qt::AlignCenter);
        }
    } else {
        // No album art available
        ui->albumArtLabel->clear();
        ui->albumArtLabel->setText("No Album Art");
        ui->albumArtLabel->setAlignment(Qt::AlignCenter);
    }
    
    // Update status bar with additional info
    QString statusInfo = QString("Loaded: %1").arg(trackTitle);
    if (metadata.value(QMediaMetaData::AudioBitRate).isValid()) {
        int bitrate = metadata.value(QMediaMetaData::AudioBitRate).toInt();
        statusInfo += QString(" | %1 kbps").arg(bitrate / 1000);
    }
    statusBar()->showMessage(statusInfo, 3000);
}

//playback control slots
//main play/pause button handler 
void MainWindow::on_playPause_clicked()
{
    if (isPlaying) {
        MPlayer->pause();
        ui->playPause->setIcon(QIcon(":/icons/assets/play.png"));
        statusBar()->showMessage("Playback paused", 2000);
        isPlaying = false;
    } else {
        MPlayer->play();
        ui->playPause->setIcon(QIcon(":/icons/assets/pause.png"));
        statusBar()->showMessage("Playback started", 2000);
        isPlaying = true;
    }
}

//button click vs hold behavior implementation, click for next/previous track, hold for seeking
void MainWindow::on_nextTrack_clicked()
{
    if (isButtonHeld) {
        // Was held - seeking already handled
        isButtonHeld = false;
        return;
    }
    
    // Quick click - go to next track
    if (currentTrackIndex + 1 < playlist.size()) {
        bool wasPlaying = isPlaying;  // Save current playing state
        loadTrack(currentTrackIndex + 1);
        if (wasPlaying) {
            MPlayer->play();
            isPlaying = true;
            ui->playPause->setIcon(QIcon(":/icons/assets/pause.png"));
        }
        statusBar()->showMessage("Next track", 2000);
    } else {
        statusBar()->showMessage("End of playlist", 2000);
    }
}

//same as above but for previous track button
void MainWindow::on_previousTrack_clicked()
{
    if (isButtonHeld) {
        // Was held - seeking already handled
        isButtonHeld = false;
        return;
    }
    
    // Quick click - go to previous track
    if (currentTrackIndex > 0) {
        bool wasPlaying = isPlaying;  // Save current playing state
        loadTrack(currentTrackIndex - 1);
        if (wasPlaying) {
            MPlayer->play();
            isPlaying = true;
            ui->playPause->setIcon(QIcon(":/icons/assets/pause.png"));
        }
        statusBar()->showMessage("Previous track", 2000);
    } else {
        // Already at first track, restart current track
        MPlayer->setPosition(0);
        statusBar()->showMessage("Restarting track", 2000);
    }
}

//this is for shuffle, currently a placeholder 
//to-do try implement shuffle functionality using fisher-yates algorithm
void MainWindow::on_Shuffle_clicked()
{
    // TODO: Implement shuffle functionality
}

//seek forward
void MainWindow::seekForward()
{
    qint64 currentPos = MPlayer->position();
    qint64 newPos = qMin(mediaDuration, currentPos + 10000);
    MPlayer->setPosition(newPos);
    statusBar()->showMessage("Seeking forward", 1000);
}

//same for backwards
void MainWindow::seekBackward()
{
    qint64 currentPos = MPlayer->position();
    qint64 newPos = qMax(qint64(0), currentPos - 10000);
    MPlayer->setPosition(newPos);
    statusBar()->showMessage("Seeking backward", 1000);
}

//audio output controls
//mute state toggle button handler
void MainWindow::onMuteToggle()
{
    isMuted = !isMuted;
    if (isMuted) {
        ui->muteButton->setIcon(QIcon(":/icons/assets/mute.png"));
        ui->volumeSlider->setValue(0);
        statusBar()->showMessage("Volume muted", 2000);
    } else {
        ui->muteButton->setIcon(QIcon(":/icons/assets/unmuted.png"));
        ui->volumeSlider->setValue(70);
        statusBar()->showMessage("Volume unmuted", 2000);
    }
}

//volume control slider handler
void MainWindow::on_volumeSlider_valueChanged(int value)
{
    audioOutput->setVolume(value / 100.0);
}

//skimming through track using seek slider
void MainWindow::on_seekSlider_valueChanged(int value)
{
    if (!isSeeking && mediaDuration > 0) {
        qint64 position = (value * mediaDuration) / 100;
        MPlayer->setPosition(position);
    }
}

//updates for media player signals
//time and slider updates during playback
void MainWindow::onPositionChanged(qint64 position)
{
    if (!ui->seekSlider->isSliderDown() && mediaDuration > 0) {
        isSeeking = true;
        int sliderPosition = (position * 100) / mediaDuration;
        ui->seekSlider->setValue(sliderPosition);
        isSeeking = false;
    }
    
    // Update timestamp label
    qint64 currentSeconds = position / 1000;
    qint64 totalSeconds = mediaDuration / 1000;
    
    QString timeText = QString("%1:%2 / %3:%4")
        .arg(currentSeconds / 60, 2, 10, QChar('0'))
        .arg(currentSeconds % 60, 2, 10, QChar('0'))
        .arg(totalSeconds / 60, 2, 10, QChar('0'))
        .arg(totalSeconds % 60, 2, 10, QChar('0'));
    
    ui->timeStamp->setText(timeText);
}

//track duration loaded handler
void MainWindow::onDurationChanged(qint64 duration)
{
    mediaDuration = duration;
    ui->seekSlider->setEnabled(duration > 0);
}

//auto play next after current
void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        // Current track ended, auto-play next track
        if (currentTrackIndex + 1 < playlist.size()) {
            loadTrack(currentTrackIndex + 1);
            MPlayer->play();
            isPlaying = true;
            ui->playPause->setIcon(QIcon(":/icons/assets/pause.png"));
            statusBar()->showMessage("Playing next track", 2000);
        } else {
            // End of playlist
            isPlaying = false;
            ui->playPause->setIcon(QIcon(":/icons/assets/play.png"));
            statusBar()->showMessage("End of playlist", 2000);
        }
    }
}



 //Tracks mouse position and triggers partial repaints for gradient rendering.
 // Optimized for 60fps with frame throttling and minimal repaint regions.
 
 // DO NOT MODIFY unless necessary - already optimized for performance.
 
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    qint64 currentTime = frameTimer.elapsed();
    qint64 elapsed = currentTime - lastUpdateTime;
    
    // Throttle updates to maintain 60fps (16ms per frame)
    // Allow updates if enough time has passed OR if mouse moved significantly
    QPoint newPos = centralWidget() ? centralWidget()->mapFrom(this, event->pos()) : event->pos();
    int distance = QPoint(newPos - mousePos).manhattanLength();
    
    // Lower threshold for smoother movement, especially over buttons
    if (elapsed < TARGET_FRAME_TIME - 2 && distance < 2) {
        // Skip this update if we're updating too frequently and mouse barely moved
        QMainWindow::mouseMoveEvent(event);
        return;
    }
    
    lastUpdateTime = currentTime;
    
    // Map the event position to central widget coordinates if available
    lastMousePos = mousePos;
    mousePos = newPos;
    
    // Only update the region affected by the gradient (old and new positions)
    if (!lastMousePos.isNull()) {
        QPoint globalMousePos = centralWidget() ? centralWidget()->mapTo(this, mousePos) : mousePos;
        QPoint globalLastPos = centralWidget() ? centralWidget()->mapTo(this, lastMousePos) : lastMousePos;
        
        // Update both old and new gradient areas (with some margin)
        int radius = 110; // Slightly larger than gradient radius for smooth transition
        update(QRect(globalLastPos.x() - radius, globalLastPos.y() - radius, radius * 2, radius * 2));
        update(QRect(globalMousePos.x() - radius, globalMousePos.y() - radius, radius * 2, radius * 2));
    } else {
        update();
    }
    
    QMainWindow::mouseMoveEvent(event);
}


//mouse move event routing and button hold detection, gradient effect handling. click vs hold for next/previous buttons

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Route mouse move events to gradient handler
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QWidget *widget = qobject_cast<QWidget*>(obj);
        
        // Skip next/previous buttons (they have their own event handling)
        if (widget && obj != ui->nextTrack && obj != ui->previousTrack) {
            QPoint globalPos = widget->mapTo(this, mouseEvent->pos());
            QMouseEvent mappedEvent(QEvent::MouseMove, globalPos, mouseEvent->button(), 
                                   mouseEvent->buttons(), mouseEvent->modifiers());
            mouseMoveEvent(&mappedEvent);
        }
    }
    
    // Handle button press/hold detection for next/previous buttons
    if (obj == ui->nextTrack || obj == ui->previousTrack) {
        if (event->type() == QEvent::MouseButtonPress) {
            buttonPressTimer.start();
            isButtonHeld = false;
            
            // Check after 500ms if button is still held
            QTimer::singleShot(500, this, [this, obj]() {
                if (buttonPressTimer.isValid() && buttonPressTimer.elapsed() >= 500) {
                    isButtonHeld = true;
                    // Start seeking
                    if (obj == ui->nextTrack) {
                        seekForward();
                    } else if (obj == ui->previousTrack) {
                        seekBackward();
                    }
                }
            });
        } else if (event->type() == QEvent::MouseButtonRelease) {
            buttonPressTimer.invalidate();
        }
    }
    
    return QMainWindow::eventFilter(obj, event);
}


 //drawing the gradient effect on hover , dont modify unless necessary.
 //Already optimized for performance.
void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    
    // Draw the gradient on the central widget
    if (centralWidget() && !mousePos.isNull()) {
        QPainter painter(this);
        
        // Optimize painter settings for performance
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        
        // Clip to the update region for better performance
        painter.setClipRegion(event->region());
        
        // Map mouse position to this widget's coordinates
        QPoint globalMousePos = centralWidget()->mapTo(this, mousePos);
        
        // Create radial gradient centered on mouse position
        QRadialGradient gradient(globalMousePos, 100);
        gradient.setColorAt(0, QColor(255, 255, 255, 60));  // White at center with transparency
        gradient.setColorAt(0.7, QColor(255, 255, 255, 20)); // Softer falloff
        gradient.setColorAt(1, QColor(255, 255, 255, 0));   // Fully transparent at edge
        
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(globalMousePos, 100, 100);
    }
}


// void MainWindow::on_repeatToggle_clicked()
// {

// }

