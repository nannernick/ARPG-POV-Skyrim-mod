#pragma once

#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <numbers>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <REX/W32/D3D11.h>
#include <fmt/format.h>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <list>
#include <span>
#include <unordered_set>

namespace logs = SKSE::log;
namespace logger = SKSE::log;  // the name the outline renderer (src/render) uses

// Most things the outline renderer keeps ready to draw at once (its cache size).
static constexpr uint32_t Max_Corpse_Count = 96;

#include "plugin.h"

// CommonLib only enables "..."sv literals inside its own namespaces, but the plugin-info
// file it generates uses them at global scope.
using namespace std::literals;
