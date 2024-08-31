#include "ctrl_session.h"

#include "globalctx.h"

#include <alice/joker/library/backend/backend.h>
#include <alice/joker/library/log/log.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/stream/file.h>

namespace NAlice::NJoker {
namespace {

class IPushOutputVisitor;
class TRunLogVisitor;

class TStubLog {
public:
    using TProto = NJokerProto::TStubLogEntry;

public:
    static THolder<TStubLog> Create(const TSession& session, TProto&& proto);

public:
    TStubLog(const TProto& proto)
        : StubId_{proto.GetProjectId(), proto.GetParentId(), proto.GetReqHash()}
    {
    }
    virtual ~TStubLog() = default;

    virtual void Visit(TRunLogVisitor& visitor) const = 0;

    const TStubId& StubId() const {
        return StubId_;
    }

protected:
    TStubId StubId_;
};

class TStubLogFail final : public TStubLog {
public:
    explicit TStubLogFail(TString errorMsg) // TODO (petrk) add customized proto.
        : TStubLog{TProto{}}
        , ErrorMsg_{std::move(errorMsg)}
    {
    }

    TStubLogFail(const TProto& proto, const TProto::TFailed& data)
        : TStubLog{proto}
        , ErrorMsg_{data.GetMessage()}
    {
        if (data.GetRequest().HasFirstLine()) {
            FirstLine_.ConstructInPlace(data.GetRequest().GetFirstLine());
        }
    }

    void Visit(TRunLogVisitor& visitor) const override;

private:
    const TString ErrorMsg_;
    TMaybe<TString> FirstLine_;
};

class TStubLogDelete final : public TStubLog {
public:
    TStubLogDelete(const TProto& proto, const TSession& session, const TProto::TSynced& syncedProto)
        : TStubLog{proto}
    {
        TStubItemPtr stubItem;
        if (!session.LoadStub(StubId(), &syncedProto.GetVersion(), stubItem)) {
            // Success
            RequestUrl_.ConstructInPlace(stubItem->Get().GetRequest().GetUrl());
        }
    }

    void Visit(TRunLogVisitor& visitor) const override;

private:
    TMaybe<TString> RequestUrl_;
};

class TStubLogNotModifed final : public TStubLog {
public:
    TStubLogNotModifed(const TProto& proto, const TProto::TVersioned& versionedProto)
        : TStubLog{proto}
        , Version_{versionedProto.GetSynced().GetVersion()}
        , FirstLine_{versionedProto.GetRequest().GetFirstLine()}
    {
    }

    void Visit(TRunLogVisitor& visitor) const override;

private:
    const TString Version_;
    const TString FirstLine_;
};

class TStubLogNewVersion final : public TStubLog {
public:
    TStubLogNewVersion(const TProto& proto, const TProto::TCreated& created)
        : TStubLog{proto}
        , FirstLine_{created.GetRequest().GetFirstLine()}
        , Recent_{created.GetRecent()}
    {
    }

    TStubLogNewVersion(const TProto& proto, const TProto::TModified& modified)
        : TStubLog{proto}
        , FirstLine_{modified.GetCreated().GetRequest().GetFirstLine()}
        , Recent_{modified.GetCreated().GetRecent()}
        , SyncedVersion_{modified.GetVersioned().GetSynced().GetVersion()}
    {
    }

    void Visit(TRunLogVisitor& visitor) const override;

private:
    TString FirstLine_;
    bool Recent_;
    TMaybe<TString> SyncedVersion_;
};

// static
THolder<TStubLog> TStubLog::Create(const TSession& session, TStubLog::TProto&& proto) {
    THolder<TStubLog> stubLog;

    switch (proto.GetTypeCase()) {
        case TProto::TypeCase::kFailed:
            stubLog = MakeHolder<TStubLogFail>(proto, proto.GetFailed());
            break;

        // Stub has only synced type which means that it is not requested, so it will be deleted!
        case TProto::TypeCase::kSynced:
            stubLog = MakeHolder<TStubLogDelete>(proto, session, proto.GetSynced());
            break;

        case TProto::TypeCase::kVersioned:
            stubLog = MakeHolder<TStubLogNotModifed>(proto, proto.GetVersioned());
            break;

        case TProto::TypeCase::kModified:
            stubLog = MakeHolder<TStubLogNewVersion>(proto, proto.GetModified());
            break;

        case TProto::TypeCase::kCreated:
            stubLog = MakeHolder<TStubLogNewVersion>(proto, proto.GetCreated());
            break;

        case TProto::TypeCase::TYPE_NOT_SET:
            break;
    }

    return stubLog;
}

class TProject {
public:
    class TRequest {
    public:
        TRequest(const TSession& session, const TFsPath& requestDir, const TString& projectId)
            : Id_{requestDir.Basename()}
            , ProjectId_{projectId}
        {
            TVector<TFsPath> children;
            requestDir.List(children);
            for (const TFsPath& child : children) {
                if (!child.IsFile() || TStringBuf{child.GetName()}.RAfter('.') != TStringBuf("txt")) { // TODO create a separate function for loading txt
                    continue;
                }

                TStubLog::TProto proto;
                try {
                    TFileInput in{child};
                    proto.Load(&in);

                    THolder<TStubLog> stubLog = TStubLog::Create(session, std::move(proto));
                    if (!stubLog) {
                        stubLog = MakeHolder<TStubLogFail>(TStringBuilder{} << "Protolog file is invalid: " << child);
                    }
                    Stubs_.emplace_back(std::move(stubLog));
                } catch(...) {
                    Stubs_.emplace_back(MakeHolder<TStubLogFail>(TStringBuilder{} << "Unable to parse protolog file: " << child));
                    LOG(ERROR) << "Unable parse protolog file: " << CurrentExceptionMessage() << Endl;
                }
            }
        }

        void Visit(TRunLogVisitor& visitor) const;

        const TString& Id() const {
            return Id_;
        }

        const TString& ProjectId() const {
            return ProjectId_;
        }

        ui64 TotalStubs() const {
            return Stubs_.size();
        }

    private:
        TString Id_;
        TVector<THolder<TStubLog>> Stubs_;
        const TString& ProjectId_;
    };

public:
    TProject(const TSession& session, const TFsPath& projectDirectory)
        : Id_{projectDirectory.Basename()}
    {
        TVector<TFsPath> requestDirs;
        projectDirectory.List(requestDirs);
        for (const TFsPath& dir : requestDirs) {
            Requests_.emplace_back(session, dir, Id());
        }
    }

    size_t TotalRequests() const {
        return Requests_.size();
    }

    const TString& Id() const {
        return Id_;
    }

    void Visit(TRunLogVisitor& visitor) const;

private:
    TString Id_;
    TVector<TRequest> Requests_;
};

class TRunLogVisitor {
public:
    virtual ~TRunLogVisitor() = default;

    virtual void OnProject(const TProject& project) = 0;
    virtual void OnRequest(const TProject::TRequest& request) = 0;
    virtual void OnStubFail(const TStubId& stubId, TStringBuf msg, const TString* requestLine) = 0;
    virtual void OnStubDelete(const TStubId& stubId, const TString* requestLine) = 0;
    virtual void OnStubNotModified(const TStubId& stubId, const TString& version, const TString* requestLine) = 0;
    virtual void OnStubNewVersion(const TStubId& stubId, const TString& requestLine, bool recent, const TString* oldVersion) = 0;
};

class TTextOutputRunLogVisitor : public TRunLogVisitor {
public:
    TTextOutputRunLogVisitor()
        : Out_(Cout)
    {
    }

    void OnProject(const TProject& project) override {
        Out_ << "Project '" << project.Id() << "' has " << project.TotalRequests() << " requests." << Endl;
    }

    void OnRequest(const TProject::TRequest& request) override {
        Out_ << " * " << request.Id() << " (" << request.TotalStubs() << ')' << Endl;
    }

    void OnStubFail(const TStubId& stubId, TStringBuf msg, const TString* requestLine) override {
        Out_ << "   - [FAILED] " << stubId.MakeKey(nullptr) << " (" << msg << ")" << Endl;
        if (requestLine) {
            Out_ << "     " << *requestLine << Endl;
        }
    }

    void OnStubDelete(const TStubId& stubId, const TString* requestUrl) override {
        Out_ << "   - [DELETE] " << stubId.MakeKey(nullptr) << Endl;
        if (requestUrl) {
            Out_ << "     Url: " << *requestUrl << Endl;
        }
    }

    void OnStubNotModified(const TStubId& stubId, const TString& version, const TString* requestLine) override {
        Out_ << "   - [NOT_MODIFIED] " << stubId.MakeKey(&version) << " (" << version << ')' << Endl;
        if (requestLine) {
            Out_ << "     Request: " << *requestLine << Endl;
        }
    }

    void OnStubNewVersion(const TStubId& stubId, const TString& requestLine, bool recent, const TString* prevVersion) override {
        const TString key = stubId.MakeKey(nullptr);
        if (prevVersion) {
            if (recent) {
                Out_ << "   - [MODIFIED_RECENT] " << key << " (prev version: " << *prevVersion << ')' << Endl;
            } else {
                Out_ << "   - [MODIFIED] " << key << " (prev version: " << *prevVersion << ')' << Endl;
            }
        } else if (recent) {
            Out_ << "   - [NEW_RECENT] " << key << Endl;
        } else {
            Out_ << "   - [NEW] " << key << Endl;
        }

        Out_ << "     Request: " << requestLine << Endl;
    }

private:
    IOutputStream& Out_;
};

class TPushTextLineRunLogVisitor : public TRunLogVisitor {
public:
    TPushTextLineRunLogVisitor(TSession& session, const TString& separator, IOutputStream& outputStream = Cout)
        : Out_{outputStream}
        , Session_{session}
        , Separator_{separator}
    {
    }

    void OnProject(const TProject& /* project */) override {
    }

    void OnRequest(const TProject::TRequest& request) override {
        Out_ << request.ProjectId() << Separator_ << request.Id() << Endl;
    }

    void OnStubFail(const TStubId& /* stubId */, TStringBuf /* msg */, const TString* /* requestLine */) override {
    }

    void OnStubDelete(const TStubId& /* stubId */, const TString* /* requestUrl */) override {
    }

    void OnStubNotModified(const TStubId& stubId, const TString& version, const TString* /* requestLine */) override {
        Out_ << ' ' << stubId.MakeKey(&version) << Endl;
    }

    void OnStubNewVersion(const TStubId& stubId, const TString& /* requestLine */, bool /* recent */, const TString* /* prevVersion */) override {
        TStubItemPtr stubItem;
        if (auto error = Session_.LoadStub(stubId, nullptr, stubItem)) {
            return;
        }

        const TString version = stubItem->NewVersion();
        if (auto error = Session_.Backend().SaveStub(version, stubItem)) {
            return;
        }

        Out_ << ' ' << stubId.MakeKey(&version) << Endl;
    }

private:
    IOutputStream& Out_;
    TSession& Session_;
    TString Separator_;
};

void TProject::Visit(TRunLogVisitor& visitor) const {
    visitor.OnProject(*this);
    for (const auto& request : Requests_) {
        request.Visit(visitor);
    }
};

void TProject::TRequest::Visit(TRunLogVisitor& visitor) const {
    visitor.OnRequest(*this);
    for (const auto& stubLog : Stubs_) {
        stubLog->Visit(visitor);
    }
}

void TStubLogFail::Visit(TRunLogVisitor& visitor) const {
    visitor.OnStubFail(StubId(), ErrorMsg_, FirstLine_.Get());
}

void TStubLogDelete::Visit(TRunLogVisitor& visitor) const {
    visitor.OnStubDelete(StubId(), RequestUrl_.Get());
}

void TStubLogNotModifed::Visit(TRunLogVisitor& visitor) const {
    visitor.OnStubNotModified(StubId(), Version_, &FirstLine_);
}

void TStubLogNewVersion::Visit(TRunLogVisitor& visitor) const {
    visitor.OnStubNewVersion(StubId(), FirstLine_, Recent_, nullptr);
}

TVector<TProject> ParseRunLogDirectory(const TFsPath& currentRunLogDirectory, const TSession& session) {
    TVector<TProject> projects;

    // All the directories must be projects!
    TVector<TFsPath> children;
    currentRunLogDirectory.List(children);
    for (const TFsPath& item : children) {
        if (!item.IsDirectory()) {
            continue;
        }

        projects.emplace_back(session, item);
    }

    return projects;
}

} // namespace

TSessionControl::TSessionControl(TSessionId id, const TConfig& config, const TString& runId, TInstant runAt, TFsPath currentRunDirectory)
    : TSession{std::move(id), config, runId, runAt}
    , CurrentRunLogDirectory_{std::move(currentRunDirectory)}
{
}

// static
TStatus TSessionControl::Load(const TSessionId& id, const TConfig& config, THolder<TSessionControl>& session) {
    TMaybe<TFsPath> latestRunLogDir = LatestRunLogDirectory(id, config);
    if (!latestRunLogDir.Defined()) {
        return TError{TError::EType::Logic} << "No runs for the session: " << id.Value();
    }

    NJokerProto::TSessionInfo proto;
    if (!TryParseFromTextFormat(ToString(*latestRunLogDir / "info.txt"), proto)) {
        return TError{TError::EType::Logic} << "Incorrect info.txt format in " << *latestRunLogDir;
    }

    session = THolder<TSessionControl>(new TSessionControl(
        TSessionId{proto.GetId()},
        config,
        proto.GetRunId(),
        TInstant::MilliSeconds(proto.GetRunAtMs()),
        std::move(*latestRunLogDir)
    ));

    return Success();
}

TStatus TSessionControl::Clear() {
    try {
        Directory().ForceDelete();
    } catch (...) {
        return TError{TError::EType::Logic} << "Unable to clear session: " << CurrentExceptionMessage();
    }

    return Success();
}

TStatus TSessionControl::Push(TGlobalContext&, IOutputStream& outputStream) {
    TVector<TProject> projects = ParseRunLogDirectory(CurrentRunLogDirectory_, *this);

    TPushTextLineRunLogVisitor visitor(*this, "/", outputStream); // TODO (petrk) Get separator from args.
    for (const TProject& project : projects) {
        project.Visit(visitor);
    }

    return Success();
}

TStatus TSessionControl::OutTo(IOutputStream& out) const {
    TVector<TProject> projects = ParseRunLogDirectory(CurrentRunLogDirectory_, *this);

    out << "Session Id: " << Id_.Value() << Endl
        << "Run Id: " << RunId_ << Endl
        << "Run at: " << StartedAt_ << Endl;

    TTextOutputRunLogVisitor visitor;

    for (const TProject& project : projects) {
        project.Visit(visitor);
    }

    return Success();
}

} // namespace NAlice::NJoker
