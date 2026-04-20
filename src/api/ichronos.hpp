#pragma once

#include "dto/note_dto.hpp"
#include "dto/response.hpp"

#include <vector>
#include <string>
#include <optional>

namespace chronos::api {

class IChronos {
public:
    virtual ~IChronos() = default;

    // Chronos add
    virtual Response addNote(const std::string& content,
                                 const std::vector<std::string>& tags = {},
                                 const std::vector<std::string>& attachments = {}) = 0;

    // Chronos ls
    virtual std::vector<NoteDTO> listNotes(const std::vector<std::string>& tags) = 0;

    // Сhronos rm <id>
    virtual Response removeNote(const std::vector<std::string>& ids) = 0;

    // Поиск по тексту заметок
    virtual std::vector<NoteDTO> searchNotes(const std::string& query) = 0;

    // получение заметок
    virtual std::optional<api::NoteDTO> getNoteById(const std::string& id) = 0;
    
    // получение заметок по аттачменту
    virtual std::vector<NoteDTO> getNotesByAttachment(const std::string& attachment) = 0;
    
    // удаление аттачмента в заметках
    virtual Response removeAttachment(const std::string& attachment) = 0;

};

}
