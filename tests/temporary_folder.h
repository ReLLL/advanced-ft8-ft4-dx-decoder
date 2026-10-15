// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#include <stdexcept>
// Declare before test engines/stores, so writers close before their files disappear.
struct TestFolderCleanup {
 std::filesystem::path path;
 explicit TestFolderCleanup(std::filesystem::path value):path(std::move(value)){if(path.filename().empty()||!std::filesystem::equivalent(path.parent_path(),std::filesystem::temp_directory_path()))throw std::runtime_error("Test cleanup requires an owned temporary directory");}
 ~TestFolderCleanup(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
