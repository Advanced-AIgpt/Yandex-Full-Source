#pragma once

#include <alice/cuttlefish/library/protos/session.pb.h>
#include <alice/megamind/protos/common/experiments.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>

#include <type_traits>
#include <variant>


namespace NVoice::NExperiments {

    // During the request processing session we perform several actions related to the experiment flags:
    // 1) Get raw json from flags.json service, parse it and store as part of SessionContext.
    //    To do it we use function ParseFlagsInfoFromRawResponse.
    // 2) Read flags from SessionContex. In most currently existing use cases SessionContex outlives the
    //    TFlagsJsonFlagsBase object, so we can store protobuf by non-owning const pointer.
    //    To do it we use function MakeFlagsConstRefFromSessionContextProto.
    // If you need to modify TFlagsInfo or own it, just create some new wrapper.
    // Please, do not read/write raw proto. You'll get ugly code with proto-map and oneof.

    class TFlagsJsonFlagsBase : public TThrRefBase {
    public:
        NAlice::TExperimentsProto::TValue ExperimentFlagValue(TStringBuf expName) const;
        bool ConductingExperiment(TStringBuf expName) const;

        TMaybe<bool> GetExperimentValueBoolean(const TString& expName) const;
        TMaybe<int32_t> GetExperimentValueInteger(const TString& expName) const;
        TMaybe<double> GetExperimentValueFloat(const TString& expName) const;
        TMaybe<TString> GetExperimentValueString(const TString& expName) const;

        NJson::TJsonValue GetVoiceFlagsJson() const;
        TMaybe<int32_t> GetExpConfigVersion() const;
        TMaybe<TString> GetAsrAbFlagsSerializedJson() const;
        TMaybe<TString> GetBioAbFlagsSerializedJson() const;
        TMaybe<TString> GetExpBoxes() const;
        TMaybe<TString> GetRequestId() const;

        // method for work with dirty hack when including value into name,
        // sample from https://ab.yandex-team.ru/testid/307521:
        //     { "flags": [ "asr_partials_threshold_0.26" ] }
        TMaybe<TString> GetValueFromName(TStringBuf prefixExpName) const;

        TString ToString() const;  // for logging

    protected:
        virtual const NAliceProtocol::TFlagsInfo* GetFlagsInfoProto() const = 0;
    };


    class TFlagsJsonFlagsConstRef : public TFlagsJsonFlagsBase {
    public:
        explicit TFlagsJsonFlagsConstRef(const NAliceProtocol::TFlagsInfo*);
        TFlagsJsonFlagsConstRef(const TFlagsJsonFlagsConstRef&) = default;

    protected:
        const NAliceProtocol::TFlagsInfo* GetFlagsInfoProto() const override;

    private:
        const NAliceProtocol::TFlagsInfo* FlagsInfoProto;
    };


    class TFlagsJsonFlagsHolder : public TFlagsJsonFlagsBase {
    public:
        explicit TFlagsJsonFlagsHolder(NAliceProtocol::TFlagsInfo);

    protected:
        const NAliceProtocol::TFlagsInfo* GetFlagsInfoProto() const override;

    private:
        NAliceProtocol::TFlagsInfo FlagsInfoProto;
    };


    using TFlagsJsonFlagsPtr = TIntrusivePtr<TFlagsJsonFlagsBase>;

    bool ParseFlagsInfoFromRawResponse(NAliceProtocol::TFlagsInfo* result, TStringBuf rawFjResponse);
    bool ParseFlagsInfoFromJsonResponse(NAliceProtocol::TFlagsInfo* result, const NJson::TJsonValue& jsonFjResponse);
    TFlagsJsonFlagsPtr MakeFlagsConstRefFromSessionContextProto(const NAliceProtocol::TSessionContext& ctx);
    TFlagsJsonFlagsPtr MakeFlagsHolderFromSessionContextProto(const NAliceProtocol::TSessionContext& ctx);

} // namespace NVoice::NExperiments
