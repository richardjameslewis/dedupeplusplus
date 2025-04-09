#include <gtest/gtest.h>
#include "../core/duplicate_finder.hpp"
#include "../core/filesystem_tree.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

namespace dedupe {
namespace test {

class DuplicateFinderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for testing
        testDir = std::filesystem::temp_directory_path() / "dedupe_test";
        std::filesystem::create_directories(testDir);
        
        // Create some test files with duplicate content
        std::ofstream(testDir / "file1.txt") << "duplicate content";
        std::ofstream(testDir / "file2.txt") << "duplicate content";
        std::ofstream(testDir / "file3.txt") << "unique content";
        
        // Create a subdirectory with more duplicates
        auto subdir = testDir / "subdir";
        std::filesystem::create_directories(subdir);
        std::ofstream(subdir / "file4.txt") << "duplicate content";
        std::ofstream(subdir / "file5.txt") << "another unique content";
    }
    
    void TearDown() override {
        // Clean up the test directory
        std::filesystem::remove_all(testDir);
    }
    
    std::filesystem::path testDir;
};

TEST_F(DuplicateFinderTest, FindDuplicates) {
    // Build the filesystem tree
    FileSystemTree tree = tree.buildFromPath(testDir);
    
    // Create a progress object
    //Progress progress;
    dedupe::Progress progress(
      [](const std::string& message, double progress) {
           std::cout << "\r" << message << " [" 
                   << static_cast<int>(progress * 100) << "%]" << std::flush;
            std::cout << std::endl;
      },
      nullptr
    );

   
    // Find duplicates
    auto duplicates = DuplicateFinder::findDuplicates(tree, progress);
    
    // We should have one duplicate group with 3 files
    EXPECT_EQ(duplicates.size(), 1);
    
    // Check the duplicate group
    const auto& group = duplicates[0];
    EXPECT_EQ(group.size, 17); // "duplicate content" is 17 bytes
    EXPECT_EQ(group.files.size(), 3);
    
    // Check that all files in the group have the same hash
    std::string firstHash = group.files[0].hash;
    for (const auto& file : group.files) {
        EXPECT_EQ(file.hash, firstHash);
    }
    
    // Check that the files are the ones we expect
    std::vector<std::string> expectedFiles = {
        "file1.txt",
        "file2.txt",
        "subdir/file4.txt"
    };
    
    for (const auto& file : group.files) {
        auto filename = file.path.filename().string();
        if (file.path.parent_path().filename() == "subdir") {
            filename = "subdir/" + filename;
        }
        EXPECT_TRUE(std::find(expectedFiles.begin(), expectedFiles.end(), filename) != expectedFiles.end());
    }
}

TEST_F(DuplicateFinderTest, NoDuplicates) {
    // Create a directory with no duplicates
    auto noDupDir = std::filesystem::temp_directory_path() / "dedupe_nodup_test";
    std::filesystem::create_directories(noDupDir);
    
    std::ofstream(noDupDir / "file1.txt") << "unique content 1";
    std::ofstream(noDupDir / "file2.txt") << "unique content 2";
    
    // Build the filesystem tree
    FileSystemTree tree = tree.buildFromPath(noDupDir);
    
    // Create a progress object
    Progress progress;
    // Find duplicates
    auto duplicates = DuplicateFinder::findDuplicates(tree, progress);
    
    // We should have no duplicate groups
    EXPECT_EQ(duplicates.size(), 0);
    
    // Clean up
    std::filesystem::remove_all(noDupDir);
}

TEST_F(DuplicateFinderTest, Cancellation) {
    // Build the filesystem tree
    FileSystemTree tree = tree.buildFromPath(testDir);
    
    // Create a progress object
    Progress progress;
    
    // Create a cancellation function that cancels immediately
    auto shouldCancel = []() { return true; };
    
    // Find duplicates with cancellation
    auto duplicates = DuplicateFinder::findDuplicates(tree, progress, shouldCancel);
    
    // We should have no duplicate groups due to cancellation
    EXPECT_EQ(duplicates.size(), 0);
}

TEST_F(DuplicateFinderTest, MultipleDuplicateGroups) {
    // Create a directory with multiple duplicate groups
    auto multiDupDir = std::filesystem::temp_directory_path() / "dedupe_multidup_test";
    std::filesystem::create_directories(multiDupDir);
    
    // First duplicate group
    std::ofstream(multiDupDir / "file1.txt") << "duplicate group 1";
    std::ofstream(multiDupDir / "file2.txt") << "duplicate group 1";
    
    // Second duplicate group
    std::ofstream(multiDupDir / "file3.txt") << "duplicate group 2";
    std::ofstream(multiDupDir / "file4.txt") << "duplicate group 2";
    
    // Build the filesystem tree
    FileSystemTree tree = tree.buildFromPath(multiDupDir);
    
    // Create a progress object
    Progress progress;
    
    // Find duplicates
    auto duplicates = DuplicateFinder::findDuplicates(tree, progress);
    
    // We should have two duplicate groups
    EXPECT_EQ(duplicates.size(), 2);
    
    // Check that each group has the correct number of files
    for (const auto& group : duplicates) {
        EXPECT_EQ(group.files.size(), 2);
    }
    
    // Clean up
    std::filesystem::remove_all(multiDupDir);
}

} // namespace test
} // namespace dedupe 