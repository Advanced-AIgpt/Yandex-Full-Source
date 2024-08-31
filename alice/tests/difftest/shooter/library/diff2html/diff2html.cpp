#include "diff2html.h"

#include <alice/joker/library/log/log.h>
#include <alice/hollywood/protos/bass_request_rtlog_token.pb.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <contrib/libs/re2/re2/re2.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/hash_set.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/system/shellcommand.h>
#include <util/system/tempfile.h>

#include <apphost/lib/client/client_shoot.h>
#include <apphost/lib/common/statistics.h>
#include <apphost/lib/proto_answers/http.pb.h>
#include <apphost/lib/compression/compression.h>
#include <apphost/lib/transport/transport_communication.h>

namespace NAlice::NShooter {

namespace {

constexpr auto HTML_HEADER = TStringBuf(R"(<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <title>Диффы</title>
    <meta name="description" content="Диффы от обстрела MM+BASS+VINS">

    <!-- Stylesheet -->
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/9.13.1/styles/github.min.css" />
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/diff2html/bundles/css/diff2html.min.css" />
    <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css">

    <!-- Javascripts -->
    <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.2.1/jquery.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/diff2html/bundles/js/diff2html-ui.min.js"></script>
    <script src="https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/js/bootstrap.min.js"></script>
</head>)");

constexpr auto HTML_BODY_BEGINNING = TStringBuf(R"(<body>
    <div id="flex-container" style="width:90%; margin:0 auto;">
        <div id="desc">Результаты дифф теста</div>
        <a id="mode"></a>
        <div id="elem-content"></div>
    </div>
    <script>
    document.addEventListener('DOMContentLoaded', () => {
        const urlParams = new URLSearchParams(window.location.search);
        const mode = urlParams.get('mode');
        const format = (mode == 'unified') ? 'unified' : 'side-by-side';
        var modeObj = document.getElementById("mode");
        if(format == 'unified') {
            modeObj.innerText = 'Показывать все диффы в двух столбцах';
            modeObj.href = '?mode=side-by-side';
        } else {
            modeObj.innerText = 'Показывать все диффы в одном столбце';
            modeObj.href = '?mode=unified';
        }
        const diffString = `)");

constexpr auto HTML_BODY_ENDING = TStringBuf(R"(`;
        const targetElement = document.getElementById('elem-content');
        const configuration = {
            inputFormat: 'json',
            drawFileList: true,
            matching: 'lines',
            highlight: true,
            fileListStartVisible: false,
            outputFormat: format
        };
        const diff2htmlUi = new Diff2HtmlUI(targetElement, diffString, configuration);
        diff2htmlUi.draw();
        diff2htmlUi.fileListToggle(true);
    });
    </script>
</body>
</html>)");

const TVector<TString> ERRORS = {
    "Прошу прощения, что-то сломалось.",
    "Произошла какая-то ошибка.",
    "Извините, что-то пошло не так.",
    "Даже идеальные помощники иногда ломаются.",
    "Мне кажется, меня уронили.",
    "О, кажется, мы с вами нашли во мне ошибку. Простите.",
    "Мы меня сломали, но я обязательно починюсь.",
};

bool HasError(const TString& response) {
    for (const TString& error : ERRORS) {
        if (response.Contains(error)) {
            return true;
        }
    }
    return false;
}

THashSet<TString> ObtainRequestIds(const TFsPath& responsesPath) {
    TVector<TFsPath> children;
    responsesPath.List(children);

    THashSet<TString> set;
    for (const auto& path : children) {
        if (path.IsDirectory() && path.Child("response.json").Exists()) {
            TString response = TFileInput{path.Child("response.json")}.ReadAll();
            if (!HasError(response)) {
                set.emplace(path.GetName());
            }
        }
    }
    return set;
}

std::pair<int, int> ObtainUniqueErrorsCount(const THashSet<TString>& set1, const THashSet<TString>& set2) {
    int errors1 = 0, errors2 = 0;
    for (const auto& val : set1) {
        if (!set2.contains(val)) {
            ++errors1;
        }
    }
    for (const auto& val : set2) {
        if (!set1.contains(val)) {
            ++errors2;
        }
    }
    return {errors1, errors2};
}

void WipeFields(NJson::TJsonValue& value, const TVector<TStringBuf>& path, size_t pathPos = 0) {
    TStringBuf key = path[pathPos];
    if (!value.Has(key)) {
        return;
    }
    if (pathPos == path.size() - 1) {
        value.EraseValue(key);
        return;
    }

    NJson::TJsonValue& nextValue = value[key];
    if (nextValue.IsMap()) {
        WipeFields(nextValue, path, pathPos + 1);
    } else if (nextValue.IsArray()) {
        for (auto& val : nextValue.GetArraySafe()) {
            WipeFields(val, path, pathPos + 1);
        }
    }
}

void UnpackInnerJsons(NJson::TJsonValue& value, bool unpack = false) {
    if (value.IsArray()) {
        for (auto& v : value.GetArraySafe()) {
            UnpackInnerJsons(v, unpack);
        }
    } else if (value.IsMap()) {
        for (auto& p : value.GetMapSafe()) {
            auto& val = p.second;
            if (val.IsString()) {
                try {
                    TStringStream ss{val.GetString()};
                    NJson::TJsonValue inner = NJson::ReadJsonTree(&ss, /* throwOnError = */ true);
                    UnpackInnerJsons(inner, unpack);
                    if (unpack) {
                        val = inner;
                    } else {
                        val = NJson::WriteJson(inner, /* formatOutput = */ false, /* sortKeys = */ true);
                    }
                } catch (...) {
                    // Not an error
                }
            } else if (val.IsMap() || val.IsArray()) {
                UnpackInnerJsons(val);
            }
        }
    }
}

void SortArraysByKey(NJson::TJsonValue& value) {
    if (value.IsArray()) {
        bool haveKey = false;
        bool isStringKey = true;  // key is either string or number

        for (auto& v : value.GetArraySafe()) {
            if (v.Has("key")) {
                haveKey = true;
                if (!v["key"].IsString()) {
                    isStringKey = false;
                }
                break;
            }
        }

        if (haveKey) {
            if (isStringKey) {
                SortBy(value.GetArraySafe(), [](const NJson::TJsonValue& v) { return v["key"].GetString(); });
            } else {
                SortBy(value.GetArraySafe(), [](const NJson::TJsonValue& v) { return v["key"].GetInteger(); });
            }
        }

        for (auto& v : value.GetArraySafe()) {
            SortArraysByKey(v);
        }
    } else if (value.IsMap()) {
        for (auto& p : value.GetMapSafe()) {
            SortArraysByKey(p.second);
        }
    }
}

void WipeRTLogToken(NJson::TJsonValue& value) {
    if (value.IsArray()) {
        for (auto& v : value.GetArraySafe()) {
            WipeRTLogToken(v);
        }
    } else if (value.IsMap()) {
        if (value.Has("Name") && value.Has("Value") && value["Name"].IsString() && value["Name"].GetString() == "x-rtlog-token") {
            value.EraseValue("Value");
        }

        for (auto& p : value.GetMapSafe()) {
            WipeRTLogToken(p.second);
        }
    }
}

TString NormalizeMegamindHistoriy(const TString& histories) {
    TStringStream ss{histories};
    NJson::TJsonValue value = NJson::ReadJsonTree(&ss, /* throwOnError = */ true);

    TVector<NJson::TJsonValue> sortedValues;
    for (const auto& val : value.GetArray()) {
        sortedValues.emplace_back(val);
    }
    Sort(sortedValues.begin(), sortedValues.end(), [](const auto& l, const auto& r) {
        return NJson::WriteJson(l) < NJson::WriteJson(r);
    });
    for (auto& val : sortedValues) {
        WipeFields(val, {"headers", "x-ya-service-ticket"});
        WipeFields(val, {"headers", "x-request-id"});
        WipeFields(val, {"headers", "x-market-req-id"});
        WipeFields(val, {"query", "wizextra"});

        const auto& action = val["action"].GetString();
        if (action.Contains("localhost")) {
            auto copiedAction = val["action"].GetString();
            re2::RE2::GlobalReplace(&copiedAction, "localhost:(\\S+)", "localhost:XXXX");
            val["action"] = copiedAction;
        }

        const auto& body = val["body"].GetString();
        if (body.Contains("\"timestamp\"")) {
            auto copiedBody = val["body"].GetString();
            re2::RE2::GlobalReplace(&copiedBody, "\"timestamp\":(\\S+)", "\"timestamp\":XXXXXXX");
            val["body"] = copiedBody;
        }
    }

    NJson::TJsonValue newValue;
    for (auto&& val : sortedValues) {
        newValue.AppendValue(std::move(val));
    }

    return NJson::WriteJson(newValue, /* formatOutput = */ true, /* sortKeys = */ true);
}

TString NormalizeMegamindResponse(const TString& response) {
    TString fixedResponse = response;
    re2::RE2::GlobalReplace(&fixedResponse, "\"score\"\\s*:\\s*(\\d+)\\.(\\d{1,3})(\\d+)", "\"score\":\\1.\\2");
    re2::RE2::GlobalReplace(&fixedResponse, ">Доставка.*бесплатно<", ">Доставка XXX бесплатно<");

    TStringStream ss{fixedResponse};
    NJson::TJsonValue value = NJson::ReadJsonTree(&ss, /* throwOnError = */ true);

    WipeFields(value, {"header", "response_id"});
    WipeFields(value, {"megamind_analytics_info"});
    WipeFields(value, {"response", "card", "buttons", "directives", "payload", "button_id"});
    WipeFields(value, {"response", "cards", "buttons", "directives", "payload", "button_id"});
    WipeFields(value, {"response", "suggest", "items", "directives", "payload", "button_id"});
    WipeFields(value, {"sessions"});
    WipeFields(value, {"version"});
    UnpackInnerJsons(value);

    return NJson::WriteJson(value, /* formatOutput = */ true, /* sortKeys = */ true);
}

template <typename TProto>
TMaybe<NJson::TJsonValue> TryParse(const TStringBuf raw) {
    auto decompressed = NAppHost::NCompression::Decode(raw);
    TStringBuf view(decompressed);
    view.SkipPrefix("p_");

    TProto proto;
    if (!proto.ParseFromArray(view.data(), view.size())) {
        return Nothing();
    }

    TString data = NProtobufJson::Proto2Json(proto, NProtobufJson::TProto2JsonConfig().SetFormatOutput(true));
    TStringStream ss{data};
    NJson::TJsonReaderConfig config;
    config.DontValidateUtf8 = true;
    try {
        return NJson::ReadJsonTree(&ss, &config, /* throwOnError = */ true);
    } catch (...) {
        return Nothing();
    }
}

template <typename... Rest>
typename std::enable_if<(sizeof...(Rest) == 0), TMaybe<NJson::TJsonValue>>::type
ParseProto(const TStringBuf /*raw*/) {
    return Nothing();
}

template <typename TProto, typename... Rest>
TMaybe<NJson::TJsonValue> ParseProto(const TStringBuf raw) {
    auto proto = TryParse<TProto>(raw);
    if (proto.Defined()) {
        return proto;
    } else {
        return ParseProto<Rest...>(raw);
    }
}

TString NormalizeHollywoodResponse(const TString& response) {
    TStringStream ss{response};

    NJson::TJsonReaderConfig config;
    config.DontValidateUtf8 = true;
    NJson::TJsonValue value = NJson::ReadJsonTree(&ss, &config, /* throwOnError = */ true);

    static THashMap<TString, std::function<TMaybe<NJson::TJsonValue>(const TStringBuf)>> mapperTypes = {
        {"http_request", ParseProto<NAppHostHttp::THttpRequest>},
        {"hw_bass_request", ParseProto<NAppHostHttp::THttpRequest>},
        {"hw_reply_candidates_search_request", ParseProto<NAppHostHttp::THttpRequest>},
        {"hw_suggest_candidates_search_request", ParseProto<NAppHostHttp::THttpRequest>},

        {"hw_bass_request_rtlog_token", ParseProto<NHollywood::TBassRequestRTLogToken>},
        {"hw_reply_candidates_search_request_rtlog_token", ParseProto<NHollywood::TBassRequestRTLogToken>},
        {"hw_suggest_candidates_search_request_rtlog_token", ParseProto<NHollywood::TBassRequestRTLogToken>},
        {"request_rtlog_token", ParseProto<NHollywood::TBassRequestRTLogToken>},

        {"classification_result", ParseProto<NHollywood::NGeneralConversation::TClassificationResult>},
        {"reply_state", ParseProto<NHollywood::NGeneralConversation::TReplyState>},
        {"session_state", ParseProto<NHollywood::NGeneralConversation::TSessionState>},
        {"suggest_candidates_state", ParseProto<NHollywood::NGeneralConversation::TCandidatesState>},

        {"mm_scenario_request", ParseProto<NScenarios::TScenarioRunRequest>},
        {"mm_scenario_request_meta", ParseProto<NScenarios::TRequestMeta>},
        {"mm_scenario_response", ParseProto<NScenarios::TScenarioRunResponse>},
    };

    if (value.Has("Answers")) {
        NJson::TJsonValue& answers = value["Answers"];
        for (auto& val : answers.GetArraySafe()) {
            const TString& type = val["Type"].GetString();

            // erase http_response
            if (type == "http_response") {
                val.EraseValue("Data");
                continue;
            }

            const auto iter = mapperTypes.find(type);
            if (iter == mapperTypes.end()) {
                LOG(ERROR) << "No known mappings! For " << type.Quote() << Endl;
                continue;
            }

            const auto mapped = iter->second(val["Data"].GetString());
            if (mapped.Defined()) {
                val["Data"] = std::move(*mapped);
            } else {
                LOG(ERROR) << "Parsing error! Unknown type for " << type.Quote() << Endl;
            }
        }
    }

    WipeFields(value, {"ComprTime"});
    WipeFields(value, {"DecomprTime"});
    WipeFields(value, {"FinTime"});
    WipeFields(value, {"InitTime"});
    WipeFields(value, {"LogicTime"});
    WipeFields(value, {"RequestId"});
    WipeFields(value, {"UncompressedSize"});

    WipeFields(value, {"Answers", "Data", "RTLogToken"});
    WipeFields(value, {"Answers", "Data", "Version"});

    UnpackInnerJsons(value);
    SortArraysByKey(value);
    WipeRTLogToken(value);

    return NJson::WriteJson(value, /* formatOutput = */ true, /* sortKeys = */ true);
}

void SetZeroMilliseconds(NJson::TJsonValue& value) {
    if (value.IsArray()) {
        for (auto& v : value.GetArraySafe()) {
            SetZeroMilliseconds(v);
        }
    } else if (value.IsMap()) {
        for (auto& p : value.GetMapSafe()) {
            auto& val = p.second;
            if (val.IsInteger() || val.IsUInteger()) {
                if (p.first.Contains("milliseconds")) {
                    val = 0;
                }
            } else if (val.IsMap() || val.IsArray()) {
                SetZeroMilliseconds(val);
            }
        }
    }
}

void UnpackScenarioAnalyticsInfo(NJson::TJsonValue& value) {
    if (value.IsArray()) {
        for (auto& v : value.GetArraySafe()) {
            UnpackScenarioAnalyticsInfo(v);
        }
    } else if (value.IsMap()) {
        if (value.Has("type") && value["type"].GetString() == "scenario_analytics_info") {
            if (value.Has("data")) {
                value["data"] = "<deleted>";
            }
        }

        for (auto& p : value.GetMapSafe()) {
            auto& val = p.second;
            if (val.IsMap() || val.IsArray()) {
                UnpackScenarioAnalyticsInfo(val);
            }
        }
    }
}

TString NormalizeHollywoodBassResponse(const TString& response) {
    TString fixedResponse = response;
    TStringStream ss{fixedResponse};
    NJson::TJsonValue value = NJson::ReadJsonTree(&ss, /* throwOnError = */ true);

    UnpackInnerJsons(value, /*unpack = */ true);
    SetZeroMilliseconds(value);
    UnpackScenarioAnalyticsInfo(value);

    TString result = NJson::WriteJson(value, /* formatOutput = */ true, /* sortKeys = */ true);
    return result;
}

TString CalculateDiff(const TFsPath& oldPath, const TFsPath& newPath) {
    TShellCommand cmd("diff", {"-u", oldPath, newPath});
    cmd.Run().Wait();

    TStringBuilder sb;
    for (const auto c : cmd.GetOutput()) {
        if (c == '`') {
            sb << "\\`";
        } else {
            sb << c;
        }
    }
    return sb;
}

} // namespace

TDiff2Html::TDiff2Html(int threads, int diffsPerFile, const TString& mode, const TFsPath& oldResponsesPath, const TFsPath& newResponsesPath,
                       const TFsPath& outputPath, const TMaybe<TFsPath>& statsPath)
    : ThreadCount_{threads}
    , DiffsPerFile_{diffsPerFile}
    , Mode_{mode}
    , OldResponsesPath_{oldResponsesPath}
    , NewResponsesPath_{newResponsesPath}
    , OutputPath_{outputPath}
    , StatsPath_{statsPath}
    , ResponsesDiffsSummary_{0}
{
}

void TDiff2Html::ConstructDiff() {
    LOG(INFO) << "Constructing diff" << Endl;
    LOG(INFO) << "Old responses path " << OldResponsesPath_ << Endl;
    LOG(INFO) << "New responses path " << NewResponsesPath_ << Endl;

    OutputPath_.MkDirs();
    auto oldRequestIds = ObtainRequestIds(OldResponsesPath_);
    auto newRequestIds = ObtainRequestIds(NewResponsesPath_);

    auto [uniqueOldErrors, uniqueNewErrors] = ObtainUniqueErrorsCount(oldRequestIds, newRequestIds);
    LOG(INFO) << "Unique old errors: " << uniqueOldErrors << Endl;
    LOG(INFO) << "Unique new errors: " << uniqueNewErrors << Endl;

    TVector<std::tuple<TString, TString, NormalizerFunc>> tasks;
    if (Mode_ == "megamind") {
        // diff RESPONSES and HISTORIES
        tasks = {
            {"responses", "response.json", NormalizeMegamindResponse},
            {"histories", "histories.json", NormalizeMegamindHistoriy}
        };
    } else if (Mode_ == "hollywood") {
        // diff RESPONSES
        tasks = {
            {"responses", "response.json", NormalizeHollywoodResponse}
        };
    } else if (Mode_ == "hollywood_bass") {
        // diff RESPONSES
        tasks = {
            {"responses", "response.json", NormalizeHollywoodBassResponse}
        };
    }

    for (const auto& t : tasks) {
        LOG(INFO) << "Work with files " << std::get<1>(t) << Endl;
        TFsPath{OutputPath_ / std::get<0>(t)}.MkDirs();
        DiffsCount_ = 0;
        WrittenHtmlFiles_= 0;

        THolder<IThreadPool> threadPool = CreateThreadPool(ThreadCount_);
        for (const auto& reqId : oldRequestIds) {
            if (newRequestIds.contains(reqId)) {
                threadPool->SafeAddFunc([&]() {
                    WorkWithFile(reqId, std::get<0>(t), std::get<1>(t), std::get<2>(t));
                });
            }
        }

        threadPool->Stop();
        if (!DiffInfos_.empty()) {
            FlushDiffs(std::get<0>(t));
        }
    }

    if (Mode_ == "megamind") {
        // diff SENSORS and INTENTS
        for (const auto& t : {"sensors", "intents"}) {
            TFsPath{OutputPath_ / t}.MkDirs();

            DiffsCount_ = 0;
            WrittenHtmlFiles_= 0;
            WorkWithFile(/* reqId = */ "", /* outputFolder = */ t, TString::Join(t, ".json"), [](const TString& s) {
                return s;
            });
            FlushDiffs(/* folder = */ t);
        }
    }

    // Write stats data
    if (StatsPath_.Defined()) {
        NJson::TJsonValue value;
        value["responses_diffs_count"] = ResponsesDiffsSummary_;

        TFileOutput fout{StatsPath_.GetRef()};
        NJson::WriteJson(&fout, &value, /* formatOutput = */ true, /* sortKeys = */ true);
    }
}

void TDiff2Html::WorkWithFile(const TString& reqId, const TString& outputFolder, const TString& fileName, NormalizerFunc normalizer) {
    LOG(INFO) << "Working with id " << reqId.Quote() << Endl;
    TFsPath oldPath{OldResponsesPath_ / reqId / fileName};
    TFsPath newPath{NewResponsesPath_ / reqId / fileName};

    if (!oldPath.Exists() || !newPath.Exists()) {
        return;
    }

    try {
        TString oldResponse = normalizer(TFileInput{oldPath}.ReadAll());
        TString newResponse = normalizer(TFileInput{newPath}.ReadAll());

        if (oldResponse != newResponse) {
            auto g = Guard(Lock_);

            if (outputFolder == "responses") {
                ++ResponsesDiffsSummary_;
            }
            DiffInfos_.emplace_back(TDiffInfo{reqId, oldResponse, newResponse});
            LOG(INFO) << "Found " << ++DiffsCount_ << " diffs, new diff at " << reqId << "/" << fileName << Endl;

            if (static_cast<int>(DiffInfos_.size()) >= DiffsPerFile_) {
                FlushDiffs(outputFolder);
            }
        }
    } catch(...) {
        LOG(INFO) << "Exception on " << fileName << Endl;
    }
}

void TDiff2Html::FlushDiffs(const TString& folder) {
    TFsPath htmlPath{OutputPath_ / folder / TString::Join(ToString(++WrittenHtmlFiles_), ".html")};
    LOG(INFO) << "Flush diffs to " << htmlPath << Endl;

    TFileOutput fout{htmlPath};
    fout << HTML_HEADER << Endl;
    fout << HTML_BODY_BEGINNING << Endl;
    for (const auto& p : DiffInfos_) {
        TTempFile tmpOld{TString::Join("[old]", p.ReqId)};
        TFileOutput{tmpOld.Name()}.Write(p.OldResponse);

        TTempFile tmpNew{TString::Join("[new]", p.ReqId)};
        TFileOutput{tmpNew.Name()}.Write(p.NewResponse);

        fout << CalculateDiff(tmpOld.Name(), tmpNew.Name()) << Endl;
    }
    fout << HTML_BODY_ENDING << Endl;
    DiffInfos_.clear();
}

} // namespace NAlice::NShooter
