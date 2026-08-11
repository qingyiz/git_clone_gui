add_executable(test_git_clone_workflow
    TestGitCloneWorkflow.cpp
)

target_link_libraries(test_git_clone_workflow
    PRIVATE
        git_clone_infrastructure
        ${QT_PACKAGE}::Test
)

git_clone_enable_warnings(test_git_clone_workflow)

add_test(NAME git_clone_workflow COMMAND test_git_clone_workflow)
set_tests_properties(git_clone_workflow PROPERTIES TIMEOUT 20)
