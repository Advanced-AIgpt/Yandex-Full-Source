#pragma once

#include "stub_id.h"

#include <alice/joker/library/proto/protos.pb.h>
#include <alice/joker/library/status/status.h>

#include <util/folder/path.h>
#include <util/generic/fwd.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>

namespace NAlice::NJoker {

class TStubItem;
using TStubItemPtr = TIntrusivePtr<TStubItem>;

/** The class represents a stub.
 * In other words its a wrapper over `NJokerProto::TStubProto`.
 */
class TStubItem : public TThrRefBase {
public:
    using TProto = NJokerProto::TStubProto;

public:
    static TStatus Load(const TStubId& id, IInputStream& input, TStubItemPtr& item);
    /*
    static TStatus Load(const TBlob& data, TStubItemPtr& item);
    */

    template <typename... TArgs>
    static TStubItemPtr MakePtr(TArgs... args) {
        return MakeIntrusive<TStubItem>(std::forward<TArgs>(args)...);
    }

    const TProto& Get() const {
        return Proto_;
    }

    const TStubId& Id() const {
        return Id_;
    }

public:
    TStubItem(TStubId stubId, TProto&& proto);

    TStatus Serialize(IOutputStream& out) const;

    TString NewVersion() const;

private:
    const TStubId Id_;
    const TProto Proto_;
};

} // namespace NAlice::NJoker
