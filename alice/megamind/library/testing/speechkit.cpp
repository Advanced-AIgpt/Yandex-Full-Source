#include "speechkit.h"
#include "components.h"
#include "mock_global_context.h"
#include "mock_request_context.h"
#include "speechkit_api_builder.h"

#include <alice/megamind/library/speechkit/request.h>
#include <alice/megamind/library/speechkit/request_build.h>
#include <alice/megamind/library/speechkit/request_parser.h>

#include <alice/library/logger/logger.h>

namespace NAlice {
namespace {

const std::array<TString, 3> PREDEFINED_REQUESTS = {
    {
        ToString(TSpeechKitApiRequestBuilder().BuildJson()),
        ToString(TSpeechKitApiRequestBuilder().SetTextInput("hello").BuildJson()),
        ToString(TSpeechKitApiRequestBuilder().SetTextInput("").BuildJson()),
    }
};

} // namespace

TSpeechKitRequestBuilder::TSpeechKitRequestBuilder(TStringBuf request)
    : Content_{request}
{
}

TSpeechKitRequestBuilder::TSpeechKitRequestBuilder(NJson::TJsonValue request)
    : Content_{ToString(request)}
{
}

TSpeechKitRequestBuilder::TSpeechKitRequestBuilder(EPredefined predefined)
    : Content_{PREDEFINED_REQUESTS[static_cast<size_t>(predefined)]}
{
}

TSpeechKitRequestBuilder& TSpeechKitRequestBuilder::AddHeader(THttpInputHeader header) {
    Headers_.AddHeader(std::move(header));
    return *this;
}

TSpeechKitRequestBuilder& TSpeechKitRequestBuilder::SetPath(TString path) {
    Path_ = std::move(path);
    return *this;
}

TSpeechKitRequestBuilder& TSpeechKitRequestBuilder::SetProtoPatcher(TProtoPatcher patcher) {
    ProtoPatcher_ = patcher;
    return *this;
}

TTestSpeechKitRequest TSpeechKitRequestBuilder::Build(THolder<IResponses>* responses) {
    auto init = CreateInitContext(responses);
    return TTestSpeechKitRequest{MakeSimpleShared<TTestSpeechKitRequest::TComposite>(init)};
}

TSpeechKitRequest::TCompositeHolderPtr TSpeechKitRequestBuilder::BuildCompositePtr(THolder<IResponses>* responses) {
    class TTestCompositeHolder final : public TSpeechKitRequest::TCompositeHolder {
    public:
        TTestCompositeHolder(NMegamind::TTestInitComponentContext& init)
            : Composite_{init}
        {
        }

        TSpeechKitRequest View() const override {
            return Composite_;
        }

    private:
        TTestSpeechKitRequest::TComposite Composite_;
    };

    auto init = CreateInitContext(responses);
    return std::make_unique<TTestCompositeHolder>(init);
}

NMegamind::TTestInitComponentContext TSpeechKitRequestBuilder::CreateInitContext(THolder<IResponses>* responses) {
    TMockGlobalContext globalCtx{TMockGlobalContext::EInit::GenericInit};
    auto requestCtx = CreateRequestCtx(globalCtx);

    NMegamind::TTestInitComponentContext init{CgiParams_, Headers_, Path_, ""};

    THolder<IResponses> responsesFake;
    if (!responses) {
        responses = &responsesFake;
    }

    if (const auto e = NMegamind::ParseSkRequest(*requestCtx, init)) {
        ythrow TBuildException{} << *e;
    }
    if (ProtoPatcher_) {
        ProtoPatcher_.GetRef()(init);
    }
    return init;
}

NAppHostHttp::THttpRequest TSpeechKitRequestBuilder::BuildHttpRequestProtoItem() const {
    NAppHostHttp::THttpRequest httpRequest;
    TStringBuilder uri;
    uri << Path_;
    if (!CgiParams_.empty()) {
        uri << '?' << CgiParams_.Print();
    }
    httpRequest.SetPath(uri);
    httpRequest.SetContent(Content_);
    httpRequest.SetScheme(NAppHostHttp::THttpRequest::EScheme::THttpRequest_EScheme_Http);
    httpRequest.SetMethod(NAppHostHttp::THttpRequest::EMethod::THttpRequest_EMethod_Post);
    return httpRequest;
}

NJson::TJsonValue TSpeechKitRequestBuilder::BuildHttpRequestItem() const {
    NJson::TJsonValue httpRequest;
    TStringBuilder uri;
    uri << Path_;
    if (!CgiParams_.empty()) {
        uri << '?' << CgiParams_.Print();
    }
    httpRequest["uri"] = uri;
    httpRequest["content"] = Content_;
    return httpRequest;
}

void TSpeechKitRequestBuilder::SetupRequestCtx(NMegamind::TMockRequestCtx& rCtx) const {
    EXPECT_CALL(testing::Const(rCtx), Body()).WillRepeatedly(testing::ReturnRefOfCopy(Content_));
}

NUri::TUri TSpeechKitRequestBuilder::BuildUri() const {
    return NUri::TUri{"", 80, Path_, CgiParams_.Print()};
}

NMegamind::TTestInitComponentContext TSpeechKitRequestBuilder::BuildInitContext() const {
    TMockGlobalContext globalCtx{TMockGlobalContext::EInit::GenericInit};
    auto requestCtx = CreateRequestCtx(globalCtx);

    THolder<IResponses> responses;
    NMegamind::TTestInitComponentContext initCtx{CgiParams_, Headers_, Path_, /* rngSalt= */"42"};
    if (auto e = NMegamind::ParseSkRequest(*requestCtx, initCtx)) {
        ythrow yexception() << "unable to parse speechkit request: " << e->ErrorMsg;
    }
    return initCtx;
}

std::unique_ptr<NMegamind::TMockRequestCtx> TSpeechKitRequestBuilder::CreateRequestCtx(IGlobalCtx& globalCtx) const {
    using namespace NAlice::NMegamind;
    TMockInitializer init;
    init.Uri = BuildUri();
    init.Cgi = CgiParams_;
    init.Headers = Headers_;

    auto ctx = std::make_unique<TMockRequestCtx>(globalCtx, std::move(init));
    EXPECT_CALL(*ctx, Body()).WillRepeatedly(testing::ReturnRefOfCopy(Content_));

    return ctx;
}

} // namespace NAlice
