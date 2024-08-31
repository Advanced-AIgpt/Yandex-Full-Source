#include "goodwin.h"

#include <alice/hollywood/library/registry/registry.h>

#include <alice/library/scenarios/data_sources/data_sources.h>


namespace NAlice::NHollywood {

#define PASS(...) __VA_ARGS__

#define ACCEPT_BY_FILTERS(filters_)                                                             \
    [](const NFrameFiller::TSearchDocMeta& meta) {                                    \
        const std::vector<NFrameFiller::TSearchDocMeta> filters(filters_);            \
        for (const auto& filter : filters) {                                                    \
            if ((std::tie(meta.Type, meta.Subtype) == std::tie(filter.Type, filter.Subtype)) || \
                (filter.Subtype.empty() && meta.Type == filter.Type)                            \
            ) {                                                                                 \
                return true;                                                                    \
            }                                                                                   \
        }                                                                                       \
        return false;                                                                           \
    }

#define ACCEPT_COVID ACCEPT_BY_FILTERS(PASS({{"covid", ""}}))
#define ACCEPT_ENTITY_SEARCH ACCEPT_BY_FILTERS(PASS({{"entity_search", ""}}))

#define ACCEPT_REST                                                             \
    [](const NFrameFiller::TSearchDocMeta& meta) {                              \
        return                                                                  \
            !ACCEPT_ENTITY_SEARCH(meta) &&                                      \
            !ACCEPT_COVID(meta);                                                \
    }                                                                           \

REGISTER_GOODWIN_SCENARIO("covid19", ACCEPT_COVID);
REGISTER_GOODWIN_SCENARIO("wizard", ACCEPT_REST);

}  // namespace NAlice::NHollywood
