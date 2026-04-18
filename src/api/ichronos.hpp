#pragma once

#include "dto/note_dto.hpp"
#include "dto/response.hpp"

#include <vector>
#include <string>

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
    virtual Response removeNote(const std::string& id) = 0;

    // Поиск по тексту заметок
    virtual std::vector<NoteDTO> searchNotes(const std::string& query) = 0;
};

}
