#pragma once

#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/context/wizard_response.h>
#include <alice/megamind/library/sources/request.h>
#include <alice/megamind/library/util/status.h>

#include <alice/library/network/request_builder.h>

#include <search/begemot/rules/alice/response/proto/alice_response.pb.h>

#include <util/generic/string.h>

namespace NBg::NProto {
class TAliceAvailableActions;
class TAliceAvailableFrameActions;
class TAliceAvailableActiveSpaceActions;
} // namespace NBg::NProto

namespace NAlice {

class TDialogHistory;

namespace NImpl {
bool IsEnabledAliceTagger(const IContext& ctx, const ELanguage language);
TVector<TString> EnabledBegemotRules(const IContext& ctx, const ELanguage language);
bool IsEnabledResolveContextualAmbiguity(const IContext& ctx, const ELanguage language);
TString StripPhrase(const TString& phrase, size_t maxTokenCount, size_t maxSymbolCount);
void AddJsonDialogHistoryToWizextra(const TDialogHistory& dialogHistory, THashMap<TString, TString>& wizextra);
void AddJsonPhrasesToWizextra(const TDeque<TString>& phrases, const TString& wizextraKey,
                              THashMap<TString, TString>& wizextra);
NBg::NProto::TAliceAvailableActions GetScenarioAvailableActionsProto(const IContext& ctx);
NBg::NProto::TAliceAvailableFrameActions GetScenarioAvailableFrameActionsProto(const IContext& ctx);
NBg::NProto::TAliceAvailableActions GetDeviceStateAvailableActionsProto(const IContext& ctx);
NBg::NProto::TAliceAvailableActiveSpaceActions GetAvailableActiveSpaceActionsProto(const IContext& ctx);
TMaybe<TClientEntity> GetVideoGallery(const IContext& ctx, const TString& galleryName);
THashMap<TString, TString> GetWizextra(const IContext& ctx, TStringBuf text, const ELanguage language, const bool addContacts);

TString JoinWizextra(const THashMap<TString, TString>& wizextra);

void SetWizextraProtoMessage(THashMap<TString, TString>& wizextra, const TString& key,
                             const google::protobuf::Message& msg);

} // namespace NImpl

inline constexpr TStringBuf AH_ITEM_BEGEMOT_ALICE_POLYGLOT_MERGER_RESPONSE_NAME = "mm_begemot_native#AlicePolyglotMergeResponse";

NJson::TJsonValue CreateBegemotConfig();
ui64 GetBegemotBalancingHint(const NJson::TJsonValue& request);

TSourcePrepareStatus CreateNativeBegemotRequest(const TString& text, const ELanguage language, const IContext& ctx, NJson::TJsonValue& request);
NJson::TJsonValue SetRequiredRules(NJson::TJsonValue request, const IContext& ctx, const ELanguage language);
NJson::TJsonValue CreateBegemotMergerRequest(const NJson::TJsonValue& begemotRequest, const IContext& ctx);
NJson::TJsonValue CreatePolyglotBegemotMergerMergerRequest(const NJson::TJsonValue& begemotRequest);

TSourcePrepareStatus CreateBegemotRequest(const TString& text, const ELanguage language, const IContext& ctx, NNetwork::IRequestBuilder& request);

TErrorOr<TWizardResponse> ParseBegemotAliceResponse(NBg::NProto::TAlicePolyglotMergeResponseResult&& begemotResponse, const bool needGranetLog);

} // namespace NAlice
