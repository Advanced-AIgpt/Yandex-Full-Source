#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <alice/library/proto/proto.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/string/builder.h>
#include <util/string/split.h>
#include <util/random/fast.h>

using namespace NAlice;

const TString PROTOBUF_BINARY_CONTENT_TYPE = "application/protobuf";
const TString PROTOBUF_TEXT_CONTENT_TYPE = "text/protobuf";

const TVector<TVector<TString>> FAKE_DIALOG_HISTORIES = {
    {"Привет", "Как дела?", "Норм, а у тебя?"}
};

TString GenerateAmmo(const TStringBuf path, const TStringBuf contentType, const bool addInternalHeader, const TStringBuf body, const TMaybe<TString> tag = Nothing()) {
    TStringBuilder bodyBuilder;
    bodyBuilder << body << Endl;

    TStringBuilder requestBuilder;
    requestBuilder << "POST " << path << " HTTP/1.1" << Endl;
    requestBuilder << "Content-Length: " << bodyBuilder.size() << Endl;
    requestBuilder << "Content-Type: " << contentType << Endl;
    if (addInternalHeader) {
        requestBuilder << "X-Yandex-Internal-Request: 1" << Endl;
    }
    requestBuilder << "Host: localhost" << Endl;
    requestBuilder << Endl;
    requestBuilder << bodyBuilder;

    TStringBuilder ammoBuilder;
    ammoBuilder << requestBuilder.size();
    if (tag) {
        ammoBuilder << " " << tag.GetRef();
    }
    ammoBuilder << Endl << requestBuilder << Endl;

    return ammoBuilder;
}

void AddDialogHistory(NScenarios::TScenarioRunRequest& request, const TVector<TString>& dialogHistory) {
    NScenarios::TDialogHistoryDataSource dialogHistoryDataSource;
    for (const auto& phrase : dialogHistory) {
        *dialogHistoryDataSource.AddPhrases() = phrase;
    }
    auto& dataSource = (*request.MutableDataSources())[EDataSourceType::DIALOG_HISTORY];
    *dataSource.MutableDialogHistory() = std::move(dialogHistoryDataSource);
}

void AddState(NScenarios::TScenarioRunRequest& request, const TVector<TString>& usedReplies) {
    NHollywood::NGeneralConversation::TSessionState sessionState;
    for (const auto& phrase : usedReplies) {
        auto* used = sessionState.AddUsedRepliesInfo();
        used->SetHash(THash<TString>{}(phrase));
        used->SetText(phrase);
    }
    request.MutableBaseRequest()->MutableState()->PackFrom(sessionState);
}

void AddExperiments(NScenarios::TScenarioRunRequest& request, const TString& experiments) {
    auto& experimentsProto = *request.MutableBaseRequest()->MutableExperiments();
    for (const auto& it : StringSplitter(experiments).Split(',')) {
        (*experimentsProto.mutable_fields())[ToString(it.Token())].set_number_value(1);
    }
}

int main(int argc, const char *argv[]) {
    bool addDialogHistory = false;
    bool addState = false;
    bool addInternalHeader = false;
    TString experiments;
    TString path;

    NLastGetopt::TOpts opts;
    opts.AddLongOption("add-dialog-history")
        .Help("Add fake dialog history")
        .NoArgument()
        .Optional()
        .StoreTrue(&addDialogHistory);
    opts.AddLongOption("add-state")
        .Help("Add fake state")
        .NoArgument()
        .Optional()
        .StoreTrue(&addState);
    opts.AddLongOption("experiments")
        .Help("Experiments comma separated")
        .Optional()
        .StoreResult(&experiments);
    opts.AddLongOption("add-internal-header")
        .Help("Adding X-Yandex-Internal-Request into header (needed for srcrwr to work)")
        .NoArgument()
        .Optional()
        .StoreTrue(&addInternalHeader);
    opts.AddLongOption("path")
        .Help("url")
        .DefaultValue("/general_conversation/run")
        .StoreResult(&path);

    opts.SetFreeArgsNum(0, 0);

    NLastGetopt::TOptsParseResult optsParseResult(&opts, argc, argv);
    TFastRng64 rng(1);
    TString line;
    while (Cin.ReadLine(line)) {
        if (!line.StartsWith("BaseRequest")) {
            continue;
        }
        auto request = NAlice::ParseProtoText<NScenarios::TScenarioRunRequest>(line);
        if (addDialogHistory) {
            AddDialogHistory(request, FAKE_DIALOG_HISTORIES[rng.GenRand() % FAKE_DIALOG_HISTORIES.size()]);
        }
        if (addState) {
            AddState(request, FAKE_DIALOG_HISTORIES[rng.GenRand() % FAKE_DIALOG_HISTORIES.size()]);
        }
        if (experiments) {
            AddExperiments(request, experiments);
        }

        Cout << GenerateAmmo(path, PROTOBUF_TEXT_CONTENT_TYPE, addInternalHeader, NAlice::SerializeProtoText(request, true)) << Endl;
//        Cout << GenerateAmmo("/general_conversation/run", PROTOBUF_BINARY_CONTENT_TYPE, request.SerializeAsString()) << Endl;
    }

    return 0;
}
