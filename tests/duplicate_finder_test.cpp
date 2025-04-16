#include <gtest/gtest.h>
#include "../core/duplicate_finder.hpp"
#include "../core/filesystem_tree.hpp"
#include "../core/nested_tree.hpp"
//#include "../core/progress.hpp"
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
    
    // Find duplicates
    auto duplicateFinder = DuplicateFinder(tree);
    duplicateFinder.findDuplicates(progress);
        
    // We should have one set of three files in the duplicate map
    //auto& paths = duplicateFinder.pathToDuplicate();
    auto &lookup = duplicateFinder.hashToDuplicate();
    EXPECT_EQ(lookup.size(), 5);
    auto &dups = lookup.begin()->second;
    EXPECT_EQ(dups.paths.size(), 3);
    
    //// Check that all duplicate files have the same signature
    //std::string firstHash;
    //uintmax_t firstSize;
    //int firstCount;
    //bool foundFirst = false;
    //
    //for (const auto& [path, signature] : dups) {
    //    if (!foundFirst) {
    //        firstHash = signature.hash;
    //        firstSize = signature.size;
    //        firstCount = signature.count;
    //        foundFirst = true;
    //    } else {
    //        EXPECT_EQ(signature.hash, firstHash);
    //        EXPECT_EQ(signature.size, firstSize);
    //        EXPECT_EQ(signature.count, firstCount);
    //    }
    //}
    
    // Check that the count is correct (should be 3 for the duplicate content)
    //EXPECT_EQ(firstCount, 3);
    
    // Check that the files are the ones we expect
    std::vector<std::string> expectedFiles = {
        "file1.txt",
        "file2.txt",
        "subdir/file4.txt"
    };
    
    ///for (const auto& [path, signature] : duplicates) {
    for (const auto& str : dups.paths) {
        std::filesystem::path path{ str };
        auto filename = path.filename().string();
        if (path.parent_path().filename() == "subdir") {                // RJL ??? Strips subdir off and then reapplies it...
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
    auto duplicateFinder = DuplicateFinder(tree);
    duplicateFinder.findDuplicates(progress);
    //auto duplicates = duplicateFinder.pathToDuplicate();
    auto lookup = duplicateFinder.hashToDuplicate();
    
    // We should have no duplicates
    EXPECT_EQ(lookup.size(), 3);
    auto it = lookup.begin();
    EXPECT_EQ(it->second.paths.size(), 1);
    ++it;
    EXPECT_EQ(it->second.paths.size(), 1);

    // Clean up
    std::filesystem::remove_all(noDupDir);
}

TEST_F(DuplicateFinderTest, Cancellation) {
    Progress progress(nullptr, []() { return false; });

    // Build the filesystem tree
    FileSystemTree tree = tree.buildFromPath(testDir, progress);

    auto duplicateFinder = DuplicateFinder(tree);
    // Find duplicates with cancellation
    // Create a cancellation function that cancels immediately
    Progress progress2(nullptr, []() { return true; });
    duplicateFinder.findDuplicates(progress2);
    
    // We should have no duplicates due to cancellation
    //EXPECT_EQ(duplicateFinder.pathToDuplicate().size(), 0);
    EXPECT_EQ(duplicateFinder.hashToDuplicate().size(), 0);
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
    auto duplicateFinder = DuplicateFinder(tree);
    duplicateFinder.findDuplicates(progress);
    //auto duplicates = duplicateFinder.pathToDuplicate();
    auto lookup = duplicateFinder.hashToDuplicate();

    // We should have 3 paths in the duplicate map, 2 duplicates and 2 unique
    EXPECT_EQ(lookup.size(), 3);
    auto it = lookup.begin();
    EXPECT_EQ(it->second.paths.size(), 2);
    ++it;
    EXPECT_EQ(it->second.paths.size(), 2);
    
    // Group the duplicates by their hash to verify we have two distinct groups
//    std::unordered_map<std::string, std::vector<std::filesystem::path>> hashGroups;
//    for (const auto& [path, signature] : duplicates) {
//        hashGroups[signature.hash].push_back(path);                                                     // FIX
//    }
    
    // We should have two distinct hash groups
//    EXPECT_EQ(hashGroups.size(), 2);
    
    // Each group should have exactly two files
//    for (const auto& [hash, files] : hashGroups) {
//        EXPECT_EQ(files.size(), 2);
//    }
    
    // Clean up
    std::filesystem::remove_all(multiDupDir);
}

TEST_F(DuplicateFinderTest, MixedIdenticalAndUniqueFiles) {
    // Create a directory structure with mixed identical and unique files
    auto subDir = std::filesystem::temp_directory_path() / "dedupe_mixed_test";
    auto mixedDir = subDir / "X";
    std::filesystem::create_directories(mixedDir);
    
    // Create subdirectories A, B, and C
    auto dirA = mixedDir / "A";
    auto dirB = mixedDir / "B";
    auto dirC = mixedDir / "C";
    std::filesystem::create_directories(dirA);
    std::filesystem::create_directories(dirB);
    std::filesystem::create_directories(dirC);
    
    // Create two distinct files
    std::ofstream(dirA / "file1.txt") << "content for file1";
    std::ofstream(dirA / "file2.txt") << "content for file2";
    
    // Place one file in each of B and C
    std::filesystem::copy_file(dirA / "file1.txt", dirB / "file.txt");
    std::filesystem::copy_file(dirA / "file2.txt", dirC / "file.txt");
    
    // Create a progress object
    Progress progress;
    
    // Build the filesystem tree
    FileSystemTree tree = tree.buildFromPath(subDir, progress);
    
    // Find duplicates

    auto duplicateFinder = DuplicateFinder(tree);
    duplicateFinder.findDuplicates(progress);
    //auto duplicates = duplicateFinder.pathToDuplicate();
    auto lookup = duplicateFinder.hashToDuplicate();
    
    // 9 objects, two sets of dupes so 7
    EXPECT_EQ(lookup.size(), 7);
    
    // Verify that the directory X is not marked as identical
    // This is the key test - the root directory contains both identical and unique files
    auto Xnode = tree.root()->children()[0];
    EXPECT_FALSE(Xnode->data().isIdentical);
    
    // Verify that the subdirectories are correctly marked
    auto nodeA = tree.findByPath(dirA);
    auto nodeB = tree.findByPath(dirB);
    auto nodeC = tree.findByPath(dirC);
    
    // Directory A contains both files but in different directories, so it should not be marked as identical
    EXPECT_TRUE(nodeA->data().isDuplicate);
    EXPECT_FALSE(nodeA->data().isIdentical);
    
    // Directories B and C each contain only one file, so they should be marked as identical
    EXPECT_TRUE(nodeB->data().isDuplicate);
    EXPECT_FALSE(nodeB->data().isIdentical);
    EXPECT_TRUE(nodeC->data().isDuplicate);
    EXPECT_FALSE(nodeC->data().isIdentical);

    // Clean up
    std::filesystem::remove_all(subDir);
}

TEST_F(DuplicateFinderTest, DeepIdenticalFolders) {
    // Create a deep directory structure with mixed identical and unique files
    auto subDir = std::filesystem::temp_directory_path() / "dedupe_deep_test";
    auto mixedDir = subDir / "X";
    std::filesystem::create_directories(mixedDir);

    // Create subdirectories A, B, and C
    auto dirAA = mixedDir / "A" / "A";
    auto dirBB = mixedDir / "B" / "B";
    auto dirCC = mixedDir / "C" / "C";
    std::filesystem::create_directories(dirAA);
    std::filesystem::create_directories(dirBB);
    std::filesystem::create_directories(dirCC);
    auto dirAAA = dirAA / "A";
    auto dirBBB = dirBB / "B";
    auto dirCCC = dirCC / "C";
    std::filesystem::create_directories(dirAAA);
    std::filesystem::create_directories(dirBBB);
    std::filesystem::create_directories(dirCCC);

    // Create two distinct files
    std::ofstream(dirAAA / "file1.txt") << "content for file1";
    std::ofstream(dirAAA / "file2.txt") << "content for file2";

    // Place one file in each of B and C
    std::filesystem::copy_file(dirAAA / "file1.txt", dirBBB / "file.txt");
    std::filesystem::copy_file(dirAAA / "file1.txt", dirCCC / "fileA.txt");
    std::filesystem::copy_file(dirAAA / "file2.txt", dirCCC / "fileB.txt");

    // Create a progress object
    Progress progress;

    // Build the filesystem tree
    FileSystemTree tree = tree.buildFromPath(subDir, progress);

    // Find duplicates

    auto duplicateFinder = DuplicateFinder(tree);
    duplicateFinder.findDuplicates(progress);
    //auto duplicates = duplicateFinder.pathToDuplicate();
    auto lookup = duplicateFinder.hashToDuplicate();

    // root + X = 2
    // X/A, X/C, X/A/A, X/C/C, X/A/A/A, X/C/C/C = 3
    // file1 file2 = 2
    // X/B, X/B/B. X/B/B/B = 3
    // = 10

    EXPECT_EQ(lookup.size(), 10);

    // Verify that the directory X is not marked as identical
    // This is the key test - the root directory contains both identical and unique files
    auto Xnode = tree.root()->children()[0];
    EXPECT_FALSE(Xnode->data().isIdentical);

    //// Verify that the subdirectories are correctly marked
    auto nodeMixed = tree.findByPath(mixedDir);
    auto nodeAA = tree.findByPath(dirAA);
    auto nodeBB = tree.findByPath(dirBB);
    auto nodeCC = tree.findByPath(dirCC);
    auto nodeAAA = tree.findByPath(dirAAA);
    auto nodeBBB = tree.findByPath(dirBBB);
    auto nodeCCC = tree.findByPath(dirCCC);

    // Directory X contains duplicates, but it should not be marked as identical
    EXPECT_TRUE(nodeMixed->data().isDuplicate);
    EXPECT_FALSE(nodeMixed->data().isIdentical);

    //// Directory AAA and CCC are identical
    EXPECT_TRUE(nodeAAA->data().isDuplicate);
    EXPECT_TRUE(nodeAAA->data().isIdentical);
    EXPECT_TRUE(nodeCCC->data().isDuplicate);
    EXPECT_TRUE(nodeCCC->data().isIdentical);

    // Directory BBB contains only one file, and should not be marked as identical
    EXPECT_TRUE(nodeBBB->data().isDuplicate);
    EXPECT_FALSE(nodeBBB->data().isIdentical);

    //// Directory AA and CC are identical
    EXPECT_TRUE(nodeAA->data().isDuplicate);
    EXPECT_TRUE(nodeAA->data().isIdentical);
    EXPECT_TRUE(nodeCC->data().isDuplicate);
    EXPECT_TRUE(nodeCC->data().isIdentical);

    // Clean up
    std::filesystem::remove_all(subDir);
}
} // namespace test
} // namespace dedupe 