#include "Config.h"
#include <fstream>
#include <shlobj.h>
#include <windows.h>

namespace shell::common {

Config Config::Load() {
    Config config;
    return config;
}

bool Config::Save(const std::filesystem::path& /*path*/) const {
    return false;
}

} // namespace shell::common
