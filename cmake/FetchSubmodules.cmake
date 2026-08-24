# Auto-fetches git submodules on first configure if a shallow (non-recursive) clone left them empty.
# Collected into lists first: the groups below are independent, and a separate `git submodule update`
# per group would run them back to back, each with its own parallel pool.
set(SUBMODULES_RECURSIVE "") # only SDL2_ttf actually has nested submodules (freetype, harfbuzz)
set(SUBMODULES_FLAT "")      # SDL2_image/SDL2_mixer nest ~760 MB of codecs we never build, see below

macro(need_submodule out_var marker_file)
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/${marker_file}")
        list(APPEND ${out_var} ${ARGN})
    endif ()
endmacro()

set(SDL2_SUBMODULES ThirdParty/SDL2 ThirdParty/SDL2_ttf ThirdParty/SDL2_image ThirdParty/SDL2_mixer)
need_submodule(SUBMODULES_RECURSIVE ThirdParty/SDL2/CMakeLists.txt ThirdParty/SDL2)
need_submodule(SUBMODULES_RECURSIVE ThirdParty/SDL2_ttf/CMakeLists.txt ThirdParty/SDL2_ttf)
need_submodule(SUBMODULES_RECURSIVE ThirdParty/boost/uuid/CMakeLists.txt ThirdParty/boost)
need_submodule(SUBMODULES_RECURSIVE ThirdParty/ser20/CMakeLists.txt ThirdParty/ser20)
need_submodule(SUBMODULES_RECURSIVE ThirdParty/googletest/CMakeLists.txt ThirdParty/googletest)
# SDL2IMAGE_VENDORED/SDL2MIXER_VENDORED are pinned OFF, so their external/* (dav1d, aom, libjxl,
# mpg123, fluidsynth with its test soundfonts, ...) is never configured - don't clone it either.
need_submodule(SUBMODULES_FLAT ThirdParty/SDL2_image/CMakeLists.txt ThirdParty/SDL2_image)
need_submodule(SUBMODULES_FLAT ThirdParty/SDL2_mixer/CMakeLists.txt ThirdParty/SDL2_mixer)

function(fetch_submodules recurse)
    if (NOT ARGN)
        return()
    endif ()

    #NOTE: --depth 1 here, not `shallow = true` in .gitmodules - that one is a recommendation git
    # drops back to a full clone whenever the pin is not the branch tip, which is our five biggest.
    set(update_flags --init --jobs 16 --depth 1) #NOTE: mostly tiny repos, cloning them one by one is network-latency bound
    if (recurse)
        list(APPEND update_flags --recursive) # git has no --no-recursive, so the flag is added, never negated
    endif ()

    find_package(Git QUIET)
    if (NOT Git_FOUND)
        message(FATAL_ERROR "Submodule(s) ${ARGN} are not initialized and git was not found. Run: git submodule update --init --recursive")
    endif ()

    message(STATUS "Submodule(s) ${ARGN} are not initialized, fetching them now...")
    execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update ${update_flags} -- ${ARGN}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE FETCH_SUBMODULES_RESULT
    )
    if (NOT FETCH_SUBMODULES_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to fetch submodule(s) ${ARGN}. Run manually: git submodule update --init --recursive")
    endif ()
endfunction()

fetch_submodules(TRUE ${SUBMODULES_RECURSIVE})
fetch_submodules(FALSE ${SUBMODULES_FLAT})
