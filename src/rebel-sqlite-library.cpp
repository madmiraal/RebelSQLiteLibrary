// SPDX-FileCopyrightText: 2025 Rebel SQLite Library contributors
// SPDX-FileCopyrightText: 2019-2020 TGRCDev
// SPDX-FileCopyrightText: 2017-2019 Khairul Hidayat
//
// SPDX-License-Identifier: MIT

#include "rebel-sqlite.h"

extern "C" {
void GDN_EXPORT rebel_gdnative_init(rebel_gdnative_init_options* options) {
    Rebel::Global::gdnative_init(options);
}

extern "C" void GDN_EXPORT
rebel_gdnative_terminate(rebel_gdnative_terminate_options* options) {
    Rebel::Global::gdnative_terminate(options);
}

extern "C" void GDN_EXPORT rebel_nativescript_init(void* handle) {
    Rebel::Global::nativescript_init(handle);
    Rebel::register_tool_class<Rebel::SQLite>();
}
} // extern "C"
