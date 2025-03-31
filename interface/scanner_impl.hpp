#pragma once

#include "iscanner.hpp"

namespace dedupe {
    

class ScannerImpl : public IScanner {
public:
    explicit ScannerImpl(bool recursive = true)
        : scanner_(recursive)
    {}

    std::vector<DuplicateGroup> scan_directory(
        const std::filesystem::path& directory,
        ProgressCallback progress_callback = nullptr,
        CancellationCallback cancellation_callback = nullptr) override {
        
        Progress progress(progress_callback, cancellation_callback);
        return scanner_.scan_directory(directory, progress);
    }

private:
    Scanner scanner_;
};

} // namespace dedupe 