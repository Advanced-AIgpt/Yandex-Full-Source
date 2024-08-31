#include "backend_fs.h"

#include <alice/joker/library/log/log.h>

#include <library/cpp/openssl/crypto/sha.h>
#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/stream/file.h>
#include <util/string/hex.h>
#include <util/string/subst.h>

namespace NAlice::NJoker {

namespace {

TString MakeFileName(TString key) {
    SubstGlobal(key, "/", "+");
    NOpenSsl::NSha256::TDigest digest = NOpenSsl::NSha256::Calc(key.data());
    key += HexEncode(digest.data(), digest.size());
    return key;
}

}

TFSBackend::TFSBackend(const TConfig::TFSBackendConfigConst& backend)
    : Dir_{backend.Dir()}
{
    if (!Dir_.Exists()) {
        Dir_.MkDirs();
    }
}

TStatus TFSBackend::ObtainStub(const TStubId& id, const TString& version, TStubItemPtr& stubItem) {
    TFsPath stubPath = Dir_ / MakeFileName(id.MakeKey(&version));
    if (!stubPath.Exists()) {
        return TError{TError::EType::Logic} << "Key not found in " << Dir_.GetPath().Quote();
    }
    TFileInput stubDataStream(stubPath);
    return TStubItem::Load(id, stubDataStream, stubItem);
}

TStatus TFSBackend::SaveStub(const TString& version, TStubItemPtr stubItem) {
    const TFsPath stubPath = Dir_ / MakeFileName(stubItem->Id().MakeKey(&version));
    TFileOutput io(stubPath);
    if (TStatus error = stubItem->Serialize(io)) {
        io.Finish();
        stubPath.DeleteIfExists();
        return *error;
    }
    return Success();
}

} // namespace NAlice::NJoker
