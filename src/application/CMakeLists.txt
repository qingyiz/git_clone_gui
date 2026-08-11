add_library(git_clone_application STATIC
    CloneController.cpp
    CloneController.h
    ConfigurationStore.h
    ProcessRunner.h
    RemoteBranchService.h
)

target_include_directories(git_clone_application
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(git_clone_application
    PUBLIC
        git_clone_core
        ${QT_PACKAGE}::Core
)

git_clone_enable_warnings(git_clone_application)
