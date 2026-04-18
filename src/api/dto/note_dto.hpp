#pragma once
#include <string>
#include <vector>

namespace chronos::api {

// Объект передачи данных заметки
struct NoteDTO {
    std::string id;
    std::string content;
    std::string created_at;
    std::string updated_at;
    std::vector<std::string> attachments;
    std::vector<std::string> tags;
};

}
