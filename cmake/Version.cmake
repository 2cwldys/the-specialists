# reTS version. In order:
#   1. $RETS_VERSION (CI sets it to the pushed tag, e.g. v0.0.1 or v1.2.0-rc1; any text is accepted);
#   2. git describe against the latest v* tag (v0.0.1 on the tag, v0.0.1-3-gabc1234 after it);
#   3. none: the short commit alone.
# Sets RETS_VERSION_FULL (that string, or the commit), RETS_VERSION_HASH (short commit, or "unknown"),
# and RETS_VERSION_MAJOR/MINOR/PATCH/BUILD for the Windows version resource's numeric fields: the
# leading X.Y.Z of the version (0.0.0 when it has none) and the commits since the tag (0 on the tag).

set(_ver "$ENV{RETS_VERSION}")
set(_hash "unknown")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
	execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE _hash ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(_hash STREQUAL "")
		set(_hash "unknown")
	endif()
	if(_ver STREQUAL "")
		execute_process(COMMAND "${GIT_EXECUTABLE}" describe --tags --match "v*"
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE _ver ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
	endif()
endif()

set(RETS_VERSION_HASH "${_hash}")
set(RETS_VERSION_MAJOR 0)
set(RETS_VERSION_MINOR 0)
set(RETS_VERSION_PATCH 0)
set(RETS_VERSION_BUILD 0)
if(_ver STREQUAL "")
	set(RETS_VERSION_FULL "${_hash}")
else()
	set(RETS_VERSION_FULL "${_ver}")
	if(_ver MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)")
		set(RETS_VERSION_MAJOR ${CMAKE_MATCH_1})
		set(RETS_VERSION_MINOR ${CMAKE_MATCH_2})
		set(RETS_VERSION_PATCH ${CMAKE_MATCH_3})
	endif()
	if(_ver MATCHES "-([0-9]+)-g[0-9a-f]+$")
		set(RETS_VERSION_BUILD ${CMAKE_MATCH_1})
	endif()
endif()
message(STATUS "reTS version ${RETS_VERSION_FULL} (${RETS_VERSION_HASH})")

# Windows: a version resource on each library. <target> <OriginalFilename> <FileDescription>
function(rets_version_resource target filename description)
	if(NOT WIN32)
		return()
	endif()
	set(RETS_RC_FILENAME "${filename}")
	set(RETS_RC_DESCRIPTION "${description}")
	set(_rc "${CMAKE_CURRENT_BINARY_DIR}/${target}_version.rc")
	configure_file("${CMAKE_SOURCE_DIR}/cmake/version.rc.in" "${_rc}" @ONLY)
	target_sources(${target} PRIVATE "${_rc}")
endfunction()
