#include "hasher.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace dedupe {

std::string Hasher::hash_file(const std::filesystem::path& file_path,
                            Progress& progress, bool quick) {
    if (!std::filesystem::exists(file_path)) {
        throw std::runtime_error("File does not exist: " + file_path.string());
    }

    return hash_content(file_path, progress, quick);
}

std::string Hasher::hash_content(const std::filesystem::path& file_path,
    Progress& progress, bool quick) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + file_path.string());
    }
    return hash_stream(file, progress, quick);
}



std::string Hasher::hash_stream(std::istream& file, 
    Progress& progress, bool quick) {

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    std::vector<char> buffer(BUFFER_SIZE);
    std::uintmax_t total_read = 0;
    //auto file_size = std::filesystem::file_size(file_path);

    while (file.read(buffer.data(), BUFFER_SIZE)) {
        if (progress.is_cancelled()) {
            return "";
        }

        SHA256_Update(&sha256, buffer.data(), file.gcount());
        total_read += file.gcount();

        //if (file_size > 0) {
        //    double progress_value = static_cast<double>(total_read) / file_size;
        //    progress.report("Hashing file: " + file_path.filename().string(), progress_value);
        //}

        // In quick mode just process the first block
        if (quick)
            break;
    }

    if (!quick && file.gcount() > 0) {
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

std::string Hasher::hash_string(std::string& str, Progress &progress) {
    std::stringstream ss{ str };
    return hash_stream(ss, progress);
}

std::string Hasher::fake_size_hash(uintmax_t size) {
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(size);
    }
    return ss.str();
}

} // namespace dedupe 