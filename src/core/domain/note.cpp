#include "note.hpp"

#include <uuid/uuid.h>

#include <iomanip>
#include <sstream>

namespace chronos::core::domain {

// Конструктор 1: Новая заметка
Note::Note(std::string content, std::vector<std::string> tags, std::vector<std::string> attachments)
    : m_content(std::move(content)), m_tags(std::move(tags)), m_attachments(std::move(attachments))
{
    m_id = generateUuid();
    m_createdAt = Clock::now();
    m_updatedAt = m_createdAt;
}

// Конструктор 2: Воскрешение из БД
Note::Note(std::string id, std::string content, TimePoint created, TimePoint updated,
           std::vector<std::string> tags, std::vector<std::string> attachments)
    : m_id(std::move(id)), m_content(std::move(content)),
      m_createdAt(created), m_updatedAt(updated),
      m_tags(std::move(tags)), m_attachments(std::move(attachments)) {}

void Note::updateContent(const std::string& newContent) {
    if (m_content != newContent) {
        m_content = newContent;
        m_updatedAt = Clock::now();
    }
}

std::string Note::getCreatedAsString() const {
    auto timer = Clock::to_time_t(m_createdAt);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timer), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Note::getUpdatedAsString() const {
    auto timer = Clock::to_time_t(m_updatedAt);
    std::stringstream ss;
    // Используем тот же формат для консистентности в базе и DTO
    ss << std::put_time(std::localtime(&timer), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Note::generateUuid() {
    uuid_t b_uuid;
    uuid_generate_random(b_uuid);
    char str[37];
    uuid_unparse_lower(b_uuid, str);
    return std::string(str);
}

}
