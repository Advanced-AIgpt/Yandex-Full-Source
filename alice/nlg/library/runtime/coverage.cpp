#include "coverage.h"

#include <library/cpp/json/json_writer.h>
#include <util/stream/file.h>
#include <util/string/cast.h>

namespace {

constexpr TStringBuf PID_SPECIFIER = "%p";

static TString FormatPid(TString filenameWithPidSpecifier, TProcessId pid) {
    Y_ASSERT(!!filenameWithPidSpecifier);

    const auto pidSpecifierPos = filenameWithPidSpecifier.find(PID_SPECIFIER);
    Y_ASSERT(pidSpecifierPos != TString::npos);

    return filenameWithPidSpecifier.replace(pidSpecifierPos, PID_SPECIFIER.Size(), ToString(pid));
}

} // namespace

namespace NAlice::NNlg {

TCoverage::TCoverage(const TString& nlgCoverageFilename, TProcessId pid)
    : NlgCoverageFilename(FormatPid(nlgCoverageFilename, pid)) {
}

void TCoverage::RegisterModule(const TStringBuf nlgFilename, const TVector<TSegment>& segments) {
    with_lock(Lock) {
        if (Modules.contains(nlgFilename)) {
            return;
        }

        TModule module{ToString(nlgFilename), {}};
        for (const auto& segment : segments) {
            for (ui64 lineIndex = segment.StartLineIndex; lineIndex < segment.EndLineIndex; ++lineIndex) {
                module.Segments[lineIndex] = 0;
            }
        }
        Modules[nlgFilename] = module;
    }
}

void TCoverage::IncCounter(const TStringBuf moduleId, ui64 lineIndex) {
    with_lock(Lock) {
        auto& module = GetModule(moduleId);
        try {
            ++module.Segments.at(lineIndex);
        } catch (std::out_of_range e) {
            throw yexception() << "Segment for " << moduleId << ":" << lineIndex << " is not registered";
        }
    }
}

TCoverage::TModule& TCoverage::GetModule(const TStringBuf moduleId) {
    try {
        return Modules.at(moduleId);
    } catch (std::out_of_range e) {
        throw yexception() << "Module " << moduleId << " is not registered";
    }
}

TVector<NJson::TJsonValue> TCoverage::ToJsonValues() const {
    TVector<NJson::TJsonValue> result;
    for (const auto& module : Modules) {
        result.push_back(module.second.ToJsonValue());
    }
    return result;
}

NJson::TJsonValue TCoverage::TModule::ToJsonValue() const {
    NJson::TJsonValue result;
    NJson::TJsonValue segments = NJson::TJsonValue{NJson::JSON_ARRAY};
    for (const auto& s : Segments) {
        auto lineIndex = s.first;
        auto counter = s.second;
        NJson::TJsonValue segment = NJson::TJsonValue{NJson::JSON_ARRAY};
        segment.AppendValue(lineIndex);
        segment.AppendValue(0);
        segment.AppendValue(lineIndex + 1);
        segment.AppendValue(0);
        segment.AppendValue(counter);
        segments.AppendValue(segment);
    }
    result["coverage"] = NJson::TJsonValue{};
    result["coverage"]["segments"] = segments;
    result["filename"] = NlgFilename;
    return result;
}

void TCoverage::WriteNlgCoverageFile() {
    TFileOutput out(NlgCoverageFilename);
    const auto jsonValues = ToJsonValues();
    for (const auto& jsonValue : jsonValues) {
        NJson::WriteJson(&out, &jsonValue);
        out << Endl;
    }
}

TCoverageWrapper::~TCoverageWrapper() {
    Coverage.WriteNlgCoverageFile();
}

} // namespace NAlice::NNlg
