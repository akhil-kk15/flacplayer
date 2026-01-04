#include <gtest/gtest.h>
#include <random>
#include <algorithm>
#include "../playlist.h"

/**
 * Test suite for custom Playlist data structure
 * Verifies dynamic array implementation functionality
 */
class PlaylistTest : public ::testing::Test {
protected:
    Playlist playlist;
};

// Test basic construction
TEST_F(PlaylistTest, DefaultConstructorCreatesEmptyPlaylist) {
    EXPECT_TRUE(playlist.isEmpty());
    EXPECT_EQ(playlist.size(), 0);
}

// Test append operation
TEST_F(PlaylistTest, AppendAddsElements) {
    playlist.append("/path/file1.flac");
    EXPECT_FALSE(playlist.isEmpty());
    EXPECT_EQ(playlist.size(), 1);
    EXPECT_EQ(playlist[0], "/path/file1.flac");
}

// Test multiple appends and capacity growth
TEST_F(PlaylistTest, MultipleAppendsGrowsDynamically) {
    for (int i = 0; i < 10; ++i) {
        playlist.append(QString("/path/file%1.flac").arg(i));
    }
    EXPECT_EQ(playlist.size(), 10);
    EXPECT_EQ(playlist[5], "/path/file5.flac");
}

// Test array subscript operator
TEST_F(PlaylistTest, SubscriptOperatorReturnsCorrectElement) {
    playlist.append("/path/track1.flac");
    playlist.append("/path/track2.flac");
    playlist.append("/path/track3.flac");
    
    EXPECT_EQ(playlist[0], "/path/track1.flac");
    EXPECT_EQ(playlist[1], "/path/track2.flac");
    EXPECT_EQ(playlist[2], "/path/track3.flac");
}

// Test indexOf
TEST_F(PlaylistTest, IndexOfFindsCorrectPosition) {
    playlist.append("/path/track1.flac");
    playlist.append("/path/track2.flac");
    playlist.append("/path/track3.flac");
    
    EXPECT_EQ(playlist.indexOf("/path/track2.flac"), 1);
    EXPECT_EQ(playlist.indexOf("/nonexistent.flac"), -1);
}

// Test clear operation
TEST_F(PlaylistTest, ClearRemovesAllElements) {
    playlist.append("/path/track1.flac");
    playlist.append("/path/track2.flac");
    playlist.clear();
    
    EXPECT_TRUE(playlist.isEmpty());
    EXPECT_EQ(playlist.size(), 0);
}

// Test copy constructor
TEST_F(PlaylistTest, CopyConstructorCreatesIndependentCopy) {
    playlist.append("/path/track1.flac");
    playlist.append("/path/track2.flac");
    
    Playlist copy(playlist);
    
    EXPECT_EQ(copy.size(), 2);
    EXPECT_EQ(copy[0], "/path/track1.flac");
    EXPECT_EQ(copy[1], "/path/track2.flac");
    
    // Modify original - copy should be independent
    playlist.append("/path/track3.flac");
    EXPECT_EQ(playlist.size(), 3);
    EXPECT_EQ(copy.size(), 2);
}

// Test copy assignment
TEST_F(PlaylistTest, CopyAssignmentCreatesIndependentCopy) {
    playlist.append("/path/track1.flac");
    playlist.append("/path/track2.flac");
    
    Playlist copy;
    copy = playlist;
    
    EXPECT_EQ(copy.size(), 2);
    EXPECT_EQ(copy[0], "/path/track1.flac");
    
    // Modify original
    playlist.clear();
    EXPECT_EQ(copy.size(), 2); // Copy should remain unchanged
}

// Test iterators for std::shuffle compatibility
TEST_F(PlaylistTest, IteratorsWorkWithStdAlgorithms) {
    playlist.append("/path/track1.flac");
    playlist.append("/path/track2.flac");
    playlist.append("/path/track3.flac");
    
    // Verify iterators point to correct positions
    EXPECT_EQ(*playlist.begin(), "/path/track1.flac");
    EXPECT_EQ(playlist.end() - playlist.begin(), 3);
    
    // Test with std::shuffle
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(playlist.begin(), playlist.end(), rng);
    
    // Size should remain same after shuffle
    EXPECT_EQ(playlist.size(), 3);
}

// Test exception handling for out of bounds access
TEST_F(PlaylistTest, OutOfBoundsAccessThrowsException) {
    playlist.append("/path/track1.flac");
    
    EXPECT_THROW(playlist[-1], std::out_of_range);
    EXPECT_THROW(playlist[5], std::out_of_range);
}

// Test large dataset performance
TEST_F(PlaylistTest, HandlesLargeDataset) {
    const int LARGE_SIZE = 1000;
    
    for (int i = 0; i < LARGE_SIZE; ++i) {
        playlist.append(QString("/path/track%1.flac").arg(i));
    }
    
    EXPECT_EQ(playlist.size(), LARGE_SIZE);
    EXPECT_EQ(playlist[500], "/path/track500.flac");
    EXPECT_EQ(playlist[LARGE_SIZE - 1], QString("/path/track%1.flac").arg(LARGE_SIZE - 1));
}

// Test modification through subscript operator
TEST_F(PlaylistTest, SubscriptOperatorAllowsModification) {
    playlist.append("/path/track1.flac");
    playlist.append("/path/track2.flac");
    
    playlist[1] = "/path/modified.flac";
    
    EXPECT_EQ(playlist[1], "/path/modified.flac");
    EXPECT_EQ(playlist.size(), 2); // Size should not change
}
