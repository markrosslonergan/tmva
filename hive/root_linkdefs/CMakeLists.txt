include(${ROOT_USE_FILE})
include_directories(${CMAKE_SOURCE_DIR}/inc)
ROOT_GENERATE_DICTIONARY(G__DeNan denan.h LINKDEF denan_LinkDef.h)
add_library(DeNan SHARED G__DeNan.cxx)

