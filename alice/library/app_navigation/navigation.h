#pragma once

#include <alice/library/client/client_features.h>
#include <alice/hollywood/library/request/request.h>

#include <util/string/builder.h>

namespace NAlice {

namespace {

const TString NAV = "nav";

} // namespace

template <typename T>
TString CreateGooglePlayAppUrl(const T& appId) {
    return TStringBuilder() << TStringBuf("https://play.google.com/store/apps/details?id=") << appId;
}

template <typename T>
TString CreateITunesAppUrl(const T& appId) {
    return TStringBuilder() << TStringBuf("https://itunes.apple.com/app/rider/") << appId;
}

NSc::TValue CreateNavBlock(const NSc::TValue& data, const TClientFeatures& client, bool preferApp, const TString& navBlockName = NAV);
NSc::TValue CreateNavBlock(const NSc::TValue& data, const NHollywood::TScenarioBaseRequestWrapper& request, bool preferApp, const TString& navBlockName = NAV);

NSc::TValue CreateNavBlockImpl(const NSc::TValue& data, const TClientFeatures& client, const TString& navBlockName = NAV);
NSc::TValue CreateNavBlockImpl(const NSc::TValue& data, const NHollywood::TScenarioBaseRequestWrapper& request, const TString& navBlockName = NAV);

NSc::TValue CreateNavigationBlock(TStringBuf text, TStringBuf tts, TStringBuf app, TStringBuf url, const TClientFeatures& client);
NSc::TValue CreateNavigationBlock(TStringBuf text, TStringBuf tts, TStringBuf app, TStringBuf url, const NScenarios::TInterfaces& interfaces, const TClientInfo& clientInfo);

} //namespace NAlice
