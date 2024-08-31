#include "bno_apps.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/resource/resource.h>

namespace NBASS {

TBnoApps::TBnoApps() {
    Reload();
}

TBnoApps::~TBnoApps() {
}

TMaybe<TString> TBnoApps::Find(TStringBuf docId, const TContext& context) const {
    if (auto ptr = Impl.AtomicLoad()) {
        return ptr->Find(docId, context.MetaClientInfo());
    }
    return Nothing();
}

bool TBnoApps::Reload() {
    try {
        std::unique_ptr<NAlice::TBnoApps> trie = std::make_unique<NAlice::TBnoApps>("data");
        LOG(INFO) << "bno.trie updated: " << trie->Size() << " documents" << Endl;
        Impl.AtomicStore(trie.release());
        return true;
    } catch (...) {
        LOG(ERR) << "Unable to load bno.trie: " << CurrentExceptionMessage();
    }
    return false;
}

}
