#include "session.h"
#include "config.h"
#include "globalctx.h"
#include "request.h"

#include <alice/joker/library/log/log.h>

#include <library/cpp/protobuf/util/pb_io.h>

#include <util/datetime/base.h>
#include <util/generic/guid.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>
#include <util/string/join.h>

namespace NAlice::NJoker {
namespace {

constexpr TStringBuf SESSION_FILENAME = "meta.proto";
    /*
constexpr TStringBuf SESSION_LOG_FILENAME = "run_log.txt";
*/
const TString DIR_FOR_NEW_STUBS{"new"};

} // namespace

// TSessionId -----------------------------------------------------------------
TSessionId::TSessionId()
    : Id_{CreateGuidAsString()}
{
}

TSessionId::TSessionId(const TString& id)
    : Id_{id.Empty() ? CreateGuidAsString() : id}
{
}

TString TSessionId::DirName() const {
    return TStringBuilder{} << Id_; // FIXME (petrk)
}

// TSession -------------------------------------------------------------------
TSession::TSession(TSessionId id, const TConfig& config)
    : TSession{id, config, CreateGuidAsString(), TInstant::Now()}
{
}

TSession::TSession(TSessionId id, const TConfig& config, const TString& runId, TInstant startedAt)
    : Id_{std::move(id)}
    , RunId_{runId}
    , StartedAt_{std::move(startedAt)}
    , Directory_{SessionDirectory(Id_, config)}
    , Backend_{config.CreateBackend()}
{
}

const TFsPath& TSession::Directory() const {
    return Directory_;
}

/*
TFsPath TSession::SessionFile() const {
    return Directory_ / SESSION_FILENAME;
}

TFsPath TSession::SessionLogFile() const {
    return Directory_ / SESSION_LOG_FILENAME;
}
*/

TFsPath TSession::CurrentRunLogDirectory() const {
    return Directory() / TStringBuf("runlog") / (TStringBuilder{} << StartedAt_.ToString() << '@' << RunId_);
}

TFsPath TSession::StubsDirectory() const {
    return Directory() / TStringBuf("stubs");
}

TStatus TSession::WriteInfoFile() const {
    const TFsPath runLogDir = CurrentRunLogDirectory();
    if (Y_UNLIKELY(runLogDir.Exists())) {
        return TError{TError::EType::Logic} << "Session run log directory '" << runLogDir << "' has already existed.";
    }
    runLogDir.MkDirs();
    TFileOutput out{runLogDir / "info.txt"};

    NJokerProto::TSessionInfo sessionInfo;
    sessionInfo.SetId(Id_.Value());
    sessionInfo.SetRunAtMs(StartedAt_.MilliSeconds());
    sessionInfo.SetRunId(RunId_);
    SerializeToTextFormat(sessionInfo, out);

    return Success();
}

// static
TFsPath TSession::SessionDirectory(const TSessionId& id, const TConfig& config) {
    return config.SessionsPath() / id.DirName();
}

// static
TFsPath TSession::RunLogDirectory(const TSessionId& id, const TConfig& config) {
    return SessionDirectory(id, config) / TStringBuf("runlog");
}

// static
TMaybe<TFsPath> TSession::LatestRunLogDirectory(const TSessionId& id, const TConfig& config) {
    TFsPath runLogDir = RunLogDirectory(id, config);
    TVector<TString> runs;
    runLogDir.ListNames(runs);

    if (runs.empty()) {
        return Nothing();
    }
    auto sortCb = [](const TString& lhs, const TString& rhs) {
        return TStringBuf{lhs}.Before('@') < TStringBuf{rhs}.Before('@');
    };
    Sort(runs.begin(), runs.end(), sortCb);
    return runLogDir / runs.back();
}

// static
TFsPath TSession::SessionFile(const TSessionId& id, const TConfig& config) {
    return SessionDirectory(id, config) / SESSION_FILENAME;
}

TStatus TSession::LoadStub(const TStubId& id, const TString* version, TStubItemPtr& stubItem) const {
    const TFsPath fsPath = StubFileName(id, version);
    try {
        TFileInput in{fsPath};
        return TStubItem::Load(id, in, stubItem);
    } catch (...) {
        return TError{TError::EType::Logic} << "Unable to open file: " << CurrentExceptionMessage();
    }
}

TFsPath TSession::StubFileName(const TStubId& stubId, const TString* version) const {
    return StubsDirectory() / (TStringBuilder() << StubId(stubId, version) << ".proto");
}

// static
TString TSession::StubId(const TStubId& stubId, const TString* version) {
    return stubId.MakeKey(version ? version : &DIR_FOR_NEW_STUBS);
}

} // namespace NAlice::NJoker

template <>
void Out<NAlice::NJoker::TSessionId>(IOutputStream& out, const NAlice::NJoker::TSessionId& sessionId) {
    out << sessionId.Value();
}
