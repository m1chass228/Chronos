#pragma once

#include <string>

namespace chronos::api {

// Стандартный ответ от ядра для любого интерфейса.
struct Response {
    bool success;
    std::string message; // Сообщение об ошибке или дополнительная инф

    // Удобные статические методы для создания ответов
    static Response Ok(const std::string& msg = "") {
        return {true, msg};
    }

    static Response Fail(const std::string& error) {
        return {false, error};
    }
};

}
