#include "application.hpp"
#include "api/dto/note_dto.hpp"
#include "storage/istorage.hpp" // Нужен здесь, чтобы знать, как создавать хранилище

#include <algorithm>
#include <sstream>
#include <format>

namespace chronos::core {

Application::Application() : m_config("config.toml") {
    std::string conn_str = m_config.getConnStr();
    m_storage = storage::createStorage(conn_str);

    if (m_storage) {
        m_storage->init();
    }

    m_fileManager = std::make_unique<filemanager::FileManager>(m_config.getAttachPath());
}

api::Response Application::addNote(const std::string& content,
                                   const std::vector<std::string>& tags,
                                   const std::vector<std::string>& attachments) 
{
    try {
        if (content.empty()) return api::Response::Fail("Note content cannot be empty");

        auto cleanTags = processRawTags(tags);
        // Хешируем и сохраняем файлы, получаем строку путей для БД
        std::vector<std::string> savedPaths = m_fileManager->storeAttachments(attachments);

        domain::Note note(content, cleanTags, savedPaths);

        if (m_storage && m_storage->saveNote(note)) {
            return api::Response::Ok("Note added with ID: " + note.getId());
        }
        return api::Response::Fail("Failed to save note to database");
    } catch (const std::exception& e) {
        return api::Response::Fail(e.what());
    }
}

std::vector<api::NoteDTO> Application::listNotes(const std::vector<std::string>& tags) {
    std::vector<api::NoteDTO> dtos;
    if (!m_storage) return dtos;

    auto cleanTags = processRawTags(tags);
    auto notes = m_storage->getNotes(cleanTags);

    std::transform(notes.begin(), notes.end(), std::back_inserter(dtos),
        [this](const domain::Note& n) { return convertToDTO(n); });

    return dtos;
}

api::Response Application::removeNote(const std::vector<std::string>& ids) {
    if (!m_storage) return api::Response::Fail("Storage not initialized");

    size_t deletedCount = 0;
    for (const auto& id : ids) {
        if (m_storage->deleteNoteByShortId(id)) deletedCount++;
    }

    if (deletedCount == 0) return api::Response::Fail("No notes found");
    return api::Response::Ok("Removed " + std::to_string(deletedCount) + " notes");
}

std::vector<api::NoteDTO> Application::getNotesByAttachment(const std::string& attachment) {
    if (attachment.empty() || !m_storage) {
        return {};
    }

    auto notes = m_storage->getNotesByAttachment(attachment);
    
    std::vector<api::NoteDTO> dtos;
    dtos.reserve(notes.size()); // Оптимизация аллокации
    for (const auto& n : notes) {
        dtos.push_back(convertToDTO(n));
    }
    return dtos;
}

api::Response Application::removeAttachment(const std::string& attachment) {
    if (attachment.empty()) {
        return api::Response::Fail("Attachment path is empty");
    }
    if (!m_storage) {
        return api::Response::Fail("Storage not initialized");
    }
    
    // 1. Пытаемся удалить запись из базы данных
    if (!m_storage->deleteAttachment(attachment)) {
        return api::Response::Fail("Failed to remove attachment record from database");
    }

    // 2. Если из базы удалили успешно, пробуем удалить файл физически
    if (m_fileManager->removeAttachment(attachment)) {
        return api::Response::Ok("Attachment successfully removed from disk and database");
    } else {
        // Ситуация "мягкой" ошибки: в базе записи нет, но файл почему-то не удалился
        // (например, его уже удалили руками или занят другим процессом)
        return api::Response::Fail("Database record deleted, but failed to remove physical file from disk");
    }
}

std::vector<api::NoteDTO> Application::searchNotes(const std::string& query) {
    std::vector<api::NoteDTO> dtos;
    if (!m_storage || query.empty()) return dtos;

    auto notes = m_storage->search(query);
    for (const auto& n : notes) dtos.push_back(convertToDTO(n));
    return dtos;
}

std::optional<api::NoteDTO> Application::getNoteById(const std::string& id) {
    if (!m_storage) return std::nullopt;
    auto note = m_storage->getNoteById(id);
    return note ? std::make_optional(convertToDTO(*note)) : std::nullopt;
}

api::NoteDTO Application::convertToDTO(const domain::Note& note) {
    return api::NoteDTO{
        .id = note.getId(),
        .content = note.getContent(),
        .created_at = std::format("{:%Y-%m-%d %H:%M:%S}", note.getCreatedAt()),
        .attachments = note.getAttachments(),
        .tags = note.getTags(),
        .updated_at = "" // Добавь это, чтобы убрать warning о missing field
    };
}

std::vector<std::string> Application::processRawTags(const std::vector<std::string>& rawTags) {
    std::vector<std::string> processed;
    for (const auto& tagStr : rawTags) {
        std::stringstream ss(tagStr);
        std::string segment;
        while (std::getline(ss, segment, ',')) {
            segment.erase(0, segment.find_first_not_of(" \t\n\r#"));
            auto last = segment.find_last_not_of(" \t\n\r");
            if (last != std::string::npos) segment.erase(last + 1);
            if (!segment.empty()) processed.push_back(segment);
        }
    }
    return processed;
}

} // namespace chronos::core