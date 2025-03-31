#pragma once

#include "../core/scanner.hpp"
#include <filesystem>
#include <vector>
#include <string>
#include <functional>



namespace dedupe {

/*struct DuplicateGroup {
    std::string hash;
    std::vector<std::filesystem::path> files;
};*/

class IScanner {
public:
    using ProgressCallback = std::function<void(const std::string&, double)>;
    using CancellationCallback = std::function<bool()>;

    virtual ~IScanner() = default;
    
    virtual std::vector<DuplicateGroup> scan_directory(
        const std::filesystem::path& directory,
        ProgressCallback progress_callback = nullptr,
        CancellationCallback cancellation_callback = nullptr) = 0;
};

} // namespace dedupe 