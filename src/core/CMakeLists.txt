add_library(git_clone_core STATIC
    CloneRequest.cpp
    CloneRequest.h
)

target_include_directories(git_clone_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(git_clone_core
    PUBLIC
        ${QT_PACKAGE}::Core
)

git_clone_enable_warnings(git_clone_core)
