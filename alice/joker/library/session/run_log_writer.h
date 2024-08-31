#pragma once

#include <alice/joker/library/proto/protos.pb.h>

#include <alice/joker/library/stub/stub_id.h>

#include <util/folder/path.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>

namespace NAlice::NJoker {

class TRunLogEntry {
public:
    TRunLogEntry(const TStubId& stubId);
    virtual ~TRunLogEntry() = default;

    const TStubId& StubId() const {
        return StubId_;
    }

    void SetupProto(NJokerProto::TStubLogEntry& proto) const;

protected:
    virtual void SetupProtoImpl(NJokerProto::TStubLogEntry& proto) const = 0;

private:
    const TStubId StubId_;
};

class TVersionLogEntry final : public TRunLogEntry {
public:
    TVersionLogEntry(const TStubId& stubId, const TString& version);
    TVersionLogEntry(const TStubId& stubId, const TString& version, const TString& firstLine);

protected:
    void SetupProtoImpl(NJokerProto::TStubLogEntry& proto) const override;

private:
    const TString Version_;
    const TMaybe<TString> FirstLine_;
};

class TFailedLogEntry final : public TRunLogEntry {
public:
    TFailedLogEntry(const TStubId& stubId, const TString& message, const TString& firstLine);

protected:
    void SetupProtoImpl(NJokerProto::TStubLogEntry& proto) const override;

private:
    const TString Message_;
    const TString FirstLine_;
};

class TCreatedLogEntry : public TRunLogEntry {
public:
    TCreatedLogEntry(const TStubId& stubId, const TString& firstLine, const TMaybe<TString>& version, bool recent);

protected:
    void SetupProtoImpl(NJokerProto::TStubLogEntry& proto) const override;

private:
    const TString FirstLine_;
    const TMaybe<TString> Version_;
    const bool Recent_;
};

// TODO (petrk) Make it async when needed.
class TRunLogWriter {
public:
    TRunLogWriter(const TFsPath& runLogDir);

    template <typename T, typename ...TArgs>
    void Emplace(TArgs... args) {
        Add(MakeHolder<T>(std::forward<TArgs>(args)...));
    }

    void Add(THolder<TRunLogEntry>&& logEntry);

private:
    void WriteImpl(TRunLogEntry& logEntry);

private:
    const TFsPath RunLogDir_;
};

} // namespace NAlice::NJoker
