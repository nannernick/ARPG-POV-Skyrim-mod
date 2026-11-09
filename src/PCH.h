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
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace logs = SKSE::log;

// CommonLib only enables "..."sv literals inside its own namespaces, but the plugin-info
// file it generates uses them at global scope.
using namespace std::literals;
