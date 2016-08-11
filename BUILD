# Bazel (http://bazel.io/) BUILD file for Aeron.

# You can use this library as: #include "aeron/Aeron.h"
cc_inc_library(
    name = "aeron",
	hdrs = [
        "aeron-client/src/main/cpp/Aeron.h",
	],
    prefix = "aeron-client/src/main/cpp",
    deps = [":aeron_impl"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "aeron_impl",
    srcs = glob([
        "aeron-client/src/main/cpp/*.h",
        "aeron-client/src/main/cpp/*.cpp",
        "aeron-client/src/main/cpp/**/*.h",
        "aeron-client/src/main/cpp/**/*.cpp",
    ]),
    copts = [
        "-Wall",
        "-stdlib=libc++",
        "-std=c++11",
    ],
    linkstatic = 1,
    visibility = ["//visibility:private"],
)
