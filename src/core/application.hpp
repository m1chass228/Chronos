#pragma once

#include "api/ichronos.hpp"
#include "storage/istorage.hpp"
#include "utils/parser.hpp"        // Полный инклуд, т.к. объект живет в стеке
#include "utils/filemanager.hpp"   // Нужен для unique_ptr и понимания типа
#include "domain/note.hpp"

#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace chronos::core {

class Application : public api::IChronos {
public:
    Application();
    ~Application() override = default;

    // Методы интерфейса
    api::Response addNote(const std::string& content,
                          const std::vector<std::string>& tags,
                          const std::vector<std::string>& attachments) override;

    std::vector<api::NoteDTO> listNotes(const std::vector<std::string>& tags) override;
    api::Response removeNote(const std::vector<std::string>& ids) override;
    std::vector<api::NoteDTO> searchNotes(const std::string& query) override;
    std::optional<api::NoteDTO> getNoteById(const std::string& id) override;
    std::vector<api::NoteDTO> getNotesByAttachment(const std::string& attachment) override;
    api::Response removeAttachment(const std::string& attachment) override;

private:
    // Поля данных
    chronos::parser::ConfigParser m_config;
    std::unique_ptr<storage::IStorage> m_storage;
    std::unique_ptr<chronos::filemanager::FileManager> m_fileManager;

    // Внутренняя кухня
    api::NoteDTO convertToDTO(const domain::Note& note);
    std::vector<std::string> processRawTags(const std::vector<std::string>& rawTags);
};

}