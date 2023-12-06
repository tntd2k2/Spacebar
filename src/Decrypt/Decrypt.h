#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <stdint.h>

class LeagueDecrypt {
public:
    static void Decrypt();
private:
    static void TriggerVEH(uint64_t address);
};