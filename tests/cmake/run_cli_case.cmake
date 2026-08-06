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

message(STATUS "Running CLI E2E tests...")

# 1. --help exit 0
run_cli_case("Help" 0 "Usage:.*" "" --help)

# 2. --version exit 0
run_cli_case("Version" 0 "0\\.3\\.0.*" "" --version)

# 3. missing argument exit 2
run_cli_case("MissingArgument" 2 "Usage:.*" "" "")

# 4. unknown option exit 2
run_cli_case("UnknownOption" 2 "" "Error: Unknown or extra argument.*" analyze input.wav --unknown)

# 5. missing input file exit 3
run_cli_case("MissingInput" 3 "" "Error: Failed to open input file" analyze missing_input_file_path_does_not_exist.wav)

# Generate fixtures
execute_process(COMMAND ${FIXTURE_GEN_EXEC} valid_mono mono.wav)
execute_process(COMMAND ${FIXTURE_GEN_EXEC} valid_stereo stereo.wav)
execute_process(COMMAND ${FIXTURE_GEN_EXEC} malformed malformed.wav)

# 6. malformed WAV exit 4
run_cli_case("MalformedWav" 4 "" "Error: Malformed or unsupported WAV" analyze malformed.wav)

# 7. output failure exit 6
run_cli_case("OutputFailure" 6 "" "Error: Failed to create output file" analyze mono.wav --output z:/invalid/path/out.json)

# 8. valid compact stdout
run_cli_case("ValidCompact" 0 "\"schemaVersion\":1.*\"tool\":{\"name\":\"lsw_audio_diagnostics_cli\",\"version\":\"0.3.0\"}.*" "" analyze mono.wav)

# 9. valid pretty stdout
run_cli_case("ValidPretty" 0 "{\n  \"schemaVersion\": 1,.*" "" analyze mono.wav --pretty)

# 10. --output success
execute_process(COMMAND ${CLI_EXEC} analyze mono.wav --output out.json RESULT_VARIABLE r OUTPUT_VARIABLE o ERROR_VARIABLE e)
if(NOT r EQUAL 0 OR NOT o STREQUAL "" OR NOT e STREQUAL "")
    message(WARNING "Test 'OutputSuccess' failed")
    set(TEST_FAILED TRUE)
endif()

# 11. deterministic output
execute_process(COMMAND ${CLI_EXEC} analyze mono.wav OUTPUT_VARIABLE out1)
execute_process(COMMAND ${CLI_EXEC} analyze mono.wav OUTPUT_VARIABLE out2)
if(NOT out1 STREQUAL out2)
    message(WARNING "Test 'Deterministic' failed: outputs differ")
    set(TEST_FAILED TRUE)
endif()

# 12. same input/output rejection
run_cli_case("SameInputOutput" 6 "" "Error: Input and output paths.*" analyze mono.wav --output mono.wav)

if(TEST_FAILED)
    message(FATAL_ERROR "One or more CLI E2E tests failed.")
else()
    message(STATUS "All CLI E2E tests passed.")
endif()
