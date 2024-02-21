# Copyright lowRISC contributors.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

load("//rules:repo.bzl", "http_archive_or_local")

def cpp_repos(
        rtmidi = None):
    http_archive_or_local(
        name = "com_github_rtmidi",
        local = rtmidi,
        sha256 = "ef7bcda27fee6936b651c29ebe9544c74959d0b1583b716ce80a1c6fea7617f0",
        strip_prefix = "rtmidi-6.0.0",
        url = "https://github.com/thestk/rtmidi/archive/refs/tags/6.0.0.tar.gz",
        build_file = Label("//third_party/cpp:BUILD.rtmidi.bazel"),
    )
