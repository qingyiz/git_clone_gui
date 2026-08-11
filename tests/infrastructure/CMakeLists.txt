add_executable(test_configuration_store
    TestConfigurationStore.cpp
)

target_link_libraries(test_configuration_store
    PRIVATE
        git_clone_infrastructure
        ${QT_PACKAGE}::Test
)

git_clone_enable_warnings(test_configuration_store)

add_test(NAME configuration_store COMMAND test_configuration_store)

add_executable(test_git_remote_branches
    TestGitRemoteBranchService.cpp
)

target_link_libraries(test_git_remote_branches
    PRIVATE
        git_clone_infrastructure
        ${QT_PACKAGE}::Test
)

git_clone_enable_warnings(test_git_remote_branches)

add_test(NAME git_remote_branches COMMAND test_git_remote_branches)
