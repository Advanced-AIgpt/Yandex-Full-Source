#include "stub.h"

#include <google/protobuf/messagext.h>

#include <util/datetime/base.h>
#include <util/generic/guid.h>
#include <util/stream/file.h>

namespace NAlice::NJoker {

// static
TStatus TStubItem::Load(const TStubId& id, IInputStream& in, TStubItemPtr& item) {
    try {
        TProto stubProto;
        stubProto.Load(&in);
        item = MakePtr(id, std::move(stubProto));
    } catch (...) {
        return TError{TError::EType::Logic} << "Unable to load stub item: " << CurrentExceptionMessage();
    }

    return Success();
}

/*
// static
TStatus TStubItem::Load(const TBlob& in, TStubItemPtr& item) {
    try {
        TProto stubProto;
        TMemoryInput input(in.Data(), in.Size());
        stubProto.Load(&input);
        item = MakePtr(std::move(stubProto));
    } catch (...) {
        return TError{TError::EType::Logic} << "Unable to load stub item: " << CurrentExceptionMessage();
    }

    return Success();
}
*/

TStubItem::TStubItem(TStubId stubId, TProto&& proto)
    : Id_{stubId}
    , Proto_{std::move(proto)}
{
}

TStatus TStubItem::Serialize(IOutputStream& out) const {
    try {
        Proto_.Save(&out);
    } catch (...) {
        return TError{TError::EType::Logic} << "Unable to save response result: " << CurrentExceptionMessage();
    }

    return Success();
}

TString TStubItem::NewVersion() const {
    return CreateGuidAsString();
}

} // namespace NAlice::NJoker
