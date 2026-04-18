#include <api/ichronos.hpp>
#include <api/dto/note_dto.hpp>
#include <api/dto/response.hpp>

#include "../formatter.hpp"

namespace chronos::cli::commands {

void executeAdd(api::IChronos* api, const std::string& content, const std::vector<std::string>& tags) {
    auto response = api->addNote(content, tags, {});
    if (response.success) {
        Formatter::success(response.message);
    } else {
        Formatter::error(response.message);
    }
}

}
