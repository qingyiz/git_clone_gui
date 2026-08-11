function(git_clone_configure_app_deployment target_name)
    install(TARGETS ${target_name}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )

    if(NOT APPLE AND NOT WIN32)
        return()
    endif()

    if(NOT TARGET ${QT_PACKAGE}::qmake)
        message(FATAL_ERROR
            "当前 Qt kit 未提供 ${QT_PACKAGE}::qmake，无法定位平台部署工具。"
        )
    endif()

    get_target_property(_git_clone_gui_qmake ${QT_PACKAGE}::qmake IMPORTED_LOCATION)
    get_filename_component(_git_clone_gui_qt_bin_dir
        "${_git_clone_gui_qmake}"
        DIRECTORY
    )

    if(APPLE)
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
        return()
    endif()

    find_program(GIT_CLONE_GUI_WINDEPLOYQT
        NAMES windeployqt
        HINTS "${_git_clone_gui_qt_bin_dir}"
        NO_DEFAULT_PATH
    )
    if(NOT GIT_CLONE_GUI_WINDEPLOYQT)
        message(FATAL_ERROR
            "当前 Qt kit 的 bin 目录中未找到 windeployqt：${_git_clone_gui_qt_bin_dir}"
        )
    endif()
    configure_file(
        "${PROJECT_SOURCE_DIR}/cmake/DeployWindows.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/DeployWindows.cmake"
        @ONLY
    )
    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/DeployWindows.cmake")
endfunction()
