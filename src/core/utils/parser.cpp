#include "parser.hpp"
#include <toml++/toml.hpp>

#include <iostream>
#include <cstdlib>
#include <sstream>

namespace chronos::parser {

ConfigParser::ConfigParser(const std::string& config_path) {
    try {
        toml::table config = toml::parse_file(config_path);

        // Читаем секцию [database] с использованием .value_or() для значений по умолчанию ---
        const auto& db_config = config["database"];
        m_host     = db_config["host"].value_or("localhost");
        m_port     = db_config["port"].value_or(5432);
        m_dbname   = db_config["dbname"].value_or("chronos");
        m_user     = db_config["user"].value_or("postgres");
        m_password = db_config["password"].value_or(""); // По умолчанию пароль пустой

        // Читаем секцию [storage]
        const auto& storage_config = config["storage"];
        m_attachments_path = storage_config["attachments_path"].value_or(""); // По умолчанию путь пустой

        // Вызываем вспомогательный метод для обработки '~'
        resolveHomeDirectory();

    } catch (const toml::parse_error& e) {
        // Если файл не найден или ошибка парсинга, выводим предупреждение.
        // Поля останутся со значениями по умолчанию (если бы они были в .hpp).
        // Сейчас они просто будут пустыми или 0.
        std::cerr << "Config warning: " << e.what() << ". Using empty/default values." << std::endl;

        // Установим хотя бы хост по умолчанию, чтобы что-то работало
        m_host = "localhost";
        m_port = 5432;
    }
}

// Реализация геттеров

// Собирает строку подключения из сохраненных полей
std::string ConfigParser::getConnStr() const {
    std::stringstream ss;
    ss << "host=" << m_host
       << " port=" << m_port
       << " dbname=" << m_dbname
       << " user=" << m_user
       << " password=" << m_password;
    return ss.str();
}

std::string ConfigParser::getAttachPath() const {
    return m_attachments_path;
}

void ConfigParser::resolveHomeDirectory() {
    if (!m_attachments_path.empty() && m_attachments_path[0] == '~') {
        const char* home_dir = getenv("HOME");
        if (home_dir) {
            // Заменяем тильду на реальный путь к домашней директории
            m_attachments_path.replace(0, 1, home_dir);
        } else {
            // Если HOME не определена, путь может стать невалидным.
            // Лучше сообщить об этом.
            std::cerr << "Warning: Could not resolve '~' in attachments_path, HOME environment variable not set." << std::endl;
        }
    }
}

}
