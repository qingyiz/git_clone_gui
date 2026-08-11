add_executable(test_presentation
    TestPresentation.cpp
)

target_link_libraries(test_presentation
    PRIVATE
        git_clone_presentation
        ${QT_PACKAGE}::Test
)

git_clone_enable_warnings(test_presentation)

add_test(NAME presentation COMMAND test_presentation)
