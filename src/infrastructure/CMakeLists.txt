add_library(git_clone_infrastructure STATIC
    GitProcessRunner.cpp
    GitProcessRunner.h
    QSettingsConfigurationStore.cpp
    QSettingsConfigurationStore.h
)

target_include_directories(git_clone_infrastructure
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(git_clone_infrastructure
    PUBLIC
        git_clone_application
    PRIVATE
        ${QT_PACKAGE}::Core
)

git_clone_enable_warnings(git_clone_infrastructure)
