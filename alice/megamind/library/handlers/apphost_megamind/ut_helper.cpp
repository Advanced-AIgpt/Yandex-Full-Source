#include "ut_helper.h"

#include "begemot.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/protos/data/language/language.pb.h>

#include <library/cpp/testing/unittest/tests_data.h>

#include <util/folder/path.h>

#include <alice/megamind/library/apphost_request/protos/uniproxy_request.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/library/apphost_request/protos/stage_timestamp.pb.h>

namespace NAlice::NMegamind::NTesting {
using namespace testing;

namespace {
TScenarioConfig MakeScenarioConfig(const TString& name) {
    TScenarioConfig sc;
    sc.SetName(name);
    sc.SetAcceptsAnyUtterance(true);
    sc.SetEnabled(true);
    sc.AddLanguages(ELang::L_RUS);
    sc.MutableHandlers()->SetRequestType(NAlice::ERequestType::AppHostPure);
    return sc;
}

} // namespace


TAppHostWalkerTestFixture::TAppHostWalkerTestFixture()
    : AhCtx{CreateAppHostContext()}
    , GeoLookup_{JoinFsPaths(GetWorkPath(), "geodata5-xurma.bin")}
    , FormulasStorage_{Storage_, {}}
{
    EXPECT_CALL(GlobalCtx, GeobaseLookup()).WillRepeatedly(ReturnRef(GeoLookup_));
    EXPECT_CALL(GlobalCtx, ScenarioConfigRegistry()).WillRepeatedly(ReturnRef(ScenarioRegistry_));
    EXPECT_CALL(GlobalCtx, GetFormulasStorage()).WillRepeatedly(ReturnRef(FormulasStorage_));
}

void TAppHostWalkerTestFixture::RegisterScenario(const TString& name) {
    ScenarioRegistry_.AddScenarioConfig(MakeScenarioConfig(name));
}

void TAppHostWalkerTestFixture::InitBegemot(TStringBuf resourceId) {
    NJson::TJsonValue response;
    NJson::ReadJsonFastTree(NResource::Find(resourceId), &response);
    NBg::NProto::TAlicePolyglotMergeResponseResult protoResponse;
    Y_PROTOBUF_SUPPRESS_NODISCARD protoResponse.ParseFromString(Base64Decode(response["response"][0]["binary"].GetString()));

    AhCtx.TestCtx().AddProtobufItem(protoResponse, AH_ITEM_BEGEMOT_ALICE_POLYGLOT_MERGER_RESPONSE_NAME, NAppHost::EContextItemKind::Input);
}

void TAppHostWalkerTestFixture::InitSkr(const TSpeechKitRequestBuilder& skrBuilder) {
    FakeSkrInit(AhCtx, skrBuilder);
}

void FakeSkrInit(TTestAppHostCtx& ahCtx, const TSpeechKitRequestBuilder& skrBuilder) {
    ahCtx.TestCtx().AddProtobufItem(skrBuilder.BuildHttpRequestProtoItem(),
                                    NAppHost::PROTO_HTTP_REQUEST,
                                    NAppHost::EContextItemKind::Input);

    TAppHostSkrNodeHandler nodeHandler{ahCtx.GlobalCtx(), false};
    nodeHandler.Execute(ahCtx);

    auto prt = ahCtx.TestCtx().GetOnlyProtobufItem<NMegamindAppHost::TUniproxyRequestInfoProto>("mm_uniproxy_request", NAppHost::EContextItemSelection::Output);
    ahCtx.TestCtx().AddProtobufItem(prt, "mm_uniproxy_request", NAppHost::EContextItemKind::Input);

    auto sk = ahCtx.TestCtx().GetOnlyProtobufItem<TSpeechKitRequestProto>("mm_speechkit_request", NAppHost::EContextItemSelection::Output);
    ahCtx.TestCtx().AddProtobufItem(sk, "mm_speechkit_request", NAppHost::EContextItemKind::Input);

    auto ev = ahCtx.TestCtx().GetOnlyProtobufItem<TEvent>("mm_skr_event", NAppHost::EContextItemSelection::Output);
    ahCtx.TestCtx().AddProtobufItem(ev, "mm_skr_event", NAppHost::EContextItemKind::Input);

    auto ci = ahCtx.TestCtx().GetOnlyProtobufItem<NMegamindAppHost::TClientItem>("mm_skr_client_info", NAppHost::EContextItemSelection::Output);
    ahCtx.TestCtx().AddProtobufItem(ci, "mm_skr_client_info", NAppHost::EContextItemKind::Input);

    auto stt = ahCtx.TestCtx().GetOnlyProtobufItem<NMegamindAppHost::TStageTimestamp>("mm_stage_timestamp", NAppHost::EContextItemSelection::Output);
    ahCtx.TestCtx().AddProtobufItem(stt, "mm_stage_timestamp", NAppHost::EContextItemKind::Input);

    auto rnm = ahCtx.TestCtx().GetOnlyProtobufItem<NMegamindAppHost::TRequiredNodeMeta>("mm_required_node_meta", NAppHost::EContextItemSelection::Output);
    ahCtx.TestCtx().AddProtobufItem(rnm, "mm_required_node_meta", NAppHost::EContextItemKind::Input);

    auto mhr = ahCtx.TestCtx().GetOnlyProtobufItem<NAppHostHttp::THttpRequest>("mm_misspell_http_request", NAppHost::EContextItemSelection::Output);
    ahCtx.TestCtx().AddProtobufItem(mhr, "mm_misspell_http_request", NAppHost::EContextItemKind::Input);
}

} // namespace NAlice::NMegamind::NTesting
