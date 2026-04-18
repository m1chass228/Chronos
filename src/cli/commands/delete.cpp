#include <api/ichronos.hpp>
#include <api/dto/note_dto.hpp>
#include <api/dto/response.hpp>

#include "../formatter.hpp"

namespace chronos::cli::commands {

void executeRemove(api::IChronos* api, const std::string& id) {
    auto response = api->removeNote(id);
    if (response.success) {
        Formatter::success("Note removed.");
    } else {
        Formatter::error(response.message);
    }
}

}
