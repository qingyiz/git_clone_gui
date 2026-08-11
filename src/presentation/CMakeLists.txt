add_library(git_clone_presentation STATIC
    AppStyle.cpp
    AppStyle.h
    BranchSelector.cpp
    BranchSelector.h
    ChildRepositoryCard.cpp
    ChildRepositoryCard.h
    DesktopNotifier.cpp
    DesktopNotifier.h
    MainWindow.cpp
    MainWindow.h
    MainWindowUi.cpp
    NotificationSeverity.h
)

target_include_directories(git_clone_presentation
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(git_clone_presentation
    PUBLIC
        git_clone_application
        git_clone_core
        ${QT_PACKAGE}::Widgets
)

git_clone_enable_warnings(git_clone_presentation)
