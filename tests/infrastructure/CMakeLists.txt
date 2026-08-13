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

add_executable(test_navigation_configuration_store
    TestNavigationConfigurationStore.cpp
)

target_link_libraries(test_navigation_configuration_store
    PRIVATE
        git_clone_infrastructure
        ${QT_PACKAGE}::Test
)

git_clone_enable_warnings(test_navigation_configuration_store)

add_test(NAME navigation_configuration_store COMMAND test_navigation_configuration_store)

add_executable(test_workspace_configuration_store
    TestWorkspaceConfigurationStore.cpp
)

target_link_libraries(test_workspace_configuration_store
    PRIVATE
        git_clone_infrastructure
        ${QT_PACKAGE}::Test
)

git_clone_enable_warnings(test_workspace_configuration_store)

add_test(NAME workspace_configuration_store COMMAND test_workspace_configuration_store)

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

add_executable(test_git_workspace
    TestGitWorkspaceService.cpp
)

target_link_libraries(test_git_workspace
    PRIVATE
        git_clone_infrastructure
        ${QT_PACKAGE}::Test
)

git_clone_enable_warnings(test_git_workspace)

add_test(NAME git_workspace COMMAND test_git_workspace)
set_tests_properties(git_workspace PROPERTIES TIMEOUT 30)
