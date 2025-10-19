cmake_minimum_required(VERSION 3.16)
project(AGChallenge LANGUAGES CXX)

# C++ standard and compile commands for clangd
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optionally enforce clang if available via presets or cache
# Users can override compilers via CMakePresets.json or cmake -D...

# Sources
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/AGChallenge)
set(SOURCES
	${SRC_DIR}/Main.cpp
	${SRC_DIR}/Configuration.cpp
	${SRC_DIR}/Evaluation.cpp
	${SRC_DIR}/Evaluator.cpp
	${SRC_DIR}/Optimizer.cpp
	${SRC_DIR}/Timer.cpp
	${SRC_DIR}/Util.cpp
	${SRC_DIR}/CIndividual.cpp
	${SRC_DIR}/Knapsack.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${SRC_DIR})

if(NOT MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Output to bin for convenience
set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Define path to problem files for runtime
set(PROBLEM_FILES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/problem_files)
target_compile_definitions(${PROJECT_NAME} PRIVATE P3_PROBLEM_FOLDER="${PROBLEM_FILES_DIR}/")
set(KP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/instances_01_KP/)
target_compile_definitions(${PROJECT_NAME} PRIVATE KP_INSTANCES_DIR="${KP_DIR}")

# Optionally, copy problem files next to the binary for convenience
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/problem_files
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROBLEM_FILES_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/problem_files
	COMMENT "Copying problem files to runtime directory"
)

# Optionally copy knapsack instances as well
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/instances_01_KP
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${KP_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/instances_01_KP
	COMMENT "Copying knapsack instances to runtime directory"
)

# Option to build with default quick run
option(QUICK_RUN "Enable quick default run" OFF)
if(QUICK_RUN)
	target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_QUICK_RUN)
endif()

