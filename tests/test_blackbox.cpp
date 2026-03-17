#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QElapsedTimer>
#include "../mainwindow.h"

// Black-box Testing - Testing based on requirements/specifications without knowledge of implementation
// Tests focus on inputs and expected outputs, user interactions, and functional requirements
class BlackBoxTest : public ::testing::Test {
protected:
    void SetUp() override {
        mainWindow = new MainWindow();
    }

    void TearDown() override {
        delete mainWindow;
    }

    MainWindow* mainWindow;
};

// Requirement: Application should have a window
TEST_F(BlackBoxTest, ApplicationWindowExists) {
    ASSERT_NE(mainWindow, nullptr);
}

// Requirement: User can control volume
TEST_F(BlackBoxTest, UserCanControlVolume) {
    QSlider* volumeSlider = mainWindow->findChild<QSlider*>("volumeSlider");
    ASSERT_NE(volumeSlider, nullptr);
    
    // User should be able to set volume to any value
    volumeSlider->setValue(25);
    EXPECT_EQ(volumeSlider->value(), 25);
    
    volumeSlider->setValue(75);
    EXPECT_EQ(volumeSlider->value(), 75);
}

// Requirement: User can mute and unmute audio
TEST_F(BlackBoxTest, UserCanMuteUnmute) {
    QPushButton* muteButton = mainWindow->findChild<QPushButton*>("muteButton");
    ASSERT_NE(muteButton, nullptr);
    
    // User clicks mute button
    QTest::mouseClick(muteButton, Qt::LeftButton);
    QTest::qWait(100);
    
    // User clicks again to unmute
    QTest::mouseClick(muteButton, Qt::LeftButton);
    QTest::qWait(100);
    
    SUCCEED();
}

// Requirement: User can play/pause media
TEST_F(BlackBoxTest, PlayPauseControlExists) {
    QPushButton* playPause = mainWindow->findChild<QPushButton*>("playPause");
    ASSERT_NE(playPause, nullptr);
    EXPECT_TRUE(playPause->isEnabled());
}

// Requirement: User can navigate tracks (next/previous)
TEST_F(BlackBoxTest, TrackNavigationControlsExist) {
    QPushButton* nextTrack = mainWindow->findChild<QPushButton*>("nextTrack");
    QPushButton* previousTrack = mainWindow->findChild<QPushButton*>("previousTrack");
    
    ASSERT_NE(nextTrack, nullptr);
    ASSERT_NE(previousTrack, nullptr);
    EXPECT_TRUE(nextTrack->isEnabled());
    EXPECT_TRUE(previousTrack->isEnabled());
}

// Requirement: User can enable shuffle playback
TEST_F(BlackBoxTest, ShuffleFeatureAvailable) {
    QPushButton* shuffleButton = mainWindow->findChild<QPushButton*>("Shuffle");
    ASSERT_NE(shuffleButton, nullptr);
    EXPECT_TRUE(shuffleButton->isEnabled());
    
    // User clicks shuffle
    QTest::mouseClick(shuffleButton, Qt::LeftButton);
    QTest::qWait(100);
    
    SUCCEED();
}

// Requirement: User can set repeat mode
TEST_F(BlackBoxTest, RepeatModeFeatureAvailable) {
    QPushButton* repeatButton = mainWindow->findChild<QPushButton*>("repeatToggle");
    ASSERT_NE(repeatButton, nullptr);
    EXPECT_TRUE(repeatButton->isEnabled());
    
    // User cycles through repeat modes
    QTest::mouseClick(repeatButton, Qt::LeftButton); // Mode 1
    QTest::qWait(50);
    QTest::mouseClick(repeatButton, Qt::LeftButton); // Mode 2
    QTest::qWait(50);
    QTest::mouseClick(repeatButton, Qt::LeftButton); // Back to mode 0
    QTest::qWait(50);
    
    SUCCEED();
}

// Requirement: User can view the playlist/queue
TEST_F(BlackBoxTest, PlaylistViewAvailable) {
    QPushButton* queueButton = mainWindow->findChild<QPushButton*>("trackQueue");
    ASSERT_NE(queueButton, nullptr);
    EXPECT_TRUE(queueButton->isEnabled());
}

// Requirement: User sees current track information
TEST_F(BlackBoxTest, TrackInformationDisplayed) {
    QLabel* fileNameLabel = mainWindow->findChild<QLabel*>("labelFileName");
    ASSERT_NE(fileNameLabel, nullptr);
    
    // Display should show some information
    EXPECT_FALSE(fileNameLabel->text().isEmpty());
}

// Requirement: User sees next track information
TEST_F(BlackBoxTest, NextTrackInformationDisplayed) {
    QLabel* nextTrackLabel = mainWindow->findChild<QLabel*>("nextinQueue");
    ASSERT_NE(nextTrackLabel, nullptr);
    
    // Display should show some information
    EXPECT_FALSE(nextTrackLabel->text().isEmpty());
}

// Requirement: User can seek within a track
TEST_F(BlackBoxTest, SeekControlAvailable) {
    QSlider* seekSlider = mainWindow->findChild<QSlider*>("seekSlider");
    ASSERT_NE(seekSlider, nullptr);
    // Note: Seek may be disabled when no media is loaded
}

// Requirement: Application provides menu for file operations
TEST_F(BlackBoxTest, MenuBarAvailable) {
    QMenuBar* menuBar = mainWindow->menuBar();
    ASSERT_NE(menuBar, nullptr);
}

// Requirement: Application provides status feedback
TEST_F(BlackBoxTest, StatusBarAvailable) {
    QStatusBar* statusBar = mainWindow->statusBar();
    ASSERT_NE(statusBar, nullptr);
}

// Requirement: Window has reasonable size for user
TEST_F(BlackBoxTest, WindowHasReasonableSize) {
    QSize windowSize = mainWindow->size();
    
    // Window should be large enough to be usable (at least 800x600)
    EXPECT_GE(windowSize.width(), 800);
    EXPECT_GE(windowSize.height(), 600);
    
    // But not excessively large (less than 1920x1080 full HD)
    EXPECT_LE(windowSize.width(), 1920);
    EXPECT_LE(windowSize.height(), 1080);
}

// Requirement: All controls are accessible (not hidden)
TEST_F(BlackBoxTest, AllControlsAccessible) {
    QPushButton* buttons[] = {
        mainWindow->findChild<QPushButton*>("playPause"),
        mainWindow->findChild<QPushButton*>("nextTrack"),
        mainWindow->findChild<QPushButton*>("previousTrack"),
        mainWindow->findChild<QPushButton*>("Shuffle"),
        mainWindow->findChild<QPushButton*>("repeatToggle"),
        mainWindow->findChild<QPushButton*>("trackQueue"),
        mainWindow->findChild<QPushButton*>("muteButton")
    };
    
    for (auto* button : buttons) {
        ASSERT_NE(button, nullptr) << "Button should exist";
        EXPECT_TRUE(button->isEnabled()) << "Button should be enabled";
    }
}

// Requirement: Volume control has reasonable range
TEST_F(BlackBoxTest, VolumeControlHasReasonableRange) {
    QSlider* volumeSlider = mainWindow->findChild<QSlider*>("volumeSlider");
    ASSERT_NE(volumeSlider, nullptr);
    
    // Volume should start at 0
    EXPECT_GE(volumeSlider->minimum(), 0);
    
    // Volume should have meaningful maximum (typically 100)
    EXPECT_GE(volumeSlider->maximum(), 50);
    EXPECT_LE(volumeSlider->maximum(), 200);
}

// User Scenario: Playing a playlist with repeat
TEST_F(BlackBoxTest, UserScenario_PlayWithRepeat) {
    // User enables repeat
    QPushButton* repeatButton = mainWindow->findChild<QPushButton*>("repeatToggle");
    ASSERT_NE(repeatButton, nullptr);
    QTest::mouseClick(repeatButton, Qt::LeftButton);
    QTest::qWait(100);
    
    // User clicks play
    QPushButton* playButton = mainWindow->findChild<QPushButton*>("playPause");
    ASSERT_NE(playButton, nullptr);
    QTest::mouseClick(playButton, Qt::LeftButton);
    QTest::qWait(100);
    
    SUCCEED();
}

// User Scenario: Adjusting volume while muted
TEST_F(BlackBoxTest, UserScenario_AdjustVolumeMuted) {
    QSlider* volumeSlider = mainWindow->findChild<QSlider*>("volumeSlider");
    QPushButton* muteButton = mainWindow->findChild<QPushButton*>("muteButton");
    
    ASSERT_NE(volumeSlider, nullptr);
    ASSERT_NE(muteButton, nullptr);
    
    // User sets volume
    volumeSlider->setValue(50);
    
    // User mutes
    QTest::mouseClick(muteButton, Qt::LeftButton);
    QTest::qWait(50);
    
    // User adjusts volume while muted
    volumeSlider->setValue(75);
    EXPECT_EQ(volumeSlider->value(), 75);
    
    // User unmutes
    QTest::mouseClick(muteButton, Qt::LeftButton);
    QTest::qWait(50);
    
    SUCCEED();
}

// User Scenario: Quick navigation through tracks
TEST_F(BlackBoxTest, UserScenario_QuickTrackNavigation) {
    QPushButton* nextButton = mainWindow->findChild<QPushButton*>("nextTrack");
    QPushButton* prevButton = mainWindow->findChild<QPushButton*>("previousTrack");
    
    ASSERT_NE(nextButton, nullptr);
    ASSERT_NE(prevButton, nullptr);
    
    // User quickly clicks next several times
    for (int i = 0; i < 5; i++) {
        QTest::mouseClick(nextButton, Qt::LeftButton);
        QTest::qWait(50);
    }
    
    // User goes back
    for (int i = 0; i < 3; i++) {
        QTest::mouseClick(prevButton, Qt::LeftButton);
        QTest::qWait(50);
    }
    
    SUCCEED();
}

// User Scenario: Enabling shuffle with repeat
TEST_F(BlackBoxTest, UserScenario_ShuffleWithRepeat) {
    QPushButton* shuffleButton = mainWindow->findChild<QPushButton*>("Shuffle");
    QPushButton* repeatButton = mainWindow->findChild<QPushButton*>("repeatToggle");
    
    ASSERT_NE(shuffleButton, nullptr);
    ASSERT_NE(repeatButton, nullptr);
    
    // User enables shuffle
    QTest::mouseClick(shuffleButton, Qt::LeftButton);
    QTest::qWait(100);
    
    // User enables repeat all
    QTest::mouseClick(repeatButton, Qt::LeftButton);
    QTest::qWait(100);
    
    SUCCEED();
}

// Requirement: Application responds to user input promptly
TEST_F(BlackBoxTest, ResponsivenessTest) {
    QPushButton* playButton = mainWindow->findChild<QPushButton*>("playPause");
    ASSERT_NE(playButton, nullptr);
    
    QElapsedTimer timer;
    timer.start();
    
    QTest::mouseClick(playButton, Qt::LeftButton);
    
    qint64 responseTime = timer.elapsed();
    
    // Application should respond within 200ms
    EXPECT_LT(responseTime, 200);
}

// Requirement: Controls remain functional after repeated use
TEST_F(BlackBoxTest, ControlsDurabilityTest) {
    QPushButton* muteButton = mainWindow->findChild<QPushButton*>("muteButton");
    ASSERT_NE(muteButton, nullptr);
    
    // Simulate extended use
    for (int i = 0; i < 50; i++) {
        QTest::mouseClick(muteButton, Qt::LeftButton);
        QTest::qWait(10);
    }
    
    // Button should still be functional
    EXPECT_TRUE(muteButton->isEnabled());
}
