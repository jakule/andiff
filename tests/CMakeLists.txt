add_test(NAME SanityCheck
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/sanity_check.py
        --diff $<TARGET_FILE:${DIFF_EXE_NAME}>
        --patch $<TARGET_FILE:${PATCH_EXE_NAME}>
        --size 1)

# TODO(JN): Fix failing testcase!!!
#add_test(NAME SanityCheckEmpty
#        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/sanity_check.py
#        --diff $<TARGET_FILE:${DIFF_EXE_NAME}>
#        --patch $<TARGET_FILE:${PATCH_EXE_NAME}>
#        --size 0)