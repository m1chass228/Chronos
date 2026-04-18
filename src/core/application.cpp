#include "application.hpp"
#include "domain/note.hpp"
#include "api/dto/note_dto.hpp"
#include "utils/parser.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

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
        if (content.empty()) return api::Response::Fail("Note content cannot be empty");

        // Распиливаем склеенные теги перед созданием объекта
        auto cleanTags = processRawTags(tags);

        domain::Note note(content, cleanTags, attachments);

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

    // Распиливаем теги и здесь, чтобы поиск работал по "success",
    // даже если юзер ввел "success,linux"
    auto cleanTags = processRawTags(tags);
    auto notes = m_storage->getNotes(cleanTags);

    std::transform(notes.begin(), notes.end(), std::back_inserter(dtos),
        [this](const domain::Note& n) { return convertToDTO(n); });

    return dtos;
}

// 3. Удалить заметку
api::Response Application::removeNote(const std::vector<std::string>& ids) {
    try {
        if (!m_storage) return api::Response::Fail("Storage not initialized");

        size_t deletedCount = 0;
        std::vector<std::string> failedIds;

        for (const auto& id : ids) {
            // Пытаемся удалить по ShortID (удобство для CLI)
            if (m_storage->deleteNoteByShortId(id)) {
                deletedCount++;
            } else {
                failedIds.push_back(id);
            }
        }

        if (deletedCount == ids.size()) {
            return api::Response::Ok("All requested notes removed successfully (" + std::to_string(deletedCount) + ")");
        } else if (deletedCount > 0) {
            return api::Response::Ok("Partially removed: " + std::to_string(deletedCount) + " ok, " + std::to_string(failedIds.size()) + " failed");
        } else {
            return api::Response::Fail("No notes were found with provided IDs");
        }

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

// --- Вспомогательный метод (можно добавить в приватную секцию application.hpp) ---
std::vector<std::string> Application::processRawTags(const std::vector<std::string>& rawTags) {
    std::vector<std::string> processed;
    for (const auto& tagStr : rawTags) {
        std::stringstream ss(tagStr);
        std::string segment;

        while (std::getline(ss, segment, ',')) {
            // 1. Убираем пробелы по краям
            segment.erase(0, segment.find_first_not_of(" \t\n\r"));
            auto last = segment.find_last_not_of(" \t\n\r");
            if (last != std::string::npos) {
                segment.erase(last + 1);
            }

            // 2. Убираем # в начале, если он есть
            if (!segment.empty() && segment[0] == '#') {
                segment.erase(0, 1);
            }

            // 3. Снова чистим пробелы (на случай "# linux")
            segment.erase(0, segment.find_first_not_of(" \t\n\r"));

            if (!segment.empty()) {
                processed.push_back(segment);
            }
        }
    }
    return processed;
}

std::optional<api::NoteDTO> Application::getNoteById(const std::string& id) {
    if (!m_storage) return std::nullopt;

    auto note = m_storage->getNoteById(id);
    if (note) {
        return convertToDTO(*note);
    }
    return std::nullopt;
}

}
