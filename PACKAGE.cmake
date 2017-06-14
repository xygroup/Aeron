set (AERON_INC ${INC_ROOT}/aeron/_/aeron)
set (AERON_LIB ${BAZEL_BIN}/aeron/libaeron_impl.a)

include_directories(${AERON_INC})
