#pragma once

#include "experiments.h"
#include "fwd.h"

#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/framework/framework_migration.h>

#include <alice/library/client/client_info.h>
#include <alice/library/geo/user_location.h>
#include <alice/library/geo_resolver/geo_position.h>
#include <alice/library/restriction_level/restriction_level.h>

#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <apphost/api/service/cpp/service_context.h>

#include <google/protobuf/any.pb.h>

namespace NAlice::NHollywood {

class TScenarioBaseRequestWrapper {
public:
    using TProto = NScenarios::TScenarioBaseRequest;

    explicit TScenarioBaseRequestWrapper(const NScenarios::TScenarioBaseRequest& request, const NAppHost::IServiceContext& serviceCtx);

    const NScenarios::TScenarioBaseRequest& BaseRequestProto() const {
        return BaseRequestProto_;
    }

    const TExpFlags& ExpFlags() const {
        return ExpFlags_;
    }

    TMaybe<TString> ExpFlag(TStringBuf name) const {
        if (const auto* ptr = ExpFlags_.FindPtr(name)) {
            return *ptr;
        }
        return Nothing();
    }

    TMaybe<TString> GetValueFromExpPrefix(const TStringBuf expPrefix) const {
        for (const auto& expFlag : ExpFlags_) {
            if (expFlag.first.StartsWith(expPrefix)) {
                return expFlag.first.substr(expPrefix.size());
            }
        }
        return Nothing();
    }

    template <typename T>
    T LoadValueFromExpPrefix(const TStringBuf expPrefix, const T defaultValue) const {
        T value = defaultValue;
        if (const auto expValue = GetValueFromExpPrefix(expPrefix); expValue.Defined()) {
            TryFromString(*expValue, value);
        }
        return value;
    }

    virtual bool HasExpFlag(TStringBuf name) const {
        const auto maybeFlag = ExpFlag(name);
        return maybeFlag.Defined() && maybeFlag.GetRef() != TStringBuf("0");
    }

    const TClientInfo& ClientInfo() const {
        return ClientInfo_;
    }

    // deprecated
    EContentSettings ContentRestrictionLevel() const {
        return ContentRestrictionLevel_;
    }

    NAlice::NScenarios::TUserPreferences_EFiltrationMode FiltrationMode() const {
        return BaseRequestProto_.GetUserPreferences().GetFiltrationMode();
    }

    ui64 ServerTimeMs() const {
        return BaseRequestProto_.GetServerTimeMs();
    }

    bool IsNewSession() const {
        return BaseRequestProto_.GetIsNewSession();
    }

    const TString& RequestId() const {
        return RequestId_;
    }

    const TString& DialogId() const {
        return BaseRequestProto_.GetDialogId();
    }

    const TLocation& Location() const {
        return Location_;
    }

    template <typename TProtoState>
    TProtoState LoadState() const;

    bool HasPersonalData(TStringBuf key) const {
        return RawPersonalData_.Has(key);
    }

    const NJson::TJsonValue* GetPersonalDataValue(TStringBuf key) const {
        const NJson::TJsonValue* result;
        if (!RawPersonalData_.GetValuePointer(key, &result)) {
            return nullptr;
        }
        return result;
    }

    const TString* GetPersonalDataString(TStringBuf key) const {
        const NJson::TJsonValue* result;
        if (!RawPersonalData_.GetValuePointer(key, &result)) {
            return nullptr;
        } else {
            return &result->GetStringSafe();
        }
    }

    const NScenarios::TInterfaces& Interfaces() const {
        return BaseRequestProto_.GetInterfaces();
    }

    NScenarios::TScenarioBaseRequest::ERequestSourceType RequestSource() const {
        return BaseRequestProto_.GetRequestSource();
    }

    bool IsStackOwner() const {
        return BaseRequestProto_.GetIsStackOwner();
    }

    const NAppHost::IServiceContext& ServiceCtx() const {
        return ServiceCtx_;
    }

    virtual ~TScenarioBaseRequestWrapper() = default;

private:
    const NScenarios::TScenarioBaseRequest& BaseRequestProto_;

    TExpFlags ExpFlags_;
    TClientInfo ClientInfo_;

    EContentSettings ContentRestrictionLevel_;

    const TString& RequestId_;
    const TLocation& Location_;
    NJson::TJsonValue RawPersonalData_;

    const NAppHost::IServiceContext& ServiceCtx_;
};

template <typename TProtoState>
TProtoState TScenarioBaseRequestWrapper::LoadState() const {
    TProtoState state;

    const auto rawState = BaseRequestProto_.GetState();
    if (rawState.Is<TProtoState>()) {
        rawState.UnpackTo(&state);
    }
    return state;
}

class TScenarioInputWrapper {
public:
    using TProto = NScenarios::TInput;

public:
    explicit TScenarioInputWrapper(const TProto& proto)
        : Proto_(proto)
    {}

    const TProto& Proto() const {
        return Proto_;
    }

    TString Utterance() const;
    bool IsTextInput() const;
    bool IsVoiceInput() const;

    const TPtrWrapper<NAlice::TSemanticFrame> FindSemanticFrame(const TStringBuf frameName) const;
    TFrame CreateRequestFrame(const TStringBuf frameName) const;
    TMaybe<TFrame> TryCreateRequestFrame(const TStringBuf frameName) const;

    const NScenarios::TCallbackDirective* GetCallback() const;

private:
    const TProto& Proto_;
};

class TScenarioBaseRequestWithInputWrapper : public TScenarioBaseRequestWrapper {
public:
    TScenarioBaseRequestWithInputWrapper(const NScenarios::TScenarioBaseRequest& request, const NScenarios::TInput& input, const NAppHost::IServiceContext& serviceCtx)
        : TScenarioBaseRequestWrapper(request, serviceCtx)
        , Input_(input)
    {}

    const TScenarioInputWrapper& Input() const {
        return Input_;
    }

private:
    TScenarioInputWrapper Input_;
};


class TScenarioRunRequestWrapper : public TScenarioBaseRequestWithInputWrapper {
public:
    using TProto = NScenarios::TScenarioRunRequest;

public:
    TScenarioRunRequestWrapper(const TProto& request, const NAppHost::IServiceContext& serviceCtx)
        : TScenarioBaseRequestWithInputWrapper(request.GetBaseRequest(), request.GetInput(), serviceCtx)
        , Proto_(request)
    {}

    virtual const TProto& Proto() const {
        return Proto_;
    }

    const NScenarios::TDataSource* GetDataSource(NAlice::EDataSourceType sourceType) const;

    virtual ~TScenarioRunRequestWrapper() = default;

private:
    const TProto& Proto_;
    mutable THashMap<NAlice::EDataSourceType, NScenarios::TDataSource> ContextDataSourcesCache_;
};


class TScenarioApplyRequestWrapper : public TScenarioBaseRequestWithInputWrapper {
public:
    using TProto = NScenarios::TScenarioApplyRequest;

public:
    TScenarioApplyRequestWrapper(const TProto& request, const NAppHost::IServiceContext& serviceCtx)
        : TScenarioBaseRequestWithInputWrapper(request.GetBaseRequest(), request.GetInput(), serviceCtx)
        , Proto_(request)
    {}

    const TProto& Proto() const {
        return Proto_;
    }

    template <typename TArguments>
    TArguments UnpackArguments() const {
        TArguments result;
        Y_ENSURE(ReadArguments(Proto_, result));
        return result;
    }

    template <typename TArguments>
    const TArguments& UnpackArgumentsAndGetRef() const {
        if (!UnpackedArguments_) {
            auto proto = UnpackArguments<TArguments>();
            UnpackedArguments_ = std::make_unique<TArguments>(std::move(proto));
        }
        const auto* rawArgs = UnpackedArguments_.get();
        const auto* args = google::protobuf::DynamicCastToGenerated<const TArguments>(rawArgs);
        Y_ENSURE(args);
        return *args;
    }

private:
    const TProto& Proto_;
    mutable THashMap<NAlice::EDataSourceType, NScenarios::TDataSource> ContextDataSourcesCache_;
    mutable std::unique_ptr<google::protobuf::Message> UnpackedArguments_;
};

template<typename T>
concept TScenarioRequestWrapper = requires {
    std::is_base_of_v<TScenarioBaseRequestWrapper, T>;
    &T::Proto;
    &T::Input;
};

static_assert(!TScenarioRequestWrapper<TScenarioBaseRequestWrapper>);
static_assert(TScenarioRequestWrapper<TScenarioRunRequestWrapper>);
static_assert(TScenarioRequestWrapper<TScenarioApplyRequestWrapper>);

const NAlice::TBlackBoxUserInfo* GetUserInfoProto(const TScenarioRunRequestWrapper& request);

const NAlice::TIoTUserInfo* GetIoTUserInfoProto(const TScenarioRunRequestWrapper& request);

const NAlice::TEnvironmentState* GetEnvironmentStateProto(const TScenarioRunRequestWrapper& request);

TStringBuf GetUid(const TScenarioRunRequestWrapper& request);

const NAlice::TGuestData* GetGuestDataProto(const TScenarioRunRequestWrapper& request);

const NAlice::TGuestOptions* GetGuestOptionsProto(const TScenarioRunRequestWrapper& request);

TUserLocation GetUserLocation(const TScenarioRequestWrapper auto& request) {
    const NScenarios::TDataSource* userLocationPtr = nullptr;
    if constexpr (requires { request.GetDataSource; }) {
        userLocationPtr = request.GetDataSource(NAlice::EDataSourceType::USER_LOCATION);
    }
    const auto& tz = request.Proto().GetBaseRequest().GetClientInfo().GetTimezone();
    return userLocationPtr ? TUserLocation{userLocationPtr->GetUserLocation(), tz} : TUserLocation{tz, /* tld */ "ru"};
}

TMaybe<TGeoPosition> InitGeoPositionFromRequest(const NScenarios::TScenarioBaseRequest& request);

TMaybe<TFrame> TryGetFrame(TStringBuf frameName, const TMaybe<TFrame>& callbackFrame, const TScenarioInputWrapper& input);

} // namespace NAlice::NHollywood
