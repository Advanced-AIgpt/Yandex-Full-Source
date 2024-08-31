#include "wizard_response.h"

#include <alice/megamind/library/experiments/flags.h>

#include <alice/begemot/lib/utils/form_to_frame.h>
#include <alice/library/search/defs.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/json/json.h>

#include <library/cpp/iterator/zip.h>

#include <search/begemot/apphost/json.h>
#include <search/begemot/rules/granet/proto/granet.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NAlice {

namespace {

// Threshold was tested in EXPERIMENTS-67690
constexpr double SEARCH_QUERY_ANAPHORA_SUBSTITUTION_THRESHOLD = 0.9;

THashSet<TString> GetGranetFrameNames(const TParsedFramesResponse& ParsedFramesResponse_) {
    auto result = THashSet<TString>(ParsedFramesResponse_.GetFrames().size());
    for (const auto& [frame, source] : Zip(ParsedFramesResponse_.GetFrames(), ParsedFramesResponse_.GetSources())) {
        if (source == GRANET_SOURCE_LABEL) {
            result.insert(frame.GetName());
        }
    }
    return result;
}

TString ExtractRewrittenRequest(const ::NBg::NProto::TAliceResponseResult& aliceResponse) {
    if (aliceResponse.GetAliceEllipsisRewriter().HasRewrittenRequest()) {
        return aliceResponse.GetAliceEllipsisRewriter().GetRewrittenRequest();
    }
    if (!aliceResponse.GetAliceAnaphoraSubstitutor().GetSubstitution().empty()) {
        return aliceResponse.GetAliceAnaphoraSubstitutor().GetSubstitution(0).GetRewrittenRequest();
    }
    return TString();
}

TVector<TSemanticFrame> ReplaceRecognizedActionWithFrameEffect(const TVector<TSemanticFrame>& frames,
                                                               const TVector<TString>& sources,
                                                               const TVector<TSemanticFrame>& recognizedActionEffectFrames)
{
    TVector<TSemanticFrame> result;
    size_t actionRecognizerFrameCount = 0;
    for (const auto& [frame, source] : Zip(frames, sources)) {
        if (source == ACTION_RECOGNIZER_SOURCE_LABEL) {
            Y_ASSERT(actionRecognizerFrameCount < 1);
            ++actionRecognizerFrameCount;
            for (const TSemanticFrame& recognizedActionEffectFrame : recognizedActionEffectFrames) {
                result.push_back(recognizedActionEffectFrame);
            }
        } else {
            result.push_back(frame);
        }
    }
    return result;
}

THashMap<TString, size_t> BuildNameToFrameIndex(const TVector<TSemanticFrame>& frames) {
    auto result = THashMap<TString, size_t>(frames.size());
    for (size_t index = 0; index < frames.size(); ++index) {
        const auto& [ it, inserted ] = result.insert({frames[index].GetName(), index});
        if (!inserted) {
            // TODO(the0): Log warning here
        }
    }
    return result;
}

TMaybe<TString> TryGetAnaphoraRewrittenQueryForSearch(const NJson::TJsonValue& json) {
    const auto* anaphoraSubstitutorResponse = json.GetValueByPath("rules.AliceAnaphoraSubstitutor.Substitution");
    if (!anaphoraSubstitutorResponse) {
        return Nothing();
    }

    if (anaphoraSubstitutorResponse->GetArray().empty()) {
        return Nothing();
    }
    const auto& topHypothesis = (*anaphoraSubstitutorResponse)[0];
    if (topHypothesis["Score"].GetDoubleSafe(0) > SEARCH_QUERY_ANAPHORA_SUBSTITUTION_THRESHOLD) {
        return topHypothesis["RewrittenRequest"].GetString();
    }

    return Nothing();
}

TMaybe<TString> TryGetSearchQueryPrepareResult(const NJson::TJsonValue& json) {
    const auto* queryPrepareResponse = json.GetValueByPath("rules.AliceSearchQueryPreparer.Frame");
    if (!queryPrepareResponse) {
        return Nothing();
    }

    TSemanticFrame frame;
    JsonToProto(*queryPrepareResponse, frame);
    for (const auto& slot : frame.GetSlots()) {
        if (slot.GetName() == "query" && slot.GetTypedValue().GetString()) {
            return slot.GetTypedValue().GetString();
        }
    }

    return Nothing();
}

} // namespace

namespace {

// TODO(alexanderplat): remove this json serialization code
// TODO: mostly copypaste, move it to search/begemot/apphost/json.h
template <typename TResult, typename TWriter>
void KosherRuleResultAsString(TWriter& out, const TStringBuf name, const TResult& res, auto& toplevelFields, ::NBg::NSerialization::EDebug debug) {
    auto result = CollectFields(res, &toplevelFields, debug);
    if (result) {
        out.WriteKey(name).BeginObject();
        for (const auto& kv : result) {
            out.WriteKey(kv.first);
            const ::NBg::NSerialization::TJsonResultField& r = kv.second;
            const bool asArray = r.AlwaysArray || r.Value.size() != 1;
            if (asArray) {
                out.BeginList();
            }
            for (const auto& v : r.Value) {
                ::NBg::NSerialization::TProto2JsonWriter<TWriter>{&out}(v, r.InlineJson);
            }
            if (asArray) {
                out.EndList();
            }
        }
        out.EndObject();
    }
}

NJson::TJsonValue ConvertBegemotResponseToJson(const NBg::NProto::TAliceResponseResult& protoResponse, const bool needGranetLog) {
    NJson::TJsonValue json(NJson::JSON_MAP);
    NJsonWriter::TBuf content(NJsonWriter::HEM_UNSAFE);
    content.BeginObject();
    content.WriteKey("rules").BeginObject();
    ::NBg::NSerialization::EDebug debug = needGranetLog ? ::NBg::NSerialization::EDebug::Yes : ::NBg::NSerialization::EDebug::No;
    const auto* descr = protoResponse.GetDescriptor();
    Y_ENSURE(descr, "Cannot get a proto descriptor!");
    const auto* reflection = protoResponse.GetReflection();
    Y_ENSURE(reflection, "Cannot get a proto reflection!");
    TVector<std::pair<const google::protobuf::Message*, const google::protobuf::FieldDescriptor*>> toplevelFields;
    for (int fieldIdx = 0; fieldIdx < descr->field_count(); ++fieldIdx) {
        const NProtoBuf::FieldDescriptor* field = descr->field(fieldIdx);
        KosherRuleResultAsString<google::protobuf::Message>(content, field->name(), reflection->GetMessage(protoResponse, field),
                                                            toplevelFields, debug);
    }
    content.EndObject(); // "rules"
    for (auto [msg, desc] : toplevelFields) {
        if (!msg || !desc) {
            continue;
        }
        auto jsonState = ::NBg::NSerialization::SaveJsonState(content);
        try {
            const TString& overrideKey = desc->options().GetExtension(toplevel);
            const TString& key = overrideKey ? overrideKey : desc->name();
            content.WriteKey(key);
            ::NBg::NSerialization::TProto2JsonWriter<NJsonWriter::TBuf>{&content}(*msg, desc);
        } catch (const yexception &e) {
            ::NBg::NSerialization::RestoreJsonState(content, *jsonState);
        }
    }
    content.EndObject();

    NJson::TJsonValue begemotResponse;
    NJson::ReadJsonFastTree(content.Str(), &begemotResponse, true);
    return begemotResponse;
}

} // namespace

TWizardResponse::TWizardResponse(NBg::NProto::TAlicePolyglotMergeResponseResult alicePolyglotMergeResponse, const bool needGranetLog)
    : ProtoWizardResponse_(std::move(alicePolyglotMergeResponse))
    , RawWizardResponse_(ConvertBegemotResponseToJson(ProtoWizardResponse_.GetAliceResponse(), needGranetLog))
    , ParsedFramesResponse_(ProtoWizardResponse_.GetAliceResponse().GetAliceParsedFrames())
    , NameToFrameIndex_(BuildNameToFrameIndex(ParsedFramesResponse_.GetFrames()))
    , AvailableGranetFrameNames_(GetGranetFrameNames(ParsedFramesResponse_))
    , Fixlist_(ProtoWizardResponse_.GetAliceResponse().GetAliceFixlist())
    , RewrittenRequest_(ExtractRewrittenRequest(ProtoWizardResponse_.GetAliceResponse()))
{
}

TVector<TSemanticFrame> TWizardResponse::GetRequestFrames(const TVector<TSemanticFrame>& recognizedActionEffectFrames) const {
    if (recognizedActionEffectFrames.empty()) {
        return ParsedFramesResponse_.GetFrames();
    }
    return ReplaceRecognizedActionWithFrameEffect(
        ParsedFramesResponse_.GetFrames(),
        ParsedFramesResponse_.GetSources(),
        recognizedActionEffectFrames
    );
}

TMaybe<TString> TWizardResponse::GetNormalizedUtterance() const {
    if (ProtoWizardResponse_.GetAliceResponse().GetAliceNormalizer().HasNormalizedRequest()) {
        return ProtoWizardResponse_.GetAliceResponse().GetAliceNormalizer().GetNormalizedRequest();
    }
    return Nothing();
}

TMaybe<TString> TWizardResponse::GetNormalizedTranslatedUtterance() const {
    if (ProtoWizardResponse_.GetTranslatedResponse().GetAliceNormalizer().HasNormalizedRequest()) {
        return ProtoWizardResponse_.GetTranslatedResponse().GetAliceNormalizer().GetNormalizedRequest();
    }
    return Nothing();
}

TMaybe<TSemanticFrame> TWizardResponse::TryGetRecognizedAction() const {
    return ParsedFramesResponse_.GetRecognizedAction();
}

TMaybe<TString> TWizardResponse::GetSearchQuery(const THashMap<TString, TMaybe<TString>>& expFlags) const {
    if (expFlags.contains(EXP_USE_SEARCH_QUERY_PREPARE_RULE)) {
        return TryGetSearchQueryPrepareResult(RawWizardResponse_);
    }

    if (const auto query = TryGetAnaphoraRewrittenQueryForSearch(RawWizardResponse_); query.Defined() && !query->Empty()) {
        return query;
    }

    if (const auto* searchFrame = GetRequestFrame(NSearch::SEARCH_FORM); searchFrame) {
        if (const auto* slot = FindIfPtr(searchFrame->GetSlots(), [](const TSemanticFrame::TSlot& slot){
                return slot.GetName() == NSearch::SLOT_QUERY;
            }))
        {
            return slot->GetValue();
        }
    }

    return Nothing();
}

void TWizardResponse::DumpQualityInfo(TRTLogger& logger, ELogPriority severity = TLOG_DEBUG) const {
    LOG_WITH_TYPE(logger, severity, ELogMessageType::MegamindPreClasification) << "Begemot quality info. Granet sample mock: "
                          << RawWizardResponse_["rules"]["Granet"]["SampleMock"].GetString();
    LOG_WITH_TYPE(logger, severity, ELogMessageType::MegamindPreClasification) << "Begemot quality info. Embeddings: "
                          << RawWizardResponse_["rules"]["AliceEmbeddingsExport"]["Embeddings"];
    LOG_WITH_TYPE(logger, severity, ELogMessageType::MegamindPreClasification) << "Begemot quality info. AliceParsedFrames: "
                          << RawWizardResponse_["rules"]["AliceParsedFrames"];

    if (!ProtoWizardResponse_.GetLog().empty()) {
        LOG_WITH_TYPE(logger, severity, ELogMessageType::MegamindPreClasification) << "Begemot quality info. Polyglot Merger Log: "
                          << JoinSeq("; ", ProtoWizardResponse_.GetLog());
    }
}
} // namespace NAlice
