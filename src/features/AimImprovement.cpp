#include "AimImprovement.h"
#include "../util.h"
#include <random>
#include <cmath>
#include <cstring>

namespace Features
{
    void AimImprovement::Install()
    {
        // Hook into the projectile launch function
        REL::Relocation<std::uintptr_t> hook{RELOCATION_ID(42928, 44108)};  // BGSProjectile::Launch3D
        auto& trampoline = SKSE::GetTrampoline();
        _AdjustProjectileAim = trampoline.write_call<5>(hook.address() + 0x216, AdjustProjectileAim);
        
        SKSE::log::info("Aim improvement feature installed");
    }

    void AimImprovement::AdjustProjectileAim(RE::Projectile* projectile, RE::Actor* shooter)
    {
        _AdjustProjectileAim(projectile, shooter);  // Call original function first
        
        if (!projectile || !shooter || projectile->IsMissileProjectile() || projectile->IsFlameProjectile()) {
            return;
        }

        // Check if this is Samandriel shooting
        if (!ShouldImproveAim(shooter)) {
            return;
        }

        // Get the target
        auto targetHandle = shooter->GetActorRuntimeData().currentCombatTarget;
        if (!targetHandle) {
            return;
        }

        auto target = targetHandle.get();
        if (!target || !target.get()) {
            return;
        }

        auto targetActor = target.get();
        if (!targetActor->Is3DLoaded()) {
            return;
        }

        // Calculate improved aim vector
        RE::NiPoint3 improvedAimVector = CalculateImprovedAimVector(shooter, targetActor);
        
        // Apply the improved aim to the projectile
        // Access projectile runtime data properly
        auto& projData = projectile->GetProjectileRuntimeData();
        if (projData.linearVelocity.Length() > 0.0f) {
            float speed = projData.linearVelocity.Length();
            projData.linearVelocity = improvedAimVector * speed;
        }
    }

    bool AimImprovement::ShouldImproveAim(RE::Actor* actor)
    {
        if (!actor) {
            return false;
        }

        // Check if this is Samandriel
        const char* editorID = actor->GetActorBase() ? actor->GetActorBase()->GetFormEditorID() : nullptr;
        if (!editorID) {
            return false;
        }

        return std::strcmp(editorID, "Samandriel_CSVP") == 0;
    }

    RE::NiPoint3 AimImprovement::CalculateImprovedAimVector(RE::Actor* shooter, RE::Actor* target)
    {
        // Get positions
        RE::NiPoint3 shooterPos = shooter->GetPosition();
        shooterPos.z += 120.0f;  // Adjust for bow height

        // Get target center mass (chest area)
        RE::NiPoint3 targetPos = target->GetPosition();
        targetPos.z += 80.0f;  // Adjust for chest height

        // Calculate basic aim vector
        RE::NiPoint3 aimVector = targetPos - shooterPos;
        float distance = aimVector.Length();
        
        if (distance > MAX_EFFECTIVE_RANGE) {
            // Beyond effective range, use standard aim
            aimVector.Unitize();
            return aimVector;
        }

        // Predict target movement if they're moving
        auto targetProcess = target->GetActorRuntimeData().currentProcess;
        if (targetProcess) {
            // Check if target is moving
            bool isMoving = target->IsMoving();
            
            if (isMoving && target->Is3DLoaded()) {
                // Get movement direction using available methods
                // We'll approximate movement based on the target's heading and movement state
                float targetHeading = target->GetHeading(false); // Get actor's heading in radians
                
                // Convert heading to direction vector
                RE::NiPoint3 forwardVector;
                forwardVector.x = std::sin(targetHeading);
                forwardVector.y = std::cos(targetHeading);
                forwardVector.z = 0.0f;
                
                // Estimate movement speed (using a constant approximation since GetActorValue isn't available)
                float movementSpeed = target->IsRunning() ? 300.0f : 150.0f; // Approximate speeds
                
                RE::NiPoint3 movementDir = forwardVector * movementSpeed;
                
                if (movementDir.Length() > 10.0f) {  // Target is moving significantly
                    // Calculate time to target based on arrow speed (roughly 3000 units/sec for vanilla arrows)
                    float timeToTarget = distance / 3000.0f;
                    
                    // Add predicted movement
                    targetPos += movementDir * timeToTarget * MOVING_TARGET_PREDICTION_FACTOR;
                    
                    // Recalculate aim vector
                    aimVector = targetPos - shooterPos;
                }
            }
        }

        // Apply accuracy improvement
        aimVector.Unitize();
        
        // Add slight randomization to prevent perfect aim being too obvious
        float accuracyFactor = BASE_ACCURACY_BONUS + (1.0f - BASE_ACCURACY_BONUS) * (distance / MAX_EFFECTIVE_RANGE);
        
        // Apply small random deviation
        float deviationAngle = (1.0f - accuracyFactor) * 0.05f;  // Max 5 degree deviation at worst accuracy
        
        // Create random deviation using standard random number generation
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
        
        RE::NiPoint3 randomAxis(dis(gen), dis(gen), dis(gen));
        if (randomAxis.Length() > 0.0f) {
            randomAxis.Unitize();
        }
        
        // Apply deviation using trigonometry instead of quaternions
        if (deviationAngle > 0.0f && randomAxis.Length() > 0.0f) {
            // Simple rotation approximation for small angles
            RE::NiPoint3 crossProduct = aimVector.Cross(randomAxis);
            aimVector += crossProduct * std::sin(deviationAngle);
            aimVector.Unitize();
        }
        
        return aimVector;
    }
}