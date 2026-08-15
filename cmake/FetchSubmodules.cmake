# Auto-fetches git submodules on first configure if a shallow (non-recursive) clone left them empty.
function(ensure_submodule marker_file)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${marker_file}")
        return()
    endif()

    find_package(Git QUIET)
    if(NOT Git_FOUND)
        message(FATAL_ERROR "Submodule(s) for ${ARGN} are not initialized and git was not found. Run: git submodule update --init --recursive")
    endif()

    message(STATUS "Submodule(s) ${ARGN} are not initialized, fetching them now...")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${ARGN}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE ENSURE_SUBMODULE_RESULT
    )
    if(NOT ENSURE_SUBMODULE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to fetch submodule(s) ${ARGN}. Run manually: git submodule update --init --recursive")
    endif()
endfunction()

set(SDL2_SUBMODULES ThirdParty/SDL2 ThirdParty/SDL2_ttf ThirdParty/SDL2_image ThirdParty/SDL2_mixer)
ensure_submodule(ThirdParty/SDL2/CMakeLists.txt ${SDL2_SUBMODULES})
ensure_submodule(ThirdParty/boost/uuid/CMakeLists.txt ThirdParty/boost)
ensure_submodule(ThirdParty/ser20/CMakeLists.txt ThirdParty/ser20)
ensure_submodule(ThirdParty/googletest/CMakeLists.txt ThirdParty/googletest)
