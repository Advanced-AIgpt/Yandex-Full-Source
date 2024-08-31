#pragma once

#include "components.h"
#include "mock_request_context.h"

#include <alice/megamind/library/context/responses.h>
#include <alice/megamind/library/request_composite/composite.h>
#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/speechkit/request_build.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <library/cpp/http/io/headers.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

namespace NAlice {

class TTestSpeechKitRequest : public TSpeechKitRequest {
public:
    using TComposite = NMegamind::TTestComponents<NMegamind::TTestRequestParts, NMegamind::TTestEventComponent, NMegamind::TTestClientComponent>;
    using TCompositePtr = TSimpleSharedPtr<TComposite>;

public:
    explicit TTestSpeechKitRequest(TCompositePtr composite)
        : TSpeechKitRequest{*composite}
        , Composite_{composite}
    {
        UNIT_ASSERT_C(Composite_, "composite must not be null when creating TTestSpeechKitRequest");
    }

    TComposite& Composite() {
        return *Composite_;
    }

private:
    TCompositePtr Composite_;
};

class TSpeechKitRequestBuilder {
public:
    using TProtoPatcher = std::function<void(NAlice::NMegamind::TSpeechKitInitContext&)>;
    class TBuildException : public yexception {
    public:
        using yexception::yexception;
    };

    enum class EPredefined : size_t {
        MinimalWithoutEvent,
        MinimalWithTextEvent,
        MinimalWithEmptyTextEvent,
    };

public:
    explicit TSpeechKitRequestBuilder(TStringBuf request);
    explicit TSpeechKitRequestBuilder(NJson::TJsonValue request);
    explicit TSpeechKitRequestBuilder(EPredefined predefined);

    TSpeechKitRequestBuilder& AddHeader(THttpInputHeader header);
    TSpeechKitRequestBuilder& SetPath(TString path);

    TSpeechKitRequestBuilder& SetProtoPatcher(TProtoPatcher patcher);

    TTestSpeechKitRequest Build(THolder<IResponses>* responses = nullptr);
    TSpeechKitRequest::TCompositeHolderPtr BuildCompositePtr(THolder<IResponses>* responses = nullptr);

    NAppHostHttp::THttpRequest BuildHttpRequestProtoItem() const;
    NJson::TJsonValue BuildHttpRequestItem() const;
    NMegamind::TTestInitComponentContext BuildInitContext() const;

    void SetupRequestCtx(NMegamind::TMockRequestCtx& rCtx) const;

    NUri::TUri BuildUri() const;

private:
    NMegamind::TTestInitComponentContext CreateInitContext(THolder<IResponses>*);
    std::unique_ptr<NMegamind::TMockRequestCtx> CreateRequestCtx(IGlobalCtx& globalCtx) const;

private:
    TString Content_;
    THttpHeaders Headers_;
    TCgiParameters CgiParams_;
    TString RngSalt_;
    TString Path_ = "/speechkit/app/pa/";
    TMaybe<TProtoPatcher> ProtoPatcher_;
};

} // namespace NAlice
