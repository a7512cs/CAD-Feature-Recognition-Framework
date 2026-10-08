cmake_minimum_required(VERSION 3.16)
project(DemoFR LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd / cpptools go-to-definition
add_compile_options(-Wall -Wextra)

# core: dependency graph, ordering, outdated propagation, the six interfaces
add_library(demofr_core
    core/DependencyGraph.cpp
    core/Model.cpp
    core/RecognitionEngine.cpp
)
target_include_directories(demofr_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# features: the recognizers and the (fake) importer
add_library(demofr_features
    features/AAGRecognizer.cpp
    features/FakeGeometryImporter.cpp
    features/FilletRecognizer.cpp
    features/HoleRecognizer.cpp
    features/RibRecognizer.cpp
)
target_link_libraries(demofr_features PUBLIC demofr_core)

# app: console UI + composition root
add_executable(demofr
    app/ConsoleUi.cpp
    app/main.cpp
)
target_link_libraries(demofr PRIVATE demofr_features)

# tests: the executable version of SPEC.md section 7
enable_testing()
add_executable(demofr_tests tests/AcceptanceTests.cpp)
target_link_libraries(demofr_tests PRIVATE demofr_features)
add_test(NAME acceptance COMMAND demofr_tests)
