#pragma once

#include "config.h"

#include <alice/joker/library/status/status.h>
#include <alice/joker/library/stub/stub_id.h>

#include <util/folder/path.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/datetime/base.h>
#include <util/system/mutex.h>

namespace NAlice::NJoker {

class TSessionId {
public:
    TSessionId();
    TSessionId(const TString& id);

    const TString& Value() const {
        return Id_;
    }

    operator const TString& () const {
        return Id_;
    }

    operator TStringBuf () const {
        return Id_;
    }

    TString DirName() const;

    const TString& RunId() const {
        return RunId_;
    }

private:
    const TString Id_;
    const TString RunId_;
    const TInstant RunAt_;
};

class TSession {
public:
    // Just a name for using in functions which need *version.
    static inline constexpr TString* NewVersion = nullptr;

public:
    TSession(TSessionId id, const TConfig& config);
    TSession(TSessionId id, const TConfig& config, const TString& runId, TInstant startedAt);
    virtual ~TSession() = default;

    const TSessionId& Id() const {
        return Id_;
    }

    /** Load stub data from a local cache.
     */
    TStatus LoadStub(const TStubId& id, const TString* version, TStubItemPtr& stubItem) const;
    const TFsPath& Directory() const;
    TFsPath StubFileName(const TStubId& stubId, const TString* version) const;

    const TString& RunId() const;

public:
    static TFsPath SessionFile(const TSessionId& id, const TConfig& config);
    static TFsPath SessionDirectory(const TSessionId& id, const TConfig& config);
    static TFsPath RunLogDirectory(const TSessionId& id, const TConfig& config);
    static TMaybe<TFsPath> LatestRunLogDirectory(const TSessionId& id, const TConfig& config);

    TFsPath StubsDirectory() const;

    TBackend& Backend() {
        return *Backend_;
    }

protected:
    static TString StubId(const TStubId& stubId, const TString* version);

    TFsPath CurrentRunLogDirectory() const;

    TStatus WriteInfoFile() const;

protected:
    const TSessionId Id_;
    const TString RunId_;
    const TInstant StartedAt_;

protected:
    const TFsPath Directory_;

    TMutex SessionLock_;

private:
    THolder<TBackend> Backend_;
};

} // namespace NAlice::NJoker
