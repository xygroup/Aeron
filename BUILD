# Bazel (http://bazel.io/) BUILD file for Aeron.

# Media Driver

cc_binary(
    name = "driver",
    srcs = glob([
        "aeron-driver/src/main/cpp/*.h",
        "aeron-driver/src/main/cpp/*.cpp",
        "aeron-driver/src/main/cpp/**/*.h",
        "aeron-driver/src/main/cpp/**/*.cpp",
    ]),
    copts = [
        "-Wall",
        "-Wsign-compare",
        "-fexceptions",
        "-m64",
        "-stdlib=libc++",
        "-std=c++11",
    ],
    deps = [
        ":aeron",
    ],
)

# You can use this library as: #include "aeron/Aeron.h"
cc_inc_library(
    name = "aeron",
	hdrs = glob([
        "aeron-client/src/main/cpp/*.h",
        "aeron-client/src/main/cpp/**/*.h",
    ]),
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
        "-Wsign-compare",
        "-fexceptions",
        "-m64",
        "-stdlib=libc++",
        "-std=c++11",
    ],
    linkstatic = 1,
    visibility = ["//visibility:private"],
)

# Test apps

cc_binary(
    name = "ping",
    srcs = glob([
        "aeron-samples/src/main/cpp/Configuration.h",
        "aeron-samples/src/main/cpp/Ping.cpp",
    ]),
    copts = [
        "-Wall",
        "-Wsign-compare",
        "-fexceptions",
        "-m64",
        "-stdlib=libc++",
        "-std=c++11",
    ],
    deps = [
        ":aeron",
        "//hdr_histogram",
    ],
)

cc_binary(
    name = "pong",
    srcs = glob([
        "aeron-samples/src/main/cpp/Configuration.h",
        "aeron-samples/src/main/cpp/Pong.cpp",
    ]),
    copts = [
        "-Wall",
        "-Wsign-compare",
        "-fexceptions",
        "-m64",
        "-stdlib=libc++",
        "-std=c++11",
    ],
    deps = [
        ":aeron",
    ],
)
