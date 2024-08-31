#include "fixlist.h"

#include <contrib/libs/pire/pire/pire.h>

#include <library/cpp/containers/comptrie/comptrie.h>

#include <util/charset/utf8.h>
#include <util/string/cast.h>
#include <util/string/strip.h>
#include <util/system/fs.h>

namespace NAlice {

///// Navigation fixlist ///////////////////////////////////////////////////////////////////////////////////////////////
namespace {
constexpr TStringBuf FIXLIST_FILENAMES[] = {
    TStringBuf("navigation_fixlist_general.json"),
    TStringBuf("navigation_fixlist_yandex.json"),
    TStringBuf("navigation_fixlist_turbo.json"),
};
constexpr TStringBuf WIN_FIXLIST_DATA_FILE = "data/windows_fixlist.data";
constexpr TStringBuf WIN_FIXLIST_OFFSETS_FILE = "data/windows_fixlist.offsets";
constexpr TStringBuf NATIVEAPPS_LIST = "navigation_nativeapps.json";

struct TPireFixedAnswer {
    TString Regex;
    NSc::TValue Data;
};

} // namespace

struct TNavigationFixList::TFixList {
    TVector<TPireFixedAnswer> PireAnswers;
    THolder<Pire::NonrelocScanner> PireScanner;
    THashMap<TString, NSc::TValue> Data; // query -> answer
};

class TNavigationFixList::TWinFixList : public TAtomicRefCount<TNavigationFixList::TWinFixList> {
public:
    TWinFixList(const TFsPath& dirPath)
            : Offsets(TBlob::FromFileContent(dirPath / WIN_FIXLIST_OFFSETS_FILE))
            , Data(TBlob::FromFileContent(dirPath / WIN_FIXLIST_DATA_FILE)) {
    }
    bool Find(const TString& query, TNavigationFixList::TWindowsApp* app) const {
        app->Reset();
        ui64 offset = 0;
        if (Offsets.Find(query, &offset)) {
            TMemoryInput in{Data.AsCharPtr() + offset, Data.Length()};
            app->Load(&in);
            return true;
        }
        return false;
    }
    size_t Size() const {
        return Offsets.Size();
    }
private:
    const TCompactTrie<char, ui64> Offsets;
    const TBlob Data;
};

TNavigationFixList::TNavigationFixList(const TFsPath& dirPath, const TLogAdapter& logAdapter) {
    LoadBundledFixList(logAdapter, dirPath);
    LoadBundledNativeAppsList(logAdapter, dirPath);
    ReloadYaStrokaFixList(logAdapter, dirPath);
}

TNavigationFixList::TNavigationFixList(const TFsPath& dirPath, const TString& nativeApps,
                                       const TVector<TString>& bundledFixlists,
                                       const TLogAdapter& logAdapter) {
    LoadBundledFixListFromContents(logAdapter, bundledFixlists);
    LoadBundledNativeAppsListFromContent(logAdapter, nativeApps);
    ReloadYaStrokaFixList(logAdapter, dirPath);
}

TNavigationFixList::~TNavigationFixList() {}

void TNavigationFixList::LoadBundledFixList(const TLogAdapter& logAdapter, const TFsPath& dirPath) {
    TVector<TString> contents;
    for (TStringBuf fixlist : FIXLIST_FILENAMES) {
        const auto path = dirPath / fixlist;
        if (!NFs::Exists(path)) {
            LOG_ADAPTER_ERROR(logAdapter) << "Fixlist file is missing: " << fixlist << Endl;
            continue;
        }

        LOG_ADAPTER_INFO(logAdapter) << "Loading " << fixlist << Endl;
        TFileInput inputStream(path);
        contents.push_back(inputStream.ReadAll());
    }
    LoadBundledFixListFromContents(logAdapter, contents);
}

void TNavigationFixList::LoadBundledFixListFromContents(const TLogAdapter& logAdapter,
                                                        const TVector<TString>& contents)
{
    TFixList result{};
    THolder<Pire::NonrelocScanner> scanner = MakeHolder<Pire::NonrelocScanner>();
    size_t count = 0;

    for (const auto& content : contents) {
        NSc::TValue json = NSc::TValue::FromJson(TStringBuf(content));
        if (json.IsNull()) {
            LOG_ADAPTER_ERROR(logAdapter) << "Content of fixlist is empty" << Endl;
            continue;
        }

        for (const NSc::TValue& queryData : json.GetArray()) {
            const NSc::TValue& data = queryData["data"];
            TStringBuilder regex;
            for (const NSc::TValue& re : queryData["regex"].GetArray()) {
                if (!regex.empty())
                    regex << "|";
                regex << "(" << re.GetString() << ")";
            }
            if (!regex.empty()) {
                TVector<wchar32> ucs4;
                Pire::Encodings::Utf8().FromLocal(regex.begin(), regex.end(), std::back_inserter(ucs4));
                TPireFixedAnswer pireFixedAnswer = {
                        .Regex = regex,
                        .Data = data
                };
                *scanner = Pire::Scanner::Glue(
                        *scanner,
                        Pire::Lexer(ucs4.begin(), ucs4.end())
                                .SetEncoding(Pire::Encodings::Utf8())
                                .Parse()
                                .Compile<Pire::NonrelocScanner>()
                );
                result.PireAnswers.emplace_back(std::move(pireFixedAnswer));
                LOG_ADAPTER_DEBUG(logAdapter) << "Navigation.Fixlist: ~ '" << regex << "'" << Endl;
            }
            for (const NSc::TValue& queryNode : queryData["queries"].GetArray()) {
                if (result.Data.contains(queryNode.GetString())) {
                    LOG_ADAPTER_ERROR(logAdapter) << "Dublicated query '" << queryNode.GetString() << "'" << Endl;
                    continue;
                }
                result.Data[queryNode.GetString()] = data;
            }
            ++count;
        }

        LOG_ADAPTER_INFO(logAdapter) << "Fixlist loaded" << Endl;
    }

    if (!scanner->Empty()) {
        result.PireScanner.Swap(scanner);
        LOG_ADAPTER_INFO(logAdapter) << "Pire FSM size = " << result.PireScanner->Size() << Endl;
        LOG_ADAPTER_INFO(logAdapter) << "Queries table size = " << result.Data.size() << Endl;
        LOG_ADAPTER_INFO(logAdapter) << "Total answers: " << count << Endl;
    } else {
        LOG_ADAPTER_ERROR(logAdapter) << "PireScanner is empty or too big" << Endl;
    }
    FixList = MakeHolder<TFixList>(std::move(result));
}

void TNavigationFixList::ReloadYaStrokaFixList(const TLogAdapter& logAdapter, const TFsPath& dirPath) {
    try {
        LOG_ADAPTER_INFO(logAdapter) << "Loading windows fixlist" << Endl;
        std::unique_ptr<TWinFixList> fixList = std::make_unique<TWinFixList>(dirPath);
        LOG_ADAPTER_INFO(logAdapter) << "Windows fixlist loaded: " << fixList->Size() << " elements" << Endl;
        WinFixList.AtomicStore(fixList.release());
    } catch (const yexception& e) {
        LOG_ADAPTER_ERROR(logAdapter) << "Unable to load windows fixlist: " << e.what();
    } catch (...) {
        LOG_ADAPTER_ERROR(logAdapter) << "Unable to load windows fixlist: " << CurrentExceptionMessage();
    }
}

void TNavigationFixList::LoadBundledNativeAppsList(const TLogAdapter& logAdapter, const TFsPath& dirPath) {
    const auto path = dirPath / NATIVEAPPS_LIST;
    if (!NFs::Exists(path)) {
        LOG_ADAPTER_ERROR(logAdapter) << "Native apps list is missing" << Endl;
        return;
    }

    LOG_ADAPTER_INFO(logAdapter) << "Loading " << NATIVEAPPS_LIST << Endl;
    TFileInput inputStream(path);
    TString content = inputStream.ReadAll();

    LoadBundledNativeAppsListFromContent(logAdapter, content);
}

void TNavigationFixList::LoadBundledNativeAppsListFromContent(const TLogAdapter& logAdapter, const TString& content) {
    NSc::TValue json = NSc::TValue::FromJson(TStringBuf(content));
    if (json.IsNull()) {
        LOG_ADAPTER_ERROR(logAdapter) << NATIVEAPPS_LIST << " is empty" << Endl;
        return;
    }

    for (const NSc::TValue& entry : json.GetArray()) {
        TNativeApp app;
        TString code = ToString(entry["code"].GetString());
        if (code.empty()) {
            LOG_ADAPTER_ERROR(logAdapter) << "Invalid entry: " << entry << Endl;
            return;
        }
        if (TStringBuf title = entry["title"].GetString()) {
            app.AndroidTitle = title;
            app.IosTitle     = title;
            app.YaAutoTitle  = title;
        }
        app.YaAutoUri    = entry.TrySelect("yaauto/uri").GetString();
        app.YaAutoTitle  = entry.TrySelect("yaauto/title").GetString(app.YaAutoTitle);
        app.AndroidUri   = entry.TrySelect("android/uri").GetString();
        app.AndroidTitle = entry.TrySelect("android/title").GetString(app.AndroidTitle);
        app.IosUri       = entry.TrySelect("ios/uri").GetString();
        app.IosTitle     = entry.TrySelect("ios/title").GetString(app.IosTitle);
        NativeApps.emplace(code, std::move(app));
    }

    LOG_ADAPTER_INFO(logAdapter) << NATIVEAPPS_LIST << " loaded " << NativeApps.size() << " apps" << Endl;
}

NSc::TValue TNavigationFixList::Find(const TString& query, const TLogAdapter& logAdapter) const {
    TString normQuery = ToLowerUTF8(StripString(query));

    if (!FixList) {
        return NSc::Null();
    }
    // first try Pire
    if (FixList->PireScanner) {
        if (auto match = Pire::Runner(*FixList->PireScanner).Begin().Run(normQuery.data(), normQuery.size()).End()) {
            std::pair<const size_t*, const size_t*> indexes = FixList->PireScanner->AcceptedRegexps(match.State());
            if (indexes.second - indexes.first > 1)
                LOG_ADAPTER_WARNING(logAdapter) << "Multiple regex match" << Endl;

            for (const size_t* index = indexes.first; index != indexes.second; ++index) {
                const TPireFixedAnswer& answer = FixList->PireAnswers[*index];
                LOG_ADAPTER_DEBUG(logAdapter) << "Fixlist match: " << normQuery << " ~= " << answer.Regex << Endl;
                return answer.Data;
            }
        } else {
            LOG_ADAPTER_DEBUG(logAdapter) << "Fixlist (pire): match nothing" << Endl;
        }
    }

    // fallback to queries map
    auto it = FixList->Data.find(normQuery);
    if (it == FixList->Data.end()) {
        LOG_ADAPTER_DEBUG(logAdapter) << "Fixlist: match nothing" << Endl;
        return NSc::Null();
    }

    return it->second;
}

TMaybe<TNavigationFixList::TWindowsApp> TNavigationFixList::FindWindowsApp(const TString& query) const {
    TWindowsApp result;
    auto ptr = WinFixList.AtomicLoad();
    if (ptr && ptr->Find(query, &result)) {
        return MakeMaybe(std::move(result));
    }
    return Nothing();
}

} // namespace NAlice
