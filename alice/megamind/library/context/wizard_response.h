#pragma once

#include "fixlist.h"
#include "parsed_frames.h"

#include <alice/megamind/protos/common/frame.pb.h>

#include <search/begemot/rules/alice/polyglot_merge_response/proto/alice_polyglot_merge_response.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>
#include <util/generic/lazy_value.h>

#include <limits>

namespace NAlice {

class TWizardResponse {
public:
    TWizardResponse() = default;
    explicit TWizardResponse(NBg::NProto::TAlicePolyglotMergeResponseResult alicePolyglotMergeResponse, const bool needGranetLog = false);

    bool HasGranetFrame(const TStringBuf scenarioName) const {
        return AvailableGranetFrameNames_.contains(scenarioName);
    }

    TVector<TSemanticFrame> GetRequestFrames(const TVector<TSemanticFrame>& recognizedActionEffectFrames = {}) const;

    const TSemanticFrame* GetRequestFrame(TStringBuf frameName) const {
        if (const size_t* index = NameToFrameIndex_.FindPtr(frameName)) {
            return &ParsedFramesResponse_.GetFrames().at(*index);
        }
        return nullptr;
    }

    const TFixlist& GetFixlist() const {
        return Fixlist_;
    }

    const NJson::TJsonValue& RawResponse() const {
        return RawWizardResponse_;
    }

    double GetFrameConfidence(const TStringBuf frameName) const {
        return ParsedFramesResponse_.GetFrameConfidence(frameName);
    }

    const TString& GetRewrittenRequest() const {
        return RewrittenRequest_;
    }

    TMaybe<TString> GetSearchQuery(const THashMap<TString, TMaybe<TString>>& expFlags) const;

    TMaybe<TString> GetNormalizedUtterance() const;
    TMaybe<TString> GetNormalizedTranslatedUtterance() const;

    TMaybe<TSemanticFrame> TryGetRecognizedAction() const;
    const NBg::NProto::TAliceResponseResult& GetProtoResponse() const {
        return ProtoWizardResponse_.GetAliceResponse();
    }

    void DumpQualityInfo(TRTLogger& logger, ELogPriority severity) const;

private:
    NBg::NProto::TAlicePolyglotMergeResponseResult ProtoWizardResponse_;
    NJson::TJsonValue RawWizardResponse_;
    TParsedFramesResponse ParsedFramesResponse_;
    THashMap<TString, size_t> NameToFrameIndex_;
    THashSet<TString> AvailableGranetFrameNames_;
    TFixlist Fixlist_;
    TString RewrittenRequest_;
};

} // namespace NAlice
