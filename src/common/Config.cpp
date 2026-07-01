#include "Config.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>

namespace shell::common {

Config Config::Load() {
    Config config;
    return config;
}

bool Config::Save(const std::filesystem::path& path) const {
    return false;
}

} // namespace shell::common
