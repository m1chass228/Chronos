#include <api/ichronos.hpp>
#include <api/dto/note_dto.hpp>
#include <api/dto/response.hpp>

#include "../formatter.hpp"

namespace chronos::cli::commands {

void executeAdd(api::IChronos* api, const std::string& content, const std::vector<std::string>& tags, const std::vector<std::string>& attachments) {
    auto response = api->addNote(content, tags, attachments);
    if (response.success) {
        Formatter::success(response.message);
    } else {
        Formatter::error(response.message);
    }
}

}
