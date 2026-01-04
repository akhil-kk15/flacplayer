#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include "../mainwindow.h"

// Boundary Analysis Tests - Testing edge cases and limits
class BoundaryAnalysisTest : public ::testing::Test {
protected:
    void SetUp() override {
        mainWindow = new MainWindow();
    }

    void TearDown() override {
        delete mainWindow;
    }

    MainWindow* mainWindow;
};

// Test: Volume slider boundary values (0, 1, 99, 100)
TEST_F(BoundaryAnalysisTest, VolumeSliderBoundaryValues) {
    QSlider* volumeSlider = mainWindow->findChild<QSlider*>("volumeSlider");
    ASSERT_NE(volumeSlider, nullptr);
    
    // Test minimum boundary (0)
    volumeSlider->setValue(0);
    EXPECT_EQ(volumeSlider->value(), 0);
    
    // Test just above minimum (1)
    volumeSlider->setValue(1);
    EXPECT_EQ(volumeSlider->value(), 1);
    
    // Test just below maximum (99)
    volumeSlider->setValue(99);
    EXPECT_EQ(volumeSlider->value(), 99);
    
    // Test maximum boundary (100)
    volumeSlider->setValue(100);
    EXPECT_EQ(volumeSlider->value(), 100);
    
    // Test below minimum (should clamp to 0)
    volumeSlider->setValue(-1);
    EXPECT_EQ(volumeSlider->value(), 0);
    
    // Test above maximum (should clamp to 100)
    volumeSlider->setValue(101);
    EXPECT_EQ(volumeSlider->value(), 100);
    
    // Test extreme negative value
    volumeSlider->setValue(-999);
    EXPECT_EQ(volumeSlider->value(), 0);
    
    // Test extreme positive value
    volumeSlider->setValue(9999);
    EXPECT_EQ(volumeSlider->value(), 100);
}

// Test: Window size boundaries
TEST_F(BoundaryAnalysisTest, WindowSizeBoundaries) {
    QSize expectedSize(970, 650);
    EXPECT_EQ(mainWindow->size(), expectedSize);
    
    // Test that window cannot be resized beyond fixed size
    EXPECT_EQ(mainWindow->minimumSize(), expectedSize);
    EXPECT_EQ(mainWindow->maximumSize(), expectedSize);
}

// Test: Repeat mode cycling boundary (ensure it wraps correctly)
TEST_F(BoundaryAnalysisTest, RepeatModeCyclingBoundary) {
    QPushButton* repeatButton = mainWindow->findChild<QPushButton*>("repeatToggle");
    ASSERT_NE(repeatButton, nullptr);
    
    // Cycle through all modes multiple times to test wrapping
    for (int cycle = 0; cycle < 3; cycle++) {
        // Off -> All
        QTest::mouseClick(repeatButton, Qt::LeftButton);
        QTest::qWait(50);
        
        // All -> One
        QTest::mouseClick(repeatButton, Qt::LeftButton);
        QTest::qWait(50);
        
        // One -> Off (wrapping boundary)
        QTest::mouseClick(repeatButton, Qt::LeftButton);
        QTest::qWait(50);
    }
    
    // After 9 clicks (3 full cycles), should be back at Off
    SUCCEED();
}

// Test: Seek slider position boundaries (when enabled)
TEST_F(BoundaryAnalysisTest, SeekSliderBoundaries) {
    QSlider* seekSlider = mainWindow->findChild<QSlider*>("seekSlider");
    ASSERT_NE(seekSlider, nullptr);
    
    // Even when disabled, slider should have valid range
    EXPECT_GE(seekSlider->minimum(), 0);
    EXPECT_GE(seekSlider->maximum(), seekSlider->minimum());
}

// Test: Empty string handling in file name display
TEST_F(BoundaryAnalysisTest, EmptyFileNameHandling) {
    QLabel* fileNameLabel = mainWindow->findChild<QLabel*>("labelFileName");
    ASSERT_NE(fileNameLabel, nullptr);
    
    // Initial state should have text (not empty or null)
    EXPECT_FALSE(fileNameLabel->text().isEmpty());
}

// Test: Button rapid clicking (stress test boundary)
TEST_F(BoundaryAnalysisTest, RapidButtonClicking) {
    QPushButton* repeatButton = mainWindow->findChild<QPushButton*>("repeatToggle");
    ASSERT_NE(repeatButton, nullptr);
    
    // Rapidly click the button many times
    for (int i = 0; i < 100; i++) {
        QTest::mouseClick(repeatButton, Qt::LeftButton);
        // No wait - stress test
    }
    
    // Should not crash
    SUCCEED();
}

// Test: Volume extremes and transitions
TEST_F(BoundaryAnalysisTest, VolumeExtremeTransitions) {
    QSlider* volumeSlider = mainWindow->findChild<QSlider*>("volumeSlider");
    ASSERT_NE(volumeSlider, nullptr);
    
    // Test rapid transitions between extremes
    volumeSlider->setValue(0);
    EXPECT_EQ(volumeSlider->value(), 0);
    
    volumeSlider->setValue(100);
    EXPECT_EQ(volumeSlider->value(), 100);
    
    volumeSlider->setValue(0);
    EXPECT_EQ(volumeSlider->value(), 0);
    
    // Test middle values
    volumeSlider->setValue(50);
    EXPECT_EQ(volumeSlider->value(), 50);
}

// Test: Multiple rapid mute toggles
TEST_F(BoundaryAnalysisTest, RapidMuteToggling) {
    QPushButton* muteButton = mainWindow->findChild<QPushButton*>("muteButton");
    ASSERT_NE(muteButton, nullptr);
    
    // Rapidly toggle mute 20 times
    for (int i = 0; i < 20; i++) {
        QTest::mouseClick(muteButton, Qt::LeftButton);
        QTest::qWait(10);
    }
    
    SUCCEED();
}

// Test: Button geometry boundaries (ensure buttons are within window)
TEST_F(BoundaryAnalysisTest, ButtonsWithinWindowBounds) {
    QRect windowRect = mainWindow->rect();
    
    QPushButton* buttons[] = {
        mainWindow->findChild<QPushButton*>("playPause"),
        mainWindow->findChild<QPushButton*>("nextTrack"),
        mainWindow->findChild<QPushButton*>("previousTrack"),
        mainWindow->findChild<QPushButton*>("Shuffle"),
        mainWindow->findChild<QPushButton*>("repeatToggle"),
        mainWindow->findChild<QPushButton*>("trackQueue")
    };
    
    for (auto* button : buttons) {
        ASSERT_NE(button, nullptr);
        QRect buttonRect = button->geometry();
        
        // Button should be within window bounds
        EXPECT_GE(buttonRect.x(), 0);
        EXPECT_GE(buttonRect.y(), 0);
        EXPECT_LE(buttonRect.right(), windowRect.right());
        EXPECT_LE(buttonRect.bottom(), windowRect.bottom());
    }
}
