add_executable(test_buffer test_buffer.c)
target_link_libraries(test_buffer PRIVATE qicto_core)
add_test(NAME test_buffer COMMAND test_buffer)

add_executable(test_editor test_editor.c)
target_link_libraries(test_editor PRIVATE qicto_core)
add_test(NAME test_editor COMMAND test_editor)

add_executable(test_commands test_commands.c)
target_link_libraries(test_commands PRIVATE qicto_core)
add_test(NAME test_commands COMMAND test_commands)

add_executable(test_layout test_layout.c)
target_link_libraries(test_layout PRIVATE qicto_core)
add_test(NAME test_layout COMMAND test_layout)
