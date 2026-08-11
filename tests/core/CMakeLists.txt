add_executable(test_clone_core
    TestCloneRequest.cpp
)

target_link_libraries(test_clone_core
    PRIVATE
        git_clone_core
        ${QT_PACKAGE}::Test
)

git_clone_enable_warnings(test_clone_core)

add_test(NAME clone_core COMMAND test_clone_core)
