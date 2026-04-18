#include <api/ichronos.hpp>
#include <api/dto/note_dto.hpp>
#include <api/dto/response.hpp>

#include "../formatter.hpp"

namespace chronos::cli::commands {

void executeList(api::IChronos* api, const std::vector<std::string>& tags) {
    // Просто передаем вектор тегов дальше в ядро
    auto notes = api->listNotes(tags);
    Formatter::list(notes);
}

}
