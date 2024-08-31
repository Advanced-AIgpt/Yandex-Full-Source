#include "util.h"

#include <contrib/libs/re2/re2/re2.h>

namespace NAlice {

    namespace {
        constexpr TStringBuf YEXCEPTION = "yexception";
        constexpr TStringBuf SCENARIO = "Scenario";
        constexpr TStringBuf ERROR = "error:";
        constexpr TStringBuf YP_SUFFIX = ".yp-c.yandex.net";


        const re2::RE2 PYTHON_LANGUAGE_PATTERN{R"(\.(?:py|pyc)\b)"};
        const re2::RE2 JAVA_LANGUAGE_PATTERN{R"(\.(?:java|kt)\b)"};
        const re2::RE2 GO_LANGUAGE_PATTERN{R"(\.go\b)"};
        const re2::RE2 CPP_LANGUAGE_PATTERN{R"(\.(?:c|cc|h|hpp|cpp|cxx)\b)"};
    }

    bool NeedToSend(const TString& message, const TString& backtrace) {
        if (!backtrace.empty() || message.Contains(YEXCEPTION)) {
            return true;
        }
        return message.StartsWith(SCENARIO) && message.Contains(ERROR);
    }

    TMaybe<TString> GetEnv(const TString& nannyServiceId) {
        if (FindPtr(PRODUCTION_NANNY_SERVICES, nannyServiceId)) {
            return TString(PRODUCTION);
        }
        if (FindPtr(HAMSTER_NANNY_SERVICES, nannyServiceId)) {
            return TString(HAMSTER);
        }
        return Nothing();
    }

    TString DataCenterFromHost(TStringBuf host) {
        if (host.ChopSuffix(YP_SUFFIX)) {
            return TString(host.Last(3));
        }
        return TString(host.Trunc(3));
    }

    TString FileFromPos(TStringBuf pos) {
        return TString(pos.Before(':'));
    }

    ui64 LineFromPos(TStringBuf pos) {
        return FromString<ui64>(pos.After(':'));
    }

    TString GetLanguage(const TString& backtrace) {
        if (re2::RE2::PartialMatch(backtrace, PYTHON_LANGUAGE_PATTERN)) {
            return TString{"python"};
        }
        if (re2::RE2::PartialMatch(backtrace, GO_LANGUAGE_PATTERN)) {
            return TString{"go"};
        }
        if (re2::RE2::PartialMatch(backtrace, CPP_LANGUAGE_PATTERN)) {
            return TString{"c++"};
        }
        if (re2::RE2::PartialMatch(backtrace, JAVA_LANGUAGE_PATTERN)) {
            return TString{"java"};
        }
        return {};
    }

} // namespace NAlice
