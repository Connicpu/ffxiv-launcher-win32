#pragma once

#include <filesystem>
#include <optional>

std::optional<std::filesystem::path> TryFindGameDir();
bool IsGameDir(const std::filesystem::path &path);
