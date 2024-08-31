#include "bno_apps.h"

#include <library/cpp/resource/resource.h>

namespace NAlice {

TBnoApps::TBnoApps(const TFsPath& dirPath)
    : Trie(TBlob::FromFileContent(dirPath / "bno.trie"))
{
}

TMaybe<TString> TBnoApps::Find(TStringBuf docId, const TClientInfo& clientInfo) const {
    TBnoApp result;
    if (Trie.Find(docId, &result)) {
        if (clientInfo.IsAndroid()) {
            return result.AndroidAppId ? MakeMaybe(result.AndroidAppId) : Nothing();
        }
        if (clientInfo.IsIOS()) {
            // todo: ipad support
            return result.IPhoneAppId ? MakeMaybe(result.IPhoneAppId) : Nothing();
        }
    }
    return Nothing();
}

size_t TBnoApps::Size() const {
    return Trie.Size();
}

} // namespace NAlice
