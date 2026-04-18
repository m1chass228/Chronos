#pragma once

#include <api/ichronos.hpp>
#include "storage/istorage.hpp"

#include <memory>

namespace chronos::core {

class Application : public api::IChronos {
public:
    Application();
    virtual ~Application() override = default;

    // Реализация методов API
    api::Response addNote(const std::string& content,
                            const std::vector<std::string>& tags,
                            const std::vector<std::string>& attachments) override;

    std::vector<api::NoteDTO> listNotes(const std::vector<std::string>& tags) override;
    api::Response removeNote(const std::vector<std::string>& ids) override;
    std::vector<api::NoteDTO> searchNotes(const std::string& query) override;
    std::optional<api::NoteDTO> getNoteById(const std::string& id) override;

private:
    // Умный указатель на хранилище (инверсия зависимостей)
    std::unique_ptr<storage::IStorage> m_storage;

    // Внутренний метод для конвертации внутренних моделей в DTO для UI
    api::NoteDTO convertToDTO(const domain::Note& note);

    // Распиливатель тегов
    std::vector<std::string> processRawTags(const std::vector<std::string>& rawTags);

};

}
