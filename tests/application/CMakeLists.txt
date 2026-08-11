add_executable(test_clone_controller
    TestCloneController.cpp
)

target_link_libraries(test_clone_controller
    PRIVATE
        git_clone_application
        ${QT_PACKAGE}::Test
)

git_clone_enable_warnings(test_clone_controller)

add_test(NAME clone_controller COMMAND test_clone_controller)
