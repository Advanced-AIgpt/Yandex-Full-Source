#include "backend_s3.h"

#include <alice/joker/library/log/log.h>

#include <library/cpp/openssl/crypto/sha.h>

#include <util/generic/buffer.h>
#include <util/generic/strbuf.h>

namespace NAlice::NJoker {
namespace {

TMaybe<IS3Storage::TCredentials> ConstructCredentials(const TConfig::TS3BackendConfigConst& backend) {
    if (!backend.HasCredentials()) {
        return Nothing();
    }

    return IS3Storage::TCredentials{TString{*backend.Credentials().KeyId()}, TString{*backend.Credentials().KeySecret()}};
}

} // namespace

TS3Backend::TS3Backend(const TConfig::TS3BackendConfigConst& backend)
    : Storage_{IS3Storage::Create(backend.Host(), backend.Bucket(), backend.Timeout(), ConstructCredentials(backend))}
{
}

TStatus TS3Backend::ObtainStub(const TStubId& id, const TString& version, TStubItemPtr& stubItem) {
    TStringStream stubDataStream;
    if (TStatus error = Storage_->Get(id.MakeKey(&version), stubDataStream)) {
        return error;
    }

    return TStubItem::Load(id, stubDataStream, stubItem);
}

TStatus TS3Backend::SaveStub(const TString& version, TStubItemPtr stubItem) {
    TStringStream io;
    if (TStatus error = stubItem->Serialize(io)) {
        return *error;
    }

    return Storage_->Put(stubItem->Id().MakeKey(&version), TBuffer(io.Str().data(), io.Str().length()));
}

} // namespace NAlice::NJoker
