#pragma once

#include "request_parts.h"

#include <alice/megamind/library/request_composite/composite.h>
#include <alice/megamind/library/request_composite/event.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/io/headers.h>
#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <functional>

namespace NAlice::NMegamind {

class TSpeechKitInitContext {
public:
    using TProtoPtr = TSimpleSharedPtr<TSpeechKitRequestProto>;

public:
    TSpeechKitInitContext(const TCgiParameters& cgi, const THttpHeaders& headers, const TString& path, const TString& rngSalt)
        : Proto{MakeSimpleShared<TSpeechKitRequestProto>()}
        , EventProtoPtr{MakeSimpleShared<TEventComponent::TEventProto>()}
        , Cgi{cgi}
        , Headers{headers}
        , Path{path}
        , RngSalt{rngSalt}
    {
    }

    TProtoPtr Proto;
    TEventComponent::TEventProtoPtr EventProtoPtr;
    const TCgiParameters& Cgi;
    const THttpHeaders& Headers;
    const TString Path;
    const TString RngSalt;
};

template <typename... TComponents>
using TSpeechKitRequestComposite = TRequestComposite<TComponents...>;

} // namespace NAlice::NMegamind
