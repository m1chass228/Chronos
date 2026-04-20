#pragma once

#include <string>
#include <vector>

namespace chronos::filemanager {

class FileManager {
public: 

    // Конструктор, принимающий путь к файлу
    explicit FileManager(const std::string& basePath);

    std::vector<std::string> storeAttachments(const std::vector<std::string>& file_paths);

    bool removeAttachment(const std::string& attachment);

private:
    std::string m_basePath;

    std::string calculateHash(const std::string& path);

};   


}
