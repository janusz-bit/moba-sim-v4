#include "stats/modifier.hpp"

#include "stats/stat_pipeline.hpp"

namespace moba_sim {

void apply_modifier(StatPipeline& pipe, const Modifier& mod) {
    apply_modifier(pipe, mod, mod.source);
}

void apply_modifier(StatPipeline& pipe, const Modifier& mod, const std::string& source) {
    switch (mod.kind) {
    case ModifierKind::Base:
        pipe.add_base(mod.value, source);
        break;
    case ModifierKind::Inc:
        pipe.add_inc(mod.value, source);
        break;
    case ModifierKind::More:
        pipe.add_more(mod.value, source);
        break;
    }
}

} // namespace moba_sim
