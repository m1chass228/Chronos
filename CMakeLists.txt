cmake_minimum_required(VERSION 3.20)
project(Chronos VERSION 0.1.0 LANGUAGES CXX)

# --- Настройка Тулчейна (Clang + LLD + Ninja) ---
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Проверяем наличие LLD
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} "-fuse-ld=lld" "-Wl,--version" 
                    ERROR_VARIABLE LLD_ERR 
                    RESULT_VARIABLE LLD_RESULT)
    
    if(LLD_RESULT EQUAL 0)
        add_link_options("-fuse-ld=lld")
        message(STATUS "High-performance trio enabled: Clang + LLD + ${CMAKE_GENERATOR}")
    else()
        message(STATUS "LLD not found, falling back to default linker")
    endif()
else()
    message(STATUS "Clang not detected. Using default compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# --- Общие настройки ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Оптимизации для Clang (если используем его)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -fcolor-diagnostics)
endif()

# --- Зависимости ---
find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# Пути для заголовочных файлов (external для toml++)
include_directories(external)

include(FetchContent)

FetchContent_Declare(
  cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG        v2.4.2  # Актуальная версия
)
FetchContent_MakeAvailable(cli11)

# --- Модули ---
include_directories(src) # Добавь это рядом с include_directories(external)
add_subdirectory(src/core)
add_subdirectory(src/cli)
# add_subdirectory(src/tui)
