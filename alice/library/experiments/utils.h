#pragma once

#include <alice/megamind/protos/common/experiments.pb.h>

#include <google/protobuf/struct.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/string.h>

namespace NAlice::NMegamind {

class IExpFlagVisitor {
public:
    virtual ~IExpFlagVisitor() = default;

    virtual void operator()(const TString& key, bool value) = 0;
    virtual void operator()(const TString& key, double value) = 0;
    virtual void operator()(const TString& key, const TString& value) = 0;
    virtual void operator()(const TString& key) = 0;

    void Visit(const TExperimentsProto& proto);
};

class TExpFlagsToStructVisitor : public IExpFlagVisitor {
public:
    using TStorage = ::google::protobuf::Struct;

public:
    explicit TExpFlagsToStructVisitor(TStorage& storage);

    void operator()(const TString& key, bool value) override;
    void operator()(const TString& key, double value) override;
    void operator()(const TString& key, const TString& value) override;
    void operator()(const TString& key) override;

private:
    TStorage& Storage_;
};

class TExpFlagsToJsonVisitor : public IExpFlagVisitor {
public:
    explicit TExpFlagsToJsonVisitor(NJson::TJsonValue& json);

    void operator()(const TString& key, bool value) override;
    void operator()(const TString& key, double value) override;
    void operator()(const TString& key, const TString& value) override;
    void operator()(const TString& key) override;

private:
    NJson::TJsonValue& Json_;
};

} // namespace NAlice::NMegamind
