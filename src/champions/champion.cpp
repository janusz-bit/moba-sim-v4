#include "champions/champion.hpp"

namespace moba_sim {

double Champion::base_value(StatId stat, int level) const {
    const int growth_steps = level > 1 ? level - 1 : 0;

    switch (stat) {
    case StatId::Health:
        return health + health_growth * growth_steps;
    case StatId::HealthRegen:
        return health_regen + health_regen_growth * growth_steps;
    case StatId::Resource:
        return resource + resource_growth * growth_steps;
    case StatId::ResourceRegen:
        return resource_regen + resource_regen_growth * growth_steps;
    case StatId::AttackDamage:
        return attack_damage + attack_damage_growth * growth_steps;
    case StatId::AttackSpeed:
        return attack_speed;
    case StatId::Armor:
        return armor + armor_growth * growth_steps;
    case StatId::MagicResist:
        return magic_resist + magic_resist_growth * growth_steps;
    case StatId::MovementSpeed:
        return movement_speed;
    case StatId::AttackRange:
        return attack_range;
    }
    return 0.0;
}

StatPipeline Champion::pipeline_for(StatId stat, int level) const {
    StatPipeline pipeline;
    pipeline.add({ModifierKind::Base, base_value(stat, level)});
    return pipeline;
}

} // namespace moba_sim