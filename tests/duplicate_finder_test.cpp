#include <gtest/gtest.h>
#include "../core/duplicate_finder.hpp"
#include "../core/filesystem_tree.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <unordered_map>

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
    Progress progress;
    FileSystemTree tree = tree.buildFromPath(testDir, progress);
    
    // Create a progress object
    // dedupe::Progress progress(
    //   [](const std::string& message, double progress) {
    //        std::cout << "\r" << message << " [" 
    //                << static_cast<int>(progress * 100) << "%]" << std::flush;
    //         std::cout << std::endl;
    //   },
    //   nullptr
    // );
   
    // Find duplicates
    auto duplicates = DuplicateFinder::findDuplicates(tree, progress);
    
    // We should have three files in the duplicate map
    EXPECT_EQ(duplicates.size(), 3);
    
    // Check that all duplicate files have the same signature
    std::string firstHash;
    uintmax_t firstSize;
    int firstCount;
    bool foundFirst = false;
    
    for (const auto& [path, signature] : duplicates) {
        if (!foundFirst) {
            firstHash = signature.hash;
            firstSize = signature.size;
            firstCount = signature.count;
            foundFirst = true;
        } else {
            EXPECT_EQ(signature.hash, firstHash);
            EXPECT_EQ(signature.size, firstSize);
            EXPECT_EQ(signature.count, firstCount);
        }
    }
    
    // Check that the count is correct (should be 3 for the duplicate content)
    EXPECT_EQ(firstCount, 3);
    
    // Check that the files are the ones we expect
    std::vector<std::string> expectedFiles = {
        "file1.txt",
        "file2.txt",
        "subdir/file4.txt"
    };
    
    for (const auto& [path, signature] : duplicates) {
        auto filename = path.filename().string();
        if (path.parent_path().filename() == "subdir") {
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

    // Create a progress object
    Progress progress;

    // Build the filesystem tree
    FileSystemTree tree = tree.buildFromPath(noDupDir, progress);
    
    // Find duplicates
    auto duplicates = DuplicateFinder::findDuplicates(tree, progress);
    
    // We should have no duplicates
    EXPECT_EQ(duplicates.size(), 0);
    
    // Clean up
    std::filesystem::remove_all(noDupDir);
}

TEST_F(DuplicateFinderTest, Cancellation) {
    // Create a progress object
    Progress progress;

    // Build the filesystem tree
    FileSystemTree tree = tree.buildFromPath(testDir, progress);
        
    // Create a cancellation function that cancels immediately
    auto shouldCancel = []() { return true; };
    
    // Find duplicates with cancellation
    auto duplicates = DuplicateFinder::findDuplicates(tree, progress, shouldCancel);
    
    // We should have no duplicates due to cancellation
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
    
    // Create a progress object
    Progress progress;

    // Build the filesystem tree
    FileSystemTree tree = tree.buildFromPath(multiDupDir, progress);
    
    
    // Find duplicates
    auto duplicates = DuplicateFinder::findDuplicates(tree, progress);
    
    // We should have four files in the duplicate map (2 files in each group)
    EXPECT_EQ(duplicates.size(), 4);
    
    // Group the duplicates by their hash to verify we have two distinct groups
    std::unordered_map<std::string, std::vector<std::filesystem::path>> hashGroups;
    for (const auto& [path, signature] : duplicates) {
        hashGroups[signature.hash].push_back(path);
    }
    
    // We should have two distinct hash groups
    EXPECT_EQ(hashGroups.size(), 2);
    
    // Each group should have exactly two files
    for (const auto& [hash, files] : hashGroups) {
        EXPECT_EQ(files.size(), 2);
    }
    
    // Clean up
    std::filesystem::remove_all(multiDupDir);
}

} // namespace test
} // namespace dedupe 