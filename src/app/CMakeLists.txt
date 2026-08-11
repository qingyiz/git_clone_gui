if(APPLE)
    set(GIT_CLONE_GUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/GitCloneGui.icns")
    set_source_files_properties("${GIT_CLONE_GUI_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    add_executable(GitCloneGui MACOSX_BUNDLE main.cpp "${GIT_CLONE_GUI_ICON}")
else()
    add_executable(GitCloneGui WIN32 main.cpp)
endif()

target_link_libraries(GitCloneGui
    PRIVATE
        git_clone_presentation
        git_clone_infrastructure
        ${QT_PACKAGE}::Widgets
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

install(TARGETS GitCloneGui
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

if(APPLE)
    if(NOT TARGET ${QT_PACKAGE}::qmake)
        message(FATAL_ERROR "当前 Qt kit 未提供 ${QT_PACKAGE}::qmake，无法定位 macdeployqt。")
    endif()

    get_target_property(_git_clone_gui_qmake ${QT_PACKAGE}::qmake IMPORTED_LOCATION)
    get_filename_component(_git_clone_gui_qt_bin_dir
        "${_git_clone_gui_qmake}"
        DIRECTORY
    )
    find_program(GIT_CLONE_GUI_MACDEPLOYQT
        NAMES macdeployqt
        HINTS "${_git_clone_gui_qt_bin_dir}"
        NO_DEFAULT_PATH
    )

    if(NOT GIT_CLONE_GUI_MACDEPLOYQT)
        message(FATAL_ERROR
            "当前 Qt kit 的 bin 目录中未找到 macdeployqt：${_git_clone_gui_qt_bin_dir}"
        )
    endif()

    configure_file(
        "${PROJECT_SOURCE_DIR}/cmake/DeployMacOS.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/DeployMacOS.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/DeployMacOS.cmake")
endif()
