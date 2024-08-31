#pragma once

#include <util/generic/fwd.h>
#include <util/generic/map.h>
#include <util/generic/yexception.h>

#include <typeindex>

namespace NBASS {
class IDirective {
};

class TDirectiveFactory {
private:
    using TMapByClasses = TMap<std::type_index, TString>;

public:
    using TConstDirectiveIterator = typename TMapByClasses::const_iterator;
    using TDirectiveIndex = std::type_index;

    static TDirectiveFactory *Get() {
        return Singleton<TDirectiveFactory>();
    }

    template<class TDirective>
    void RegisterDirective(const TStringBuf analyticsTag) {
        RegisterDirectiveImpl<TDirective>(analyticsTag);
    }

    template<class TDirective>
    TString GetAnalyticsTag() {
        const auto typeIndex = std::type_index(typeid(TDirective));
        CheckDirectiveRegistered(typeIndex);
        return DirectiveAnalyticTags[typeIndex];
    }

    TString GetAnalyticsTag(const TDirectiveIndex& typeIndexDirective) {
        CheckDirectiveRegistered(typeIndexDirective);
        return DirectiveAnalyticTags[typeIndexDirective];
    }

    TConstDirectiveIterator begin() const {
        return DirectiveAnalyticTags.begin();
    }

    TConstDirectiveIterator end() const {
        return DirectiveAnalyticTags.end();
    }

private:
    TMapByClasses DirectiveAnalyticTags;

    template<typename TDirective>
    void RegisterDirectiveImpl(const TStringBuf analytics_tag) {
        const auto typeIndex = std::type_index(typeid(TDirective));
        CheckNotRegistered(typeIndex);
        DirectiveAnalyticTags[typeIndex] = analytics_tag;
    }

    void CheckNotRegistered(const TDirectiveIndex& typeIndex) {
        Y_ENSURE(!DirectiveAnalyticTags.contains(typeIndex),
                 "type_info '" << typeIndex.name() << "'"
                 "is already registered under name '" << DirectiveAnalyticTags[typeIndex] << "'");
    }

    void CheckDirectiveRegistered(const std::type_index& typeIndex) {
        Y_ENSURE(DirectiveAnalyticTags.contains(typeIndex),
                 "type_info '" << typeIndex.name() << "' is not registered, use REGISTER_DIRECTIVE macros");
    }
};

template<class TDirective>
struct TDirectiveRegistrator {
    TDirectiveRegistrator(const TStringBuf analyticsTag) {
        TDirectiveFactory::Get()->RegisterDirective<TDirective>(analyticsTag);
    }
};

template<class TDirective>
inline TDirectiveFactory::TDirectiveIndex GetAnalyticsTagIndex() {
    return TDirectiveFactory::TDirectiveIndex(typeid(TDirective));
}

} // NBASS

#define REGISTER_DIRECTIVE(className, analyticsTag) \
static NBASS::TDirectiveRegistrator<className> \
Y_GENERATE_UNIQUE_ID(TDirectiveRegistrator)((analyticsTag));
