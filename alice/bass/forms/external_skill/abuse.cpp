#include "abuse.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {
namespace NExternalSkill {
namespace {

constexpr TStringBuf SUBSTITUTE_TEXT_MARKER = "<censored>";
constexpr TStringBuf SUBSTITUTE_TTS_MARKER = "<speaker audio=\"beep_mat_1.opus\">";

} // namespace

void TAbuse::AddString(NSc::TValue string, bool isForTTS) {
    if (TString str{string.GetString()}) {
        Strings[str].emplace_back(TDescr{isForTTS, std::move(string)});
    }
}

bool TAbuse::Substitute(TContext& ctx) {
    if (Strings.empty()) {
        return true;
    }

    THolder<NHttpFetcher::TRequest> r = ctx.GetSources().AbuseApi().Request();
    if (!r) {
        LOG(ERR) << "Unable to create request for abuse api" << Endl;
        return false;
    }

    // {"jsonrpc": "2.0", "method": "tmu.check_all", "params": [[{"uri": "doc1", "document": "первый документ"}, {"uri": "doc2", "document": "второй документ"}]], "id": 1}
    NSc::TValue body;
    body["method"].SetString("tmu.check_all");
    body["jsonrpc"].SetString("2.0");
    body["id"].SetIntNumber(1);
    NSc::TValue& docs = body["params"]["documents"];

    for (const auto& kv : Strings) {
        NSc::TValue& doc = docs.Push();
        doc["document"].SetString(kv.first);
        doc["uri"].SetString(kv.first);
    }

    r->SetBody(body.ToJson(), TStringBuf("POST"));
    r->SetContentType(TStringBuf("application/json"));
    NHttpFetcher::TResponse::TRef resp = r->Fetch()->Wait();
    if (!resp->IsHttpOk()) {
        LOG(ERR) << "Unable to fetch abuse api: " << resp->GetErrorText() << Endl;
        return false;
    }

    const NSc::TValue respJson{NSc::TValue::FromJson(resp->Data)};
    for (const NSc::TValue& r : respJson["result"].GetArray()) {
        const NSc::TValue& data = r["result"];
        // Ususally it contains only one key (CENSORED_TEXT) and it means
        // that there isn't any offencive words.
        if (data.DictSize() < 2) {
            continue;
        }

        constexpr TStringBuf blockOpen = "<censored>";
        constexpr TStringBuf blockClose = "</censored>";

        TStringBuilder textString;
        TStringBuilder ttsString;
        auto addText = [&textString, &ttsString](TStringBuf text) {
            textString << text;
            ttsString << text;
        };
        auto addMarker = [&textString, &ttsString]() {
            textString << SUBSTITUTE_TEXT_MARKER;
            ttsString << SUBSTITUTE_TTS_MARKER;
        };

        TStringBuf censored = data["CENSORED_TEXT"].GetString();
        for (size_t pos = censored.find(blockOpen); pos != censored.npos; pos = censored.find(blockOpen)) {
            addText(censored.Head(pos));
            addMarker();
            censored.Skip(censored.find(blockClose) + blockClose.size());
        }

        if (censored) {
            addText(censored);
        }

        TVector<TDescr>* strs = Strings.FindPtr(r["uri"].GetString());
        if (!strs) {
            LOG(ERR) << "must not happened: unable to find string in map" << Endl;
            continue;
        }

        for (TDescr& v : *strs) {
            v.Node.SetString(v.IsForTTS ? ttsString : textString);
        }
    }

    return true;
}

} // namespace NExternalSkill
} // namespace NBASS
