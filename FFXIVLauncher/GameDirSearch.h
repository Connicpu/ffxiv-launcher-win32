#pragma once

#include <filesystem>
#include <optional>

std::filesystem::path GetSelfPath();
std::optional<std::filesystem::path> TryFindGameDir();
bool IsGameDir(const std::filesystem::path &path);
