file(GLOB_RECURSE CORE_SOURCES 
    "*.cpp"
    "domain/*.cpp"
    "storage/*.cpp"
    "utils/*.cpp"
)

add_library(chronos_core STATIC ${CORE_SOURCES})

target_link_libraries(chronos_core PRIVATE uuid)
# Линкуем Postgres и libpqxx
target_link_libraries(chronos_core PRIVATE 
    OpenSSL::Crypto
    CLI11::CLI11
    PostgreSQL::PostgreSQL 
    ${PQXX_LIBRARIES}
)

# Чтобы другие могли подключать хедеры ядра
target_include_directories(chronos_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
