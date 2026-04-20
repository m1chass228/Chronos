#include "filemanager.hpp"

#include <openssl/sha.h>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace chronos::filemanager {

FileManager::FileManager(const std::string& basePath) : m_basePath(basePath) {
    if (!m_basePath.empty()) {
        std::error_code ec;
        if (std::filesystem::create_directories(m_basePath, ec)) {
            std::cout << "✔ Storage directory initialized at: " << m_basePath << std::endl;
        } else if (ec) {
            std::cerr << "Failed to create storage: " << ec.message() << std::endl;
        }
    }
}

bool FileManager::removeAttachment(const std::string& attachment) {
    if (!attachment.empty()) {
        try {
            if (std::filesystem::remove(std::filesystem::path(attachment))) {
                return true;
            } else {
                return false;
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
        // Сюда попадем, если, например, нет прав доступа
        std::cerr << "FS Error: " << e.what() << std::endl;
        std::cerr << "Error code: " << e.code().message() << std::endl;
        } 
        catch (const std::exception& e) {
            // Для прочих стандартных исключений
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    return false;
}

std::vector<std::string> FileManager::storeAttachments(const std::vector<std::string>& file_paths) {
    std::vector<std::string> result_paths;

    for (const auto &file : file_paths) {
        if (!std::filesystem::exists(file)) {
            std::cerr << "File not found at: " << file << std::endl;
            continue;
        }

        std::string hash = calculateHash(file);
        if (hash.empty()) continue;

        std::filesystem::path oldPath(file);
        std::filesystem::path newPath = std::filesystem::path(m_basePath) / (hash + oldPath.extension().string());

        if (std::filesystem::exists(newPath)) {
            result_paths.push_back(newPath.string());
        } else {
            try {
                std::filesystem::copy_file(oldPath, newPath);
                result_paths.push_back(newPath.string());
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "FS Error: " << e.what() << std::endl;
            }
        }
    }

    return result_paths; // Возвращаем чистый вектор путей
}

std::string FileManager::calculateHash(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        SHA256_Update(&sha256, buffer, file.gcount());
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();

}

}
