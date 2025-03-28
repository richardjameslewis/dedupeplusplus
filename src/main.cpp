#include "scanner.hpp"
#include "progress.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <atomic>

void print_help() {
    std::cout << "Usage: dedupe++ [options] <directory>\n\n"
              << "Options:\n"
              << "  --help              Show this help message\n"
              << "  --recursive         Scan directories recursively (default: true)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    bool recursive = true;
    std::filesystem::path directory;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            print_help();
            return 0;
        }
        else if (arg == "--recursive") {
            recursive = true;
        }
        else {
            directory = arg;
        }
    }

    if (directory.empty()) {
        std::cerr << "Error: Directory not specified\n";
        print_help();
        return 1;
    }

    try {
        std::atomic<bool> cancelled{false};
        dedupe::Progress progress(
            [](const std::string& message, double progress) {
                std::cout << "\r" << message << " [" 
                         << static_cast<int>(progress * 100) << "%]" << std::flush;
            },
            [&cancelled]() { return cancelled.load(); }
        );

        dedupe::Scanner scanner(recursive);
        auto duplicates = scanner.scan_directory(directory, progress);
        
        std::cout << "\n\nFound " << duplicates.size() << " groups of duplicate files:\n\n";
        
        for (const auto& group : duplicates) {
            std::cout << "Hash: " << group.hash << "\n";
            for (const auto& file : group.files) {
                std::cout << "  " << file.string() << "\n";
            }
            std::cout << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 