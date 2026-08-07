# SPDX-FileCopyrightText: 2026 HIROAKI KAWAKITA
# SPDX-License-Identifier: MIT

# Inputs:
# - CLI_EXEC: Path to lsw_audio_diagnostics_cli
# - FIXTURE_GEN_EXEC: Path to lsw_audio_diagnostics_cli_fixture_generator

set(TEST_FAILED FALSE)

function(run_cli_case name expected_exit_code expected_stdout_regex expected_stderr_regex)
    execute_process(
        COMMAND ${CLI_EXEC} ${ARGN}
        RESULT_VARIABLE res
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )

    if(NOT res EQUAL expected_exit_code)
        message(WARNING "Test '${name}' failed: Expected exit code ${expected_exit_code}, got ${res}\nCommand: ${CLI_EXEC} ${ARGN}\nStdout: ${out}\nStderr: ${err}")
        set(TEST_FAILED TRUE PARENT_SCOPE)
    endif()

    if(expected_stdout_regex AND NOT out MATCHES "${expected_stdout_regex}")
        message(WARNING "Test '${name}' failed: Stdout did not match regex '${expected_stdout_regex}'\nStdout: ${out}")
        set(TEST_FAILED TRUE PARENT_SCOPE)
    endif()

    if(expected_stderr_regex AND NOT err MATCHES "${expected_stderr_regex}")
        message(WARNING "Test '${name}' failed: Stderr did not match regex '${expected_stderr_regex}'\nStderr: ${err}")
        set(TEST_FAILED TRUE PARENT_SCOPE)
    endif()
endfunction()

function(run_cli_case_no_args name expected_exit_code expected_stdout_regex expected_stderr_regex)
    execute_process(
        COMMAND ${CLI_EXEC}
        RESULT_VARIABLE res
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )

    if(NOT res EQUAL expected_exit_code)
        message(WARNING "Test '${name}' failed: Expected exit code ${expected_exit_code}, got ${res}\nCommand: ${CLI_EXEC}\nStdout: ${out}\nStderr: ${err}")
        set(TEST_FAILED TRUE PARENT_SCOPE)
    endif()

    if(expected_stdout_regex AND NOT out MATCHES "${expected_stdout_regex}")
        message(WARNING "Test '${name}' failed: Stdout did not match regex '${expected_stdout_regex}'\nStdout: ${out}")
        set(TEST_FAILED TRUE PARENT_SCOPE)
    endif()

    if(expected_stderr_regex AND NOT err MATCHES "${expected_stderr_regex}")
        message(WARNING "Test '${name}' failed: Stderr did not match regex '${expected_stderr_regex}'\nStderr: ${err}")
        set(TEST_FAILED TRUE PARENT_SCOPE)
    endif()
endfunction()

message(STATUS "Running CLI E2E tests...")

# 1. --help exit 0
run_cli_case("Help" 0 "Usage:.*" "" --help)

# 2. --version exit 0
run_cli_case("Version" 0 "0\\.3\\.0.*" "" --version)

# 3. missing argument exit 2 (using dedicated 0-args function)
run_cli_case_no_args("MissingArgumentNoArgs" 2 "Usage:.*" "")

# 4. unknown option exit 2
run_cli_case("UnknownOption" 2 "" "Error: Unknown or extra argument.*" analyze input.wav --unknown)

# 5. missing input file exit 3
run_cli_case("MissingInput" 3 "" "Error: Failed to open input file" analyze missing_input_file_path_does_not_exist.wav)

# Generate fixtures
execute_process(COMMAND ${FIXTURE_GEN_EXEC} valid_mono mono.wav)
execute_process(COMMAND ${FIXTURE_GEN_EXEC} valid_stereo stereo.wav)
execute_process(COMMAND ${FIXTURE_GEN_EXEC} partial_4097 partial.wav)
execute_process(COMMAND ${FIXTURE_GEN_EXEC} malformed malformed.wav)
execute_process(COMMAND ${FIXTURE_GEN_EXEC} valid_mono "path with space.wav")

# 6. malformed WAV exit 4
run_cli_case("MalformedWav" 4 "" "Error: Malformed or unsupported WAV" analyze malformed.wav)

# 7. output failure exit 6
run_cli_case("OutputFailure" 6 "" "Error: Failed to create temporary output file" analyze mono.wav --output z:/invalid/path/out.json)

# 8. valid compact stdout
run_cli_case("ValidCompact" 0 "\"schemaVersion\":1.*\"tool\":{\"name\":\"lsw_audio_diagnostics_cli\",\"version\":\"0.3.0\"}.*" "" analyze mono.wav)

# 9. valid pretty stdout
run_cli_case("ValidPretty" 0 "{\n  \"schemaVersion\": 1,.*" "" analyze mono.wav --pretty)

# 10. --output success (stdout and stderr MUST be empty)
execute_process(COMMAND ${CLI_EXEC} analyze mono.wav --output out.json RESULT_VARIABLE r OUTPUT_VARIABLE o ERROR_VARIABLE e)
if(NOT r EQUAL 0 OR NOT o STREQUAL "" OR NOT e STREQUAL "")
    message(WARNING "Test 'OutputSuccess' failed")
    set(TEST_FAILED TRUE)
endif()

# 11. partial block E2E check
run_cli_case("PartialBlock" 0 "\"processedFrameCount\":4097.*\"processedBlockCount\":2.*" "" analyze partial.wav --block-size 4096)

# 12. block-size invariance with normalized processedBlockCount
execute_process(COMMAND ${CLI_EXEC} analyze partial.wav --block-size 1 OUTPUT_VARIABLE out_bs1)
execute_process(COMMAND ${CLI_EXEC} analyze partial.wav --block-size 4096 OUTPUT_VARIABLE out_bs4096)

string(REGEX REPLACE "\"processedBlockCount\":[ ]*[0-9]+" "\"processedBlockCount\":0" norm_bs1 "${out_bs1}")
string(REGEX REPLACE "\"processedBlockCount\":[ ]*[0-9]+" "\"processedBlockCount\":0" norm_bs4096 "${out_bs4096}")

if(NOT norm_bs1 STREQUAL norm_bs4096)
    message(WARNING "Test 'BlockSizeInvariance' failed: normalized outputs differ")
    set(TEST_FAILED TRUE)
endif()

# 13. same input/output protection & SHA256 check
file(SHA256 mono.wav sha_before)
execute_process(COMMAND ${CLI_EXEC} analyze mono.wav --output mono.wav RESULT_VARIABLE r_same)
file(SHA256 mono.wav sha_after)
if(NOT r_same EQUAL 6 OR NOT sha_before STREQUAL sha_after)
    message(WARNING "Test 'SameInputOutput' failed: path protection or SHA256 integrity check failed")
    set(TEST_FAILED TRUE)
endif()

# 14. existing output replacement
file(WRITE existing_out.json "stale_content")
execute_process(COMMAND ${CLI_EXEC} analyze mono.wav --output existing_out.json RESULT_VARIABLE r_rep)
file(READ existing_out.json rep_content)
if(NOT r_rep EQUAL 0 OR rep_content STREQUAL "stale_content" OR NOT rep_content MATCHES "\"schemaVersion\":1")
    message(WARNING "Test 'ExistingOutputReplacement' failed")
    set(TEST_FAILED TRUE)
endif()

# 15. pre-output failure preserves existing output file
file(WRITE orig_out.json "original_content")
execute_process(COMMAND ${CLI_EXEC} analyze missing_input_file.wav --output orig_out.json RESULT_VARIABLE r_fail)
file(READ orig_out.json fail_content)
if(NOT r_fail EQUAL 3 OR NOT fail_content STREQUAL "original_content")
    message(WARNING "Test 'PreOutputFailurePreservesExistingOutput' failed")
    set(TEST_FAILED TRUE)
endif()

# 16. path with spaces
execute_process(
    COMMAND ${CLI_EXEC} analyze "path with space.wav" --output "out with space.json"
    RESULT_VARIABLE res_space
    OUTPUT_VARIABLE out_space
    ERROR_VARIABLE err_space
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
)
if(NOT res_space EQUAL 0 OR NOT out_space STREQUAL "")
    message(WARNING "Test 'PathWithSpace' failed: exit code ${res_space}, stdout: ${out_space}, stderr: ${err_space}")
    set(TEST_FAILED TRUE)
endif()

if(TEST_FAILED)
    message(FATAL_ERROR "One or more CLI E2E tests failed.")
else()
    message(STATUS "All CLI E2E tests passed.")
endif()
