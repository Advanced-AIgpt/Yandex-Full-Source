#pragma once

#include <library/cpp/json/writer/json_value.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/singleton.h>
#include <util/system/env.h>
#include <util/system/getpid.h>
#include <util/system/spinlock.h>

namespace NAlice::NNlg {

struct TSegment;

class TCoverage final {
public:
    TCoverage(const TString& nlgCoverageFilename, TProcessId pid);

    void RegisterModule(const TStringBuf moduleId, const TVector<TSegment>& segments);
    void IncCounter(const TStringBuf moduleId, ui64 lineIndex);
    TVector<NJson::TJsonValue> ToJsonValues() const;
    TString GetNlgCoverageFilename() const {
        return NlgCoverageFilename;
    }
    void WriteNlgCoverageFile();

private:
    struct TModule final {
        TString NlgFilename;       // relative to arcadia's root
        TMap<ui64, ui64> Segments; // LineIndex -> Counter

        NJson::TJsonValue ToJsonValue() const;
    };

    TModule& GetModule(const TStringBuf moduleId);

    THashMap<TString, TModule> Modules; // ModuleId -> TModule
    TAdaptiveLock Lock;
    TString NlgCoverageFilename;
};

struct TSegment final {
    ui64 StartLineIndex;
    ui64 EndLineIndex; // exclusive
};

struct TCoverageWrapper final {
    TCoverage Coverage;
    TCoverageWrapper()
        : Coverage(GetEnv("NLG_COVERAGE_FILENAME"), GetPID()) {
    }
    ~TCoverageWrapper();
};

inline TCoverage& GetNlgCoverage() {
    return Singleton<NAlice::NNlg::TCoverageWrapper>()->Coverage;
}

} // namespace NAlice::NNlg
