#include <gtest/gtest.h>
#include <QApplication>

int main(int argc, char **argv) {
    // Initialize Qt Application for GUI tests
    QApplication app(argc, argv);
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run tests
    return RUN_ALL_TESTS();
}
