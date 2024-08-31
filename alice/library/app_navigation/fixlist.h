#pragma once

#include <alice/library/client/client_info.h>
#include <alice/library/logger/logadapter.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/threading/hot_swap/hot_swap.h>

#include <util/folder/path.h>


namespace NAlice {

/**
 * fixlist для навигационников.
 */
class TNavigationFixList {
public:
    TNavigationFixList(const TFsPath& dirPath, const TLogAdapter& logAdapter);
    TNavigationFixList(const TFsPath& dirPath, const TString& nativeApps,
                       const TVector<TString>& bundledFixlists,
                       const TLogAdapter& logAdapter);
    ~TNavigationFixList();

    /// Находит и возвращает соответствующий запросу блок из фикслиста.
    NSc::TValue Find(const TString& query, const TLogAdapter& logAdapter) const;

    struct TWindowsApp {
        TString NormName = "";
        TString Title = "";
        TString Url = "";
        bool Native = false;

        void Reset() {
            NormName = "";
            Title = "";
            Url = "";
            Native = false;
        }

        TString ToString() const {
            return TStringBuilder() << "{'" << NormName << "', '" << Title << "', '" << Url << "', " << Native << "}";
        }

        Y_SAVELOAD_DEFINE(NormName, Title, Url, Native);
    };

    TMaybe<TWindowsApp> FindWindowsApp(const TString& query) const;

    struct TNativeApp {
        TString AndroidUri;
        TString AndroidTitle;
        TString IosUri;
        TString IosTitle;
        TString YaAutoUri;
        TString YaAutoTitle;

        bool Empty(const TClientInfo& clientInfo) const {
            if (clientInfo.IsYaAuto())
                return YaAutoTitle.empty() || YaAutoUri.empty();
            if (clientInfo.IsAndroid())
                return AndroidTitle.empty() || AndroidUri.empty();
            if (clientInfo.IsIOS())
                return IosTitle.empty() || IosUri.empty();
            return true;
        }
    };
    TMaybe<TNativeApp> FindNativeApp(const TString& code) const {
        if (const auto* app = NativeApps.FindPtr(code)) {
            return *app; // yes, we do copy here
        }
        return Nothing();
    }

    void ReloadYaStrokaFixList(const TLogAdapter& logAdapter, const TFsPath& dirPath);
    class TWinFixList;

private:
    void LoadBundledFixList(const TLogAdapter& logAdapter, const TFsPath& dirPath);
    void LoadBundledFixListFromContents(const TLogAdapter& logAdapter, const TVector<TString>& contents);
    void LoadBundledNativeAppsList(const TLogAdapter& logAdapter, const TFsPath& dirPath);
    void LoadBundledNativeAppsListFromContent(const TLogAdapter& logAdapter, const TString& content);

private:
    struct TFixList;
    THolder<const TFixList> FixList;

    THotSwap<TWinFixList> WinFixList;

    using TNativeApps = THashMap<TString, TNativeApp>;
    TNativeApps NativeApps;
};

}  // namespace NAlice
