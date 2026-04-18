#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace chronos::core::domain {

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;

class Note {
public:
    // Конструктор для новых заметок (генерирует ID и время автоматически)
    Note(std::string content,
         std::vector<std::string> tags = {},
         std::vector<std::string> attachments = {});

    // Конструктор для восстановления из БД (когда ID и время уже известны)
    Note(std::string id,
        std::string content,
        TimePoint created,
        TimePoint updated,
        std::vector<std::string> tags,
        std::vector<std::string> attachments);

    // Геттеры
    std::string getId() const { return m_id; }
    std::string getContent() const { return m_content; }
    TimePoint getCreatedAt() const { return m_createdAt; }
    TimePoint getUpdatedAt() const { return m_updatedAt; }
    const std::vector<std::string>& getTags() const { return m_tags; }
    const std::vector<std::string>& getAttachments() const { return m_attachments; }

    // Методы логики
    void updateContent(const std::string& newContent);
    bool isValid() const; // Например проверка что контент не пустой

    // Вспомогательный метод для конвертации времени в строку (для DTO)
    std::string getCreatedAsString() const;
    std::string getUpdatedAsString() const;

private:
    std::string m_id;
    std::string m_content;
    TimePoint m_createdAt;
    TimePoint m_updatedAt;
    std::vector<std::string> m_tags;
    std::vector<std::string> m_attachments;
    std::string generateUuid();
};
}
