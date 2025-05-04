#include "hook.h"
#include "features/AimImprovement.h"

namespace Hooks
{
    void Install()
    {
        SKSE::AllocTrampoline(14 * 8);  // Allocate space for hooks
        
        Features::AimImprovement::Install();
        
        SKSE::log::info("All hooks installed successfully");
    }
}