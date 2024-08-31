#include "utils.h"

#include <alice/megamind/protos/common/events.pb.h>

#include <alice/library/typed_frame/typed_semantic_frame_request.h>

#include <util/stream/str.h>

using namespace NAlice;
using namespace NAlice::NCuttlefish::NAppHostServices;

namespace {

inline constexpr TStringBuf SEMANTIC_FRAME_REQUEST_NAME = "@@mm_semantic_frame";

TString GetWords(const TSubrequestPtr& req) {
    TString words;
    auto& hypos = req->Request.GetRequest().GetEvent().GetAsrResult();
    if (hypos.size()) {
        TStringOutput so(words);
        for (auto& word : hypos[0].GetWords()) {
            if (words.Size()) {
                so << ' ';
            }
            so << word.GetValue();
        }
    }
    return words;
}

void SetSetraceLabelForVoiceInput(TSubrequestPtr& req, TString words) {
    TStringOutput so(req->SetraceLabel);
    so << " voice_input: " << (words.empty() ? GetWords(req) : words);
}

void SetSetraceLabelForTextInput(TSubrequestPtr& req) {
    TStringOutput so(req->SetraceLabel);
    auto text = req->Request.GetRequest().GetEvent().GetText();
    so << " text_input: " << text;
}

void SetSetraceLabelForImageInput(TSubrequestPtr& req) {
    TStringOutput so(req->SetraceLabel);
    so << " image_input";
}

void SetSetraceLabelForMusicInput(TSubrequestPtr& req) {
    TStringOutput so(req->SetraceLabel);
    so << " music_input";
}

void SetSetraceLabelForServerAction(TSubrequestPtr& req) {
    TStringOutput so(req->SetraceLabel);
    const TString& name = req->Request.GetRequest().GetEvent().GetName();
    if (name == SEMANTIC_FRAME_REQUEST_NAME) {
        try {
            TTypedSemanticFrameRequest frame{
                req->Request.GetRequest().GetEvent().GetPayload(),
                /* validateAnalytics= */ false
            };
            const auto& semanticFrame = frame.SemanticFrame;
            TString sfName;
            if (semanticFrame.HasTypedSemanticFrame()) {
                const auto& tsf = semanticFrame.GetTypedSemanticFrame();
                if (const auto type = tsf.GetTypeCase(); type != TTypedSemanticFrame::TypeCase::TYPE_NOT_SET) {
                    sfName = tsf.GetDescriptor()->FindFieldByNumber(type)->name();
                } else {
                    sfName = "bad content (failed to get semantic frame type)";
                }
            } else {
                sfName = semanticFrame.GetName();
            }
            so << SEMANTIC_FRAME_REQUEST_NAME << ": " << sfName;
        } catch (...) {
            so << SEMANTIC_FRAME_REQUEST_NAME << ": bad content (" << CurrentExceptionMessage() << ')';
        }
    } else {
        so << " callback: " << name;
    }
}

} // anonymous namespace

void SetSetraceLabel(TSubrequestPtr& req, TString words) {
    auto eventType = req->Request.GetRequest().GetEvent().GetType();
    switch (eventType) {
        case EEventType::voice_input:
            SetSetraceLabelForVoiceInput(req, words);
            break;
        case EEventType::text_input:
            SetSetraceLabelForTextInput(req);
            break;
        case EEventType::suggested_input:
            SetSetraceLabelForTextInput(req);
            break;
        case EEventType::image_input:
            SetSetraceLabelForImageInput(req);
            break;
        case EEventType::music_input:
            SetSetraceLabelForMusicInput(req);
            break;
        case EEventType::server_action:
            SetSetraceLabelForServerAction(req);
            break;
    }
}
