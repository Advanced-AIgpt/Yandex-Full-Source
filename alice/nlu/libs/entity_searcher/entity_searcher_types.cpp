#include "entity_searcher_types.h"

namespace NAlice::NNlu {

IOutputStream& operator<<(IOutputStream& out, const TEntityString& entity) {
    return out << "{\"" << entity.Sample << "\", \"" << entity.Type << "\", \"" << entity.Value << "\", " << entity.LogProbability << "}";
}

} // namespace NAlice::NNlu
