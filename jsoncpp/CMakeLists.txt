set (IOGLTF_MAJOR_VERSION 1)
set (IOGLTF_MINOR_VERSION 0)
set (IOGLTF_PATCH_VERSION 0)
set (IOGLTF_VERSION ${IOGLTF_MAJOR_VERSION}.${IOGLTF_MINOR_VERSION}.${IOGLTF_PATCH_VERSION})

# jsoncpp dynamic libray
file (GLOB_RECURSE jsoncpp-src ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
include_directories (
	../
	/usr/local/include
)
link_directories (
	/usr/local/lib
)
add_library (jsoncpp SHARED ${jsoncpp-src})
target_link_libraries (
	jsoncpp
)