#pragma once

#include <alice/joker/library/status/status.h>
#include <alice/joker/library/stub/stub.h>
#include <alice/joker/library/stub/stub_id.h>

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>

namespace NAlice::NJoker {

class TBackend : private NNonCopyable::TNonCopyable {
public:
    virtual ~TBackend() = default;

    /** Get stub data from backend 'storage'.
     */
    virtual TStatus ObtainStub(const TStubId& id, const TString& version, TStubItemPtr& item) = 0;

    /** Save stub in the backend 'storage'.
     */
    virtual TStatus SaveStub(const TString& version, TStubItemPtr item) = 0;
};

} // namespace NAlice::NJoker
