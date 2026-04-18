#pragma once

#include <string>

namespace chronos::parser {

class ConfigParser {
public:
    // Парсер, конструктор
    explicit ConfigParser(const std::string& config_path);

    // Геттеры
    std::string getConnStr() const;
    std::string getAttachPath() const;

private:
    // Приватные поля для хранения настроек
    std::string m_host;
    int m_port;
    std::string m_dbname;
    std::string m_user;
    std::string m_password;
    std::string m_attachments_path;

    // Вспомогательный приватный метод для обработки тильды
    void resolveHomeDirectory();
};



}
