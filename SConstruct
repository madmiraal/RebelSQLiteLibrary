#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2025 Rebel SQLite Library contributors
# SPDX-FileCopyrightText: 2019-2020 TGRCDev
# SPDX-FileCopyrightText: 2017-2019 Khairul Hidayat
#
# SPDX-License-Identifier: MIT

from RebelSDK.tools.build import *

environment = get_environment(ARGUMENTS)
sqlite_environment = environment.Clone()

# Build Rebel SDK
Export("environment")
SConscript("RebelSDK/SConstruct")

# Don't encrypt the header of SQLite databases
sqlite_environment.Append(CPPDEFINES=[("SKIP_HEADER_BYTES", 32)])

# Include directories
sqlite_environment.Append(
    CPPPATH=[
        ".",
        "RebelSDK/include",
        "RebelSDK/include/api",
        "third-party",
    ]
)

# Source files
sources = [
    "src/rebel-sqlite.cpp",
    "src/rebel-sqlite-library.cpp",
    "third-party/sqleet/sqleet.c",
    "third-party/spmemvfs/spmemvfs.c",
]

# Add Rebel SDK Library
sqlite_environment.Append(LIBPATH=["RebelSDK/builds/"])
sqlite_environment.Append(LIBS=[get_rebel_sdk_library_name(environment)])

# Rebel SQLite Library name
rebel_sqlite_library = "-".join(["rebel-sqlite", get_suffix(environment)])

# Build library
sqlite_environment.SharedLibrary(
    target="builds/" + rebel_sqlite_library, source=sources
)
