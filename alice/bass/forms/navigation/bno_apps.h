#pragma once

#include <alice/library/app_navigation/bno_apps.h>

#include <library/cpp/threading/hot_swap/hot_swap.h>
#include <util/generic/maybe.h>


namespace NBASS {

class TContext;

/**
 * Хранилище мобильных приложений для БНО (организаций, сайтов, ...)
 */
class TBnoApps {
public:
    ~TBnoApps();

    static TBnoApps* Instance() {
        return Singleton<TBnoApps>();
    }

    /**
     * @param docId   id документа
     * @param context текущий контекст
     * @return идентификатор приложения или Nothing()
     */
    TMaybe<TString> Find(TStringBuf docId, const TContext& context) const;

    /**
     * Перезагружает трай data/bno.trie
     * @return true в случае успеха
     */
    bool Reload();
private:
    Y_DECLARE_SINGLETON_FRIEND();

    TBnoApps();

    THotSwap<NAlice::TBnoApps> Impl;
};

}
