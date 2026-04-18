#include <api/ichronos.hpp>
#include <api/dto/note_dto.hpp>
#include <api/dto/response.hpp>

#include "../formatter.hpp"

namespace chronos::cli::commands {

void executeSearch(api::IChronos* api, const std::string& query) {
    auto notes = api->searchNotes(query);
    Formatter::list(notes); // Используем тот же форматтер
}

}
