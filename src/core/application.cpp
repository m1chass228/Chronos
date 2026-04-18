#include "application.hpp"
#include "domain/note.hpp"
#include "api/dto/note_dto.hpp"
#include "utils/parser.hpp"

#include <algorithm>
#include <iostream>

namespace chronos::core {

Application::Application() {

    chronos::parser::ConfigParser config_parser("config.toml");

    std::string conn_str = config_parser.getConnStr();

    m_storage = storage::createStorage(conn_str);
    if (m_storage) {
        m_storage->init();
    }
}

// Добавить заметку
api::Response Application::addNote(const std::string& content,
                                   const std::vector<std::string>& tags,
                                   const std::vector<std::string>& attachments)
{
    try {
        // 1. Создаем объект (Рождение новой заметки)
        domain::Note note(content, tags, attachments);

        // 2. Валидация (простая проверка)
        if (content.empty()) {
            return api::Response::Fail("Note content cannot be empty");
        }

        // 3. Сохраняем в базу через интерфейс IStorage
        if (m_storage && m_storage->saveNote(note)) {
            return api::Response::Ok("Note added with ID: " + note.getId());
        }

        return api::Response::Fail("Failed to save note to database");
    } catch (const std::exception& e) {
        return api::Response::Fail(e.what());
    }
}

// 2. Получить список
std::vector<api::NoteDTO> Application::listNotes(const std::vector<std::string>& tags) {
    std::vector<api::NoteDTO> dtos;
    if (!m_storage) return dtos;

    // Просто передаем вектор тегов дальше в storage
    auto notes = m_storage->getNotes(tags);

    std::transform(notes.begin(), notes.end(), std::back_inserter(dtos),
        [this](const domain::Note& n) { return convertToDTO(n); });

    return dtos;
}

// 3. Удалить заметку
api::Response Application::removeNote(const std::string& id) {
    try {
        if (!m_storage) return api::Response::Fail("Storage not initialized");

        // Пытаемся удалить по ShortID (удобство для CLI)
        if (m_storage->deleteNoteByShortId(id)) {
            return api::Response::Ok("Note removed successfully");
        }

        return api::Response::Fail("Note not found or ID is ambiguous");
    } catch (const std::exception& e) {
        return api::Response::Fail(e.what());
    }
}

// 4. Искать заметку
std::vector<api::NoteDTO> Application::searchNotes(const std::string& query) {
    std::vector<api::NoteDTO> dtos;
    if (!m_storage || query.empty()) return dtos;

    auto notes = m_storage->search(query);

    for (const auto& n : notes) {
        dtos.push_back(convertToDTO(n));
    }

    return dtos;
}

// Вспомогательный метод для превращения внутренней модели в DTO для CLI
api::NoteDTO Application::convertToDTO(const domain::Note& note) {
    return api::NoteDTO {
        .id = note.getId(),
        .content = note.getContent(),
        .created_at = note.getCreatedAsString(),
        .updated_at = note.getUpdatedAsString(),
        .attachments = note.getAttachments(),
        .tags = note.getTags()
    };
}

}
