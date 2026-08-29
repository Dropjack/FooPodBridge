set(FOOPODBRIDGE_THIRD_PARTY_DIR "${CMAKE_CURRENT_LIST_DIR}/../third_party")
cmake_path(NORMAL_PATH FOOPODBRIDGE_THIRD_PARTY_DIR)

set(FOOPODBRIDGE_PFC_DIR "${FOOPODBRIDGE_THIRD_PARTY_DIR}/pfc")
set(FOOPODBRIDGE_FOOBAR2000_DIR "${FOOPODBRIDGE_THIRD_PARTY_DIR}/foobar2000")

foreach(required_path IN ITEMS
  "${FOOPODBRIDGE_THIRD_PARTY_DIR}/sdk-license.txt"
  "${FOOPODBRIDGE_PFC_DIR}/pfc.h"
  "${FOOPODBRIDGE_PFC_DIR}/pfc-license.txt"
  "${FOOPODBRIDGE_FOOBAR2000_DIR}/SDK/foobar2000.h"
  "${FOOPODBRIDGE_FOOBAR2000_DIR}/foobar2000_component_client/component_client.cpp"
)
  if(NOT EXISTS "${required_path}")
    message(FATAL_ERROR "Required official SDK file is missing: ${required_path}")
  endif()
endforeach()

file(SHA256 "${FOOPODBRIDGE_THIRD_PARTY_DIR}/sdk-license.txt" FOOPODBRIDGE_SDK_LICENSE_SHA256)
if(NOT FOOPODBRIDGE_SDK_LICENSE_SHA256 STREQUAL "2aa8af2f2a0cce2dce4c2a4f422bcbd1752dab7d1e1b973981b77c971b0b8a32")
  message(FATAL_ERROR "foobar2000 SDK license hash does not match the audited 2025-03-07 package.")
endif()

set(FOOPODBRIDGE_PFC_SOURCES
  audio_math.cpp
  audio_sample.cpp
  base64.cpp
  bigmem.cpp
  bit_array.cpp
  bsearch.cpp
  charDownConvert.cpp
  cpuid.cpp
  crashWithMessage.cpp
  filehandle.cpp
  filetimetools.cpp
  guid.cpp
  nix-objects.cpp
  other.cpp
  pathUtils.cpp
  pfc-fb2k-hooks.cpp
  printf.cpp
  selftest.cpp
  SmartStrStr.cpp
  sort.cpp
  splitString2.cpp
  stdafx.cpp
  string-compare.cpp
  string-conv-lite.cpp
  string-lite.cpp
  string_base.cpp
  string_conv.cpp
  threads.cpp
  timers.cpp
  unicode-normalize.cpp
  utf8.cpp
  wildcard.cpp
  win-objects.cpp
)
list(TRANSFORM FOOPODBRIDGE_PFC_SOURCES PREPEND "${FOOPODBRIDGE_PFC_DIR}/")

add_library(foopodbridge_pfc STATIC ${FOOPODBRIDGE_PFC_SOURCES})
target_include_directories(foopodbridge_pfc SYSTEM PUBLIC "${FOOPODBRIDGE_THIRD_PARTY_DIR}")
target_compile_definitions(foopodbridge_pfc PUBLIC _UNICODE UNICODE NOMINMAX)
target_compile_options(foopodbridge_pfc PRIVATE /W3 /permissive- /Zc:__cplusplus /EHsc /wd4996)

set(FOOPODBRIDGE_FOOBAR2000_SDK_SOURCES
  abort_callback.cpp
  advconfig.cpp
  album_art.cpp
  app_close_blocker.cpp
  audio_chunk.cpp
  audio_chunk_channel_config.cpp
  cfg_var.cpp
  cfg_var_legacy.cpp
  chapterizer.cpp
  commandline.cpp
  commonObjects.cpp
  completion_notify.cpp
  componentversion.cpp
  configStore.cpp
  config_io_callback.cpp
  config_object.cpp
  console.cpp
  dsp.cpp
  dsp_manager.cpp
  file_cached_impl.cpp
  file_info.cpp
  file_info_const_impl.cpp
  file_info_impl.cpp
  file_info_merge.cpp
  file_operation_callback.cpp
  filesystem.cpp
  filesystem_helper.cpp
  foosort.cpp
  fsItem.cpp
  guids.cpp
  hasher_md5.cpp
  image.cpp
  input.cpp
  input_file_type.cpp
  link_resolver.cpp
  mainmenu.cpp
  main_thread_callback.cpp
  mem_block_container.cpp
  menu_helpers.cpp
  menu_item.cpp
  menu_manager.cpp
  metadb.cpp
  metadb_handle.cpp
  metadb_handle_list.cpp
  output.cpp
  packet_decoder.cpp
  playable_location.cpp
  playback_control.cpp
  playlist.cpp
  playlist_loader.cpp
  popup_message.cpp
  preferences_page.cpp
  replaygain.cpp
  replaygain_info.cpp
  service.cpp
  stdafx.cpp
  tag_processor.cpp
  tag_processor_id3v2.cpp
  threaded_process.cpp
  titleformat.cpp
  track_property.cpp
  ui.cpp
  ui_element.cpp
  utility.cpp
)
list(TRANSFORM FOOPODBRIDGE_FOOBAR2000_SDK_SOURCES PREPEND "${FOOPODBRIDGE_FOOBAR2000_DIR}/SDK/")

add_library(foopodbridge_foobar2000_sdk STATIC ${FOOPODBRIDGE_FOOBAR2000_SDK_SOURCES})
target_include_directories(foopodbridge_foobar2000_sdk SYSTEM PUBLIC
  "${FOOPODBRIDGE_THIRD_PARTY_DIR}"
  "${FOOPODBRIDGE_FOOBAR2000_DIR}"
)
target_compile_definitions(foopodbridge_foobar2000_sdk PUBLIC _UNICODE UNICODE NOMINMAX)
target_compile_options(foopodbridge_foobar2000_sdk PRIVATE /W3 /permissive- /Zc:__cplusplus /EHsc /wd4996)
target_link_libraries(foopodbridge_foobar2000_sdk PUBLIC foopodbridge_pfc)

add_library(foopodbridge_component_client STATIC
  "${FOOPODBRIDGE_FOOBAR2000_DIR}/foobar2000_component_client/component_client.cpp"
)
target_include_directories(foopodbridge_component_client SYSTEM PUBLIC
  "${FOOPODBRIDGE_THIRD_PARTY_DIR}"
  "${FOOPODBRIDGE_FOOBAR2000_DIR}"
)
target_compile_definitions(foopodbridge_component_client PUBLIC _UNICODE UNICODE NOMINMAX)
target_compile_options(foopodbridge_component_client PRIVATE /W3 /permissive- /Zc:__cplusplus /EHsc /wd4996)
target_link_libraries(foopodbridge_component_client PUBLIC foopodbridge_foobar2000_sdk foopodbridge_pfc)

set(FOOPODBRIDGE_SHARED_SOURCES
  audio_math.cpp
  crash_info.cpp
  font_description.cpp
  minidump.cpp
  modal_dialog.cpp
  stdafx.cpp
  systray.cpp
  text_drawing.cpp
  utf8.cpp
  utf8api.cpp
  Utility.cpp
)
list(TRANSFORM FOOPODBRIDGE_SHARED_SOURCES PREPEND "${FOOPODBRIDGE_FOOBAR2000_DIR}/shared/")

add_library(foopodbridge_shared SHARED ${FOOPODBRIDGE_SHARED_SOURCES})
set_target_properties(foopodbridge_shared PROPERTIES OUTPUT_NAME "shared")
target_include_directories(foopodbridge_shared SYSTEM PUBLIC
  "${FOOPODBRIDGE_THIRD_PARTY_DIR}"
  "${FOOPODBRIDGE_FOOBAR2000_DIR}"
)
target_compile_definitions(foopodbridge_shared PRIVATE
  _CRT_SECURE_NO_WARNINGS
  SHARED_EXPORTS
  _WINDLL
  _UNICODE
  UNICODE
  NOMINMAX
)
target_compile_options(foopodbridge_shared PRIVATE /W3 /permissive- /Zc:__cplusplus /EHsc /wd4996)
target_link_libraries(foopodbridge_shared PRIVATE
  foopodbridge_pfc
  comctl32
  imagehlp
  uxtheme
  dbghelp
)
