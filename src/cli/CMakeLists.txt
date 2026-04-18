file(GLOB COMMAND_SOURCES "commands/*.cpp")

add_executable(chronos 
    main.cpp 
    formatter.cpp
    ${COMMAND_SOURCES}
)

# Линкуем ядро (оно уже само тянет Postgres)
target_link_libraries(chronos PRIVATE chronos_core)


