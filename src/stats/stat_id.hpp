#pragma once

namespace moba_sim {

// Identifies a champion stat that flows through the Base/Inc/More pipeline.
enum class StatId {
    Health,
    HealthRegen,
    Resource,
    ResourceRegen,
    AttackDamage,
    AttackSpeed,
    Armor,
    MagicResist,
    MovementSpeed,
    AttackRange,
};

} // namespace moba_sim