#pragma once

#include "bno_apps_trie.h"

#include <alice/library/client/client_info.h>

#include <util/folder/path.h>
#include <util/generic/maybe.h>

namespace NAlice {

/**
 * Хранилище мобильных приложений для БНО (организаций, сайтов, ...)
 */
class TBnoApps : public TAtomicRefCount<TBnoApps> {
public:
    TBnoApps(const TFsPath& dirPath);

    /**
     * @param docId   id документа
     * @param context текущий контекст
     * @return идентификатор приложения или Nothing()
     */
    TMaybe<TString> Find(TStringBuf docId, const TClientInfo& clientInfo) const;

    size_t Size() const;
private:
    const TBnoAppsTrie Trie;
};

} // namespace NAlice
