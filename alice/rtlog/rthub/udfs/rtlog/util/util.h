#pragma once

#include <array>
#include <util/generic/algorithm.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/string/cast.h>

namespace NAlice {

    constexpr TStringBuf PRODUCTION = "production";
    constexpr TStringBuf HAMSTER = "testing";

    constexpr std::array<TStringBuf, 9> PRODUCTION_NANNY_SERVICES = { // [gmikee] if it becomes large: Rewrite using THashSet
        "megamind_sas", "megamind_standalone_sas", "hollywood_sas",
        "megamind_vla", "megamind_standalone_vla", "hollywood_vla",
        "megamind_man", "megamind_standalone_man", "hollywood_man"
    };

    constexpr std::array<TStringBuf, 3> HAMSTER_NANNY_SERVICES = { // [gmikee] if it becomes large: Rewrite using THashSet
        "megamind_hamster", "megamind_standalone_hamster", "hollywood_hamster"
    };

    bool NeedToSend(const TString& message, const TString& backtrace);
    TMaybe<TString> GetEnv(const TString& nannyServiceId);
    TString DataCenterFromHost(TStringBuf host);
    TString FileFromPos(TStringBuf pos);
    TString GetLanguage(const TString& backtrace);
    ui64 LineFromPos(TStringBuf pos);

} // namespace NAlice
