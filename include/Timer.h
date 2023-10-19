#pragma
#include <chrono>

namespace LEapsGL {
    class SystemBaseMilisecondTimer {
    public:
        using time_t = long long;
        static time_t getTime() {
            auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
                );
            return currentTime.count();
        };
        static const time_t null_time = -1;
    };
}
