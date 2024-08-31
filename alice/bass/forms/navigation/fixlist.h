#pragma once

#include <alice/bass/forms/context/context.h>

#include <alice/library/app_navigation/fixlist.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/threading/hot_swap/hot_swap.h>

#include <util/generic/singleton.h>


namespace NBASS {

/**
 * fixlist для навигационников.
 */
class TNavigationFixList {
public:
    ~TNavigationFixList();

    static TNavigationFixList* Instance() {
        return Singleton<TNavigationFixList>();
    }

    /// Находит и возвращает соответствующий запросу блок из фикслиста.
    NSc::TValue Find(const TString& query, const TContext& context) const;

    TMaybe<NAlice::TNavigationFixList::TWindowsApp> FindWindowsApp(const TString& query) const;

    TMaybe<NAlice::TNavigationFixList::TNativeApp> FindNativeApp(const TString& code) const {
        return Fixlist.FindNativeApp(code);
    }

    /// Reload Yandex.Stroka fixlist
    void ReloadYaStrokaFixList();
private:
    Y_DECLARE_SINGLETON_FRIEND();

    TNavigationFixList();

    NAlice::TNavigationFixList Fixlist;
};

}  // namespace NBASS
