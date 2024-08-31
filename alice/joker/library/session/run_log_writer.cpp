#include "run_log_writer.h"

#include <alice/joker/library/log/log.h>
#include <alice/joker/library/proto/protos.pb.h>

#include <util/stream/file.h>
#include <util/string/builder.h>

namespace NAlice::NJoker {

// TRunLogEntry ---------------------------------------------------------------
TRunLogEntry::TRunLogEntry(const TStubId& stubId)
    : StubId_{stubId}
{
}

void TRunLogEntry::SetupProto(NJokerProto::TStubLogEntry& proto) const {
    proto.SetProjectId(StubId().ProjectId());
    proto.SetParentId(StubId().ParentId());
    proto.SetReqHash(StubId().ReqHash());
    SetupProtoImpl(proto);
}

// TVersionLogEntry -----------------------------------------------------------
TVersionLogEntry::TVersionLogEntry(const TStubId& stubId, const TString& version)
    : TRunLogEntry{stubId}
    , Version_{version}
{
}

TVersionLogEntry::TVersionLogEntry(const TStubId& stubId, const TString& version, const TString& firstLine)
    : TRunLogEntry{stubId}
    , Version_{version}
    , FirstLine_{firstLine}
{
}

void TVersionLogEntry::SetupProtoImpl(NJokerProto::TStubLogEntry& proto) const {
    if (FirstLine_.Defined()) {
        auto* typeVersioned = proto.MutableVersioned();
        auto* typeSynced = typeVersioned->MutableSynced();
        typeSynced->SetVersion(Version_);
        auto* request = typeVersioned->MutableRequest();
        request->SetFirstLine(*FirstLine_);
    } else {
        auto* typeSynced = proto.MutableSynced();
        typeSynced->SetVersion(Version_);
    }
}

// TFailedLogEntry ------------------------------------------------------------
TFailedLogEntry::TFailedLogEntry(const TStubId& stubId, const TString& message, const TString& firstLine)
    : TRunLogEntry{stubId}
    , Message_{message}
    , FirstLine_{firstLine}
{
}

void TFailedLogEntry::SetupProtoImpl(NJokerProto::TStubLogEntry& proto) const {
    auto* typeFailed = proto.MutableFailed();
    typeFailed->SetMessage(Message_);
    auto* request = typeFailed->MutableRequest();
    request->SetFirstLine(FirstLine_);
}

// TCreatedLogEntry --------------------------------------------------------
TCreatedLogEntry::TCreatedLogEntry(const TStubId& stubId, const TString& firstLine, const TMaybe<TString>& version, bool recent)
    : TRunLogEntry{stubId}
    , FirstLine_{firstLine}
    , Version_{version}
    , Recent_{recent}
{
}

void TCreatedLogEntry::SetupProtoImpl(NJokerProto::TStubLogEntry& proto) const {
    // TODO (petrk) Refactor it!

    if (Version_.Defined()) { // Create TModified
        auto* typeModified = proto.MutableModified();

        { // Versioned
            auto* typeVersioned = typeModified->MutableVersioned();
            auto* typeSynced = typeVersioned->MutableSynced();
            typeSynced->SetVersion(*Version_);
            auto* request = typeVersioned->MutableRequest();
            request->SetFirstLine(FirstLine_);
        }

        { // Created
            auto* typeCreated = typeModified->MutableCreated();
            typeCreated->SetRecent(Recent_);
            auto* request = typeCreated->MutableRequest();
            request->SetFirstLine(FirstLine_);
        }

    } else { // Create TCreated
        auto* typeCreated = proto.MutableCreated();
        typeCreated->SetRecent(Recent_);
        auto* request = typeCreated->MutableRequest();
        request->SetFirstLine(FirstLine_);
    }
}

// TRunLogWriter --------------------------------------------------------------
TRunLogWriter::TRunLogWriter(const TFsPath& runLogDir)
    : RunLogDir_{runLogDir}
{
}

void TRunLogWriter::Add(THolder<TRunLogEntry>&& logEntry) {
    WriteImpl(*logEntry);
}

void TRunLogWriter::WriteImpl(TRunLogEntry& logEntry) {
    NJokerProto::TStubLogEntry proto;
    logEntry.SetupProto(proto);

    try {
        const TFsPath fullPath{RunLogDir_ / logEntry.StubId().MakePath(nullptr, ".txt")};
        fullPath.Parent().MkDirs();

        TFileOutput file{fullPath};
        proto.Save(&file);
    } catch (...) {
        LOG(ERROR) << "Unable to write stubrunlog: " << CurrentExceptionMessage() << Endl;
    }
}

} // namespace NAlice::NJoker
