#pragma once

namespace Features
{
    class AimImprovement
    {
    public:
        static void Install();

    private:
        static void AdjustProjectileAim(RE::Projectile* projectile, RE::Actor* shooter);
        static bool ShouldImproveAim(RE::Actor* actor);
        static RE::NiPoint3 CalculateImprovedAimVector(RE::Actor* shooter, RE::Actor* target);
        
        // Configuration values
        static constexpr float BASE_ACCURACY_BONUS = 0.7f;  // 70% accuracy improvement
        static constexpr float MAX_EFFECTIVE_RANGE = 4096.0f; // Maximum range for accuracy bonus
        static constexpr float MOVING_TARGET_PREDICTION_FACTOR = 0.8f; // How much to lead moving targets

        static inline REL::Relocation<decltype(AdjustProjectileAim)> _AdjustProjectileAim;
    };
}