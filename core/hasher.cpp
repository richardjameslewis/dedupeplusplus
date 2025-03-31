#include "hasher.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace dedupe {

std::string Hasher::hash_file(const std::filesystem::path& file_path,
                            Progress& progress) {
    if (!std::filesystem::exists(file_path)) {
        throw std::runtime_error("File does not exist: " + file_path.string());
    }

    return hash_content(file_path, progress);
}

std::string Hasher::hash_content(const std::filesystem::path& file_path,
                               Progress& progress) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + file_path.string());
    }

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    std::vector<char> buffer(BUFFER_SIZE);
    std::uintmax_t total_read = 0;
    auto file_size = std::filesystem::file_size(file_path);

    while (file.read(buffer.data(), BUFFER_SIZE)) {
        if (progress.is_cancelled()) {
            return "";
        }

        SHA256_Update(&sha256, buffer.data(), file.gcount());
        total_read += file.gcount();

        if (file_size > 0) {
            double progress_value = static_cast<double>(total_read) / file_size;
            progress.report("Hashing file: " + file_path.filename().string(), progress_value);
        }
    }

    if (file.gcount() > 0) {
        SHA256_Update(&sha256, buffer.data(), file.gcount());
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }

    return ss.str();
}

} // namespace dedupe 