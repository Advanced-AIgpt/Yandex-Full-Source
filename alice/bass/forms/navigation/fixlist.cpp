#include "fixlist.h"

#include <alice/bass/libs/logging_v2/bass_logadapter.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <contrib/libs/pire/pire/pire.h>

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/resource/resource.h>

#include <util/charset/utf8.h>
#include <util/string/cast.h>
#include <util/string/strip.h>


namespace NBASS {

namespace {
constexpr std::array<TStringBuf, 3> FIXLIST_FILENAMES{
    TStringBuf("navigation_fixlist_general.json"),
    TStringBuf("navigation_fixlist_yandex.json"),
    TStringBuf("navigation_fixlist_turbo.json"),
};

constexpr TStringBuf NATIVEAPPS_LIST = "navigation_nativeapps.json";

TString LoadNativeAppsList() {
    TString nativeApps;
    if (!NResource::FindExact(NATIVEAPPS_LIST, &nativeApps)) {
        LOG(ERR) << "Fixlist file is missing: " << NATIVEAPPS_LIST << Endl;
    }
    return nativeApps;
}

TVector<TString> LoadBundledFixlists() {
    TVector<TString> contents;
    contents.resize(FIXLIST_FILENAMES.size());
    for (size_t i = 0; i < FIXLIST_FILENAMES.size(); ++i) {
        if (!NResource::FindExact(FIXLIST_FILENAMES[i], &contents[i])) {
            LOG(ERR) << "Fixlist file is missing: " << FIXLIST_FILENAMES[i] << Endl;
            continue;
        }
    }
    return contents;
}

} // namespace

///// Navigation fixlist ///////////////////////////////////////////////////////////////////////////////////////////////
TNavigationFixList::TNavigationFixList()
    : Fixlist(TString{}, LoadNativeAppsList(), LoadBundledFixlists(), TBassLogAdapter{})
{}

TNavigationFixList::~TNavigationFixList() {}

void TNavigationFixList::ReloadYaStrokaFixList() {
    Fixlist.ReloadYaStrokaFixList(TBassLogAdapter{}, TString{});
}

NSc::TValue TNavigationFixList::Find(const TString& query, const TContext&) const {
    return Fixlist.Find(query, TBassLogAdapter{});
}

TMaybe<NAlice::TNavigationFixList::TWindowsApp> TNavigationFixList::FindWindowsApp(const TString& query) const {
    return Fixlist.FindWindowsApp(query);
}

}
