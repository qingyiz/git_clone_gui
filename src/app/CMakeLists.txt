if(APPLE)
    set(GIT_CLONE_GUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/GitCloneGui.icns")
    set_source_files_properties("${GIT_CLONE_GUI_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    add_executable(GitCloneGui MACOSX_BUNDLE main.cpp "${GIT_CLONE_GUI_ICON}")
elseif(WIN32)
    set(GIT_CLONE_GUI_WINDOWS_ICON
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/GitCloneGui.ico"
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/WindowsResources.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/WindowsResources.rc"
        @ONLY
    )
    add_executable(GitCloneGui WIN32
        main.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/WindowsResources.rc"
        "${GIT_CLONE_GUI_WINDOWS_ICON}"
    )
else()
    add_executable(GitCloneGui main.cpp)
endif()

target_link_libraries(GitCloneGui
    PRIVATE
        git_clone_presentation
        git_clone_infrastructure
        ${QT_PACKAGE}::Widgets
)

target_compile_definitions(GitCloneGui
    PRIVATE
        GIT_CLONE_GUI_VERSION="${PROJECT_VERSION}"
)

set_target_properties(GitCloneGui PROPERTIES
    OUTPUT_NAME "GitCloneGui"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    INSTALL_RPATH_USE_LINK_PATH TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "GitCloneGui"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.mingr.tools.gitclonegui"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE "GitCloneGui.icns"
)

git_clone_enable_warnings(GitCloneGui)

include("${PROJECT_SOURCE_DIR}/cmake/ConfigureAppDeployment.cmake")
git_clone_configure_app_deployment(GitCloneGui)
