cmake_minimum_required(VERSION 3.25)
project(whiteoutlib-casc CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The local storage reader plus what it links against. The other games' roots and the CDN code
# are referenced by the storage core, so they are needed even though Warcraft III never uses them
add_library(whiteout_casc STATIC
	src/whiteout/common/bit_io.cpp
	src/whiteout/common/deflate.cpp
	src/whiteout/common/huffman_table.cpp
	src/whiteout/utils/job_group.cpp
	src/whiteout/sno/core_toc.cpp
	src/whiteout/sno/sno_types.cpp
	src/whiteout/storages/common/mapped_file.cpp
	src/whiteout/storages/common/jenkins.cpp
	src/whiteout/storages/common/md5.cpp
	src/whiteout/storages/common/sha1.cpp
	src/whiteout/storages/common/inflate_fast_builtin.cpp
	src/whiteout/storages/common/codecs/bzip2.cpp
	src/whiteout/storages/common/codecs/lzma.cpp
	src/whiteout/storages/casc/codec/crypto.cpp
	src/whiteout/storages/casc/codec/blte.cpp
	src/whiteout/storages/casc/tables/config.cpp
	src/whiteout/storages/casc/tables/index.cpp
	src/whiteout/storages/casc/tables/encoding.cpp
	src/whiteout/storages/casc/roots/root.cpp
	src/whiteout/storages/casc/roots/wow_root.cpp
	src/whiteout/storages/casc/roots/d3_root.cpp
	src/whiteout/storages/casc/roots/tvfs_root.cpp
	src/whiteout/storages/casc/roots/mndx_root.cpp
	src/whiteout/storages/casc/roots/d4_root.cpp
	src/whiteout/storages/casc/roots/wow_tvfs_root.cpp
	src/whiteout/storages/casc/roots/ow_root.cpp
	src/whiteout/storages/casc/roots/ow/ow_manifest_providers.cpp
	src/whiteout/storages/casc/roots/ow/ow_manifest_crypto.cpp
	src/whiteout/storages/casc/roots/ow/ow_resource_graph.cpp
	src/whiteout/storages/casc/roots/s1_root.cpp
	src/whiteout/storages/casc/roots/install_root.cpp
	src/whiteout/storages/casc/storage/progress_reporter.cpp
	src/whiteout/storages/casc/storage/storage_backend_impl.cpp
	src/whiteout/storages/casc/storage/storage_core.cpp
	src/whiteout/storages/casc/storage/storage_local.cpp
	src/whiteout/storages/casc/storage/storage_online.cpp
	src/whiteout/storages/casc/cdn/cdn_fetcher.cpp
	src/whiteout/storages/casc/cdn/online_index.cpp
	src/whiteout/storages/casc/cdn/memory_cache.cpp
	src/whiteout/storages/casc/cdn/cdn_cache.cpp
)

target_include_directories(whiteout_casc
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	PRIVATE
		src
		src/whiteout
)
target_compile_definitions(whiteout_casc
	PUBLIC WHITEOUT_HAS_CASC=1
	PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS _SILENCE_CXX20_U8PATH_DEPRECATION_WARNING
)
if(MSVC)
	target_compile_options(whiteout_casc PRIVATE /permissive- /utf-8)
endif()

install(TARGETS whiteout_casc EXPORT whiteoutlib-casc-targets ARCHIVE DESTINATION lib)
install(FILES include/whiteout/common_types.h include/whiteout/compatibility.h DESTINATION include/whiteout)
install(FILES include/whiteout/storages/casc/storage.h include/whiteout/storages/casc/types.h DESTINATION include/whiteout/storages/casc)
install(EXPORT whiteoutlib-casc-targets NAMESPACE whiteoutlib:: FILE whiteoutlib-casc-config.cmake DESTINATION share/whiteoutlib-casc)
