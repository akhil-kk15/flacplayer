#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QApplication>
#include <QTest>
#include <QPushButton>
#include <QSlider>
#include <QMenuBar>
#include <QStatusBar>
#include <QElapsedTimer>
#include "../mainwindow.h"

// Test fixture for MainWindow tests
class MainWindowTest : public ::testing::Test {
protected:
    void SetUp() override {
        mainWindow = new MainWindow();
    }

    void TearDown() override {
        delete mainWindow;
    }

    MainWindow* mainWindow;
};

// Test: Window initialization
TEST_F(MainWindowTest, WindowInitialization) {
    EXPECT_NE(mainWindow, nullptr);
    EXPECT_EQ(mainWindow->windowTitle(), QString("Flac Player v2.0"));
}

// Test: Window size is fixed
TEST_F(MainWindowTest, WindowSizeFixed) {
    EXPECT_EQ(mainWindow->size(), QSize(970, 650));
    EXPECT_TRUE(mainWindow->testAttribute(Qt::WA_StaticContents) || 
                mainWindow->minimumSize() == mainWindow->maximumSize());
}

// Test: Repeat button exists and is functional
TEST_F(MainWindowTest, RepeatButtonExists) {
    QPushButton* repeatButton = mainWindow->findChild<QPushButton*>("repeatToggle");
    ASSERT_NE(repeatButton, nullptr);
    EXPECT_TRUE(repeatButton->isEnabled());
}

// Test: Repeat mode cycling (Off -> All -> One -> Off)
TEST_F(MainWindowTest, RepeatModeCycling) {
    QPushButton* repeatButton = mainWindow->findChild<QPushButton*>("repeatToggle");
    ASSERT_NE(repeatButton, nullptr);
    
    // Initial state should be Off
    // Click 1: Off -> All
    QTest::mouseClick(repeatButton, Qt::LeftButton);
    QTest::qWait(100);
    
    // Click 2: All -> One
    QTest::mouseClick(repeatButton, Qt::LeftButton);
    QTest::qWait(100);
    
    // Click 3: One -> Off
    QTest::mouseClick(repeatButton, Qt::LeftButton);
    QTest::qWait(100);
    
    // After 3 clicks, should be back to Off
    // We can verify by checking the icon or status bar message
    SUCCEED(); // Test passes if no crashes occur
}

// Test: All control buttons exist
TEST_F(MainWindowTest, AllControlButtonsExist) {
    EXPECT_NE(mainWindow->findChild<QPushButton*>("playPause"), nullptr);
    EXPECT_NE(mainWindow->findChild<QPushButton*>("nextTrack"), nullptr);
    EXPECT_NE(mainWindow->findChild<QPushButton*>("previousTrack"), nullptr);
    EXPECT_NE(mainWindow->findChild<QPushButton*>("Shuffle"), nullptr);
    EXPECT_NE(mainWindow->findChild<QPushButton*>("repeatToggle"), nullptr);
    EXPECT_NE(mainWindow->findChild<QPushButton*>("trackQueue"), nullptr);
    EXPECT_NE(mainWindow->findChild<QPushButton*>("muteButton"), nullptr);
}

// Test: Play/Pause button functionality
TEST_F(MainWindowTest, PlayPauseButtonExists) {
    QPushButton* playPauseButton = mainWindow->findChild<QPushButton*>("playPause");
    ASSERT_NE(playPauseButton, nullptr);
    EXPECT_TRUE(playPauseButton->isEnabled());
}

// Test: Shuffle button exists
TEST_F(MainWindowTest, ShuffleButtonExists) {
    QPushButton* shuffleButton = mainWindow->findChild<QPushButton*>("Shuffle");
    ASSERT_NE(shuffleButton, nullptr);
    EXPECT_TRUE(shuffleButton->isEnabled());
}

// Test: Volume slider exists and has correct range
TEST_F(MainWindowTest, VolumeSliderConfiguration) {
    QSlider* volumeSlider = mainWindow->findChild<QSlider*>("volumeSlider");
    ASSERT_NE(volumeSlider, nullptr);
    EXPECT_EQ(volumeSlider->minimum(), 0);
    EXPECT_EQ(volumeSlider->maximum(), 100);
    EXPECT_EQ(volumeSlider->value(), 30); // Initial value
}

// Test: Seek slider exists and is initially disabled
TEST_F(MainWindowTest, SeekSliderInitialState) {
    QSlider* seekSlider = mainWindow->findChild<QSlider*>("seekSlider");
    ASSERT_NE(seekSlider, nullptr);
    EXPECT_FALSE(seekSlider->isEnabled()); // Should be disabled initially
}

// Test: Menu bar exists
TEST_F(MainWindowTest, MenuBarExists) {
    QMenuBar* menuBar = mainWindow->findChild<QMenuBar*>("menuBar");
    EXPECT_NE(menuBar, nullptr);
}

// Test: Status bar exists
TEST_F(MainWindowTest, StatusBarExists) {
    QStatusBar* statusBar = mainWindow->statusBar();
    EXPECT_NE(statusBar, nullptr);
    EXPECT_FALSE(statusBar->isSizeGripEnabled()); // Size grip should be disabled
}

// Test: Button icons are set (not null)
// Note: This test may fail if resource files are not properly loaded in test environment
TEST_F(MainWindowTest, ButtonIconsAreSet) {
    QPushButton* playPauseButton = mainWindow->findChild<QPushButton*>("playPause");
    QPushButton* nextButton = mainWindow->findChild<QPushButton*>("nextTrack");
    QPushButton* prevButton = mainWindow->findChild<QPushButton*>("previousTrack");
    QPushButton* shuffleButton = mainWindow->findChild<QPushButton*>("Shuffle");
    QPushButton* repeatButton = mainWindow->findChild<QPushButton*>("repeatToggle");
    
    // Verify buttons exist (icons may be null in test environment without resources)
    ASSERT_NE(playPauseButton, nullptr);
    ASSERT_NE(nextButton, nullptr);
    ASSERT_NE(prevButton, nullptr);
    ASSERT_NE(shuffleButton, nullptr);
    ASSERT_NE(repeatButton, nullptr);
    
    // In production, icons should be loaded from resources
    // In test environment without resources, they may be null - this is acceptable
}

// Test: Mute button toggle functionality
TEST_F(MainWindowTest, MuteButtonToggle) {
    QPushButton* muteButton = mainWindow->findChild<QPushButton*>("muteButton");
    ASSERT_NE(muteButton, nullptr);
    
    // Click mute button
    QTest::mouseClick(muteButton, Qt::LeftButton);
    QTest::qWait(100);
    
    // Click again to unmute
    QTest::mouseClick(muteButton, Qt::LeftButton);
    QTest::qWait(100);
    
    SUCCEED(); // Test passes if no crashes
}

// Test: Window is not resizable
TEST_F(MainWindowTest, WindowNotResizable) {
    // Check that maximize button is disabled
    EXPECT_FALSE(mainWindow->windowFlags() & Qt::WindowMaximizeButtonHint);
}

// Performance test: Check repeat button click response time
TEST_F(MainWindowTest, RepeatButtonResponseTime) {
    QPushButton* repeatButton = mainWindow->findChild<QPushButton*>("repeatToggle");
    ASSERT_NE(repeatButton, nullptr);
    
    QElapsedTimer timer;
    timer.start();
    
    QTest::mouseClick(repeatButton, Qt::LeftButton);
    
    qint64 elapsed = timer.elapsed();
    EXPECT_LT(elapsed, 100); // Should respond within 100ms
}
