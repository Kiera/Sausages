# -*- cmake -*-
include(Prebuilt)
use_prebuilt_binary(Lua)
#
set(LUA_LIBRARY lua5.1)
set(LUA_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/lua)