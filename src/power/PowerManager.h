#pragma once

namespace shell::power {

class PowerManager {
public:
    static void Lock();
    static void SignOut();
    static void Restart();
    static void Shutdown();
    static void Sleep();

private:
    static bool SetShutdownPrivilege();
};

} // namespace shell::power
