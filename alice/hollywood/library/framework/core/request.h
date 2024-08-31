#pragma once

//
// HOLLYWOOD FRAMEWORK
// Base request information
//

#include <alice/hollywood/library/fast_data/fast_data.h>
#include <alice/hollywood/library/frame/frame.h> // OBSOLETE, for old style requests only
#include <alice/hollywood/library/frame/slot.h>
#include <alice/hollywood/library/nlg/nlg.h>

#include <alice/library/client/client_features.h>
#include <alice/library/client/client_info.h>
#include <alice/library/geo/user_location.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/logger/logger.h>
#include <alice/library/restriction_level/restriction_level.h>
#include <alice/library/util/rng.h>

#include <alice/megamind/protos/common/data_source_type.pb.h>

#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>

#include <apphost/api/service/cpp/service_context.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/langs/langs.h>

#include <google/protobuf/any.pb.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>

#include <chrono>

//
// Forward declarations for protobufs and 3rd party objects
//
namespace NAlice::NScenarios {
    class TRequestMeta;
    class TScenarioRunRequest;
    class TScenarioBaseRequest;
    class TScenarioApplyRequest;
    class TScenarioResponseBody;
    class TInput;
    class TInterfaces;
    class TCallbackDirective;
    class TDataSource;
    class TAnalyticsInfo_TAction;
    class TAnalyticsInfo_TObject;
    class TAnalyticsInfo_TEvent;
} // namespace NAlice::NScenarios

namespace NAppHost {
    class IServiceContext;
} // namespace NAppHost

namespace NAlice::NHollywood {
    class TCommonResources;
    class IGlobalContext;
}

namespace NAlice::NHollywoodFw {

//
// Protobuf forward declaration
//
class TProtoHwScene;
class TScenario;

//
// Aliases for classes from old namespace NAlice::NHollywood
//
using TCommonResources = NAlice::NHollywood::TCommonResources;
using TFastData = NAlice::NHollywood::TFastData;

namespace NPrivate {

struct TAICustomData {
    TString Psn;
    TString Intent;
    TString OutputFrameName;
};

class TRequestHelper;

} // namespace NPrivate

//
// Type of called node
//
enum class ENodeType {
    Run,
    Continue,
    Apply,
    Commit
};


//
// Base Hollywood request class definition
// Always constant, always read only
//
class TRequest: public NNonCopyable::TNonCopyable {
public:
    // Additional information about current apphost node
    // Use TRequest::GetApphostInfo() to retrive information
    struct TApphostNodeInfo {
        // Name of the current scenario
        TString ScenarioName;
        // Current node name (i.e. full grpc path will be ScenarioName/NodeName)
        TString NodeName;
        // Apphost parameters information
        NJson::TJsonValue ApphosParams;
        // Type of called node (Run/Continue/...)
        ENodeType NodeType;
    };

    // ctor (internal function, created automatically)
    TRequest(const NScenarios::TRequestMeta& requestMeta,
             const NScenarios::TScenarioBaseRequest& baseRequest,
             const NScenarios::TInput& inputProto,
             const NAppHost::IServiceContext& serviceCtx,
             const TProtoHwScene* protoHwScene,
             NAlice::NHollywood::IGlobalContext& globalCtx,
             TRTLogger& logger,
             const TApphostNodeInfo& apphostNodeInfo,
             TMaybe<NHollywood::TCompiledNlgComponent> nlg);

    const TApphostNodeInfo& GetApphostInfo() const {
        return ApphostInfo_;
    }

    //
    // Obsolete functions, used only for compatibility
    // Will be deprecated soon
    //
    const NScenarios::TRequestMeta& GetRequestMeta() const {
        return RequestMeta_;
    }
    const NAppHost::IServiceContext& GetServiceCtx() const {
        return ServiceCtx_;
    }

    //
    // System subclass of TRequest
    // Generic information about request (ID, etc)
    //
    class TSystem: public NNonCopyable::TNonCopyable {
    friend class TRequest;
    public:
        const TString& RequestId() const {
            return RequestId_;
        }
        TRng& Random() const {
            return Random_;
        }
        const TCommonResources& GetCommonResources() const {
            return CommonResources_;
        }
        TFastData& GetFastData() const {
            return FastData_;
        }
        NMetrics::ISensors& GetSensors() const {
            return Sensors_;
        }

    private:
        explicit TSystem(const NScenarios::TRequestMeta& requestMeta,
                         NAlice::NHollywood::IGlobalContext& globalCtx,
                         const NScenarios::TScenarioBaseRequest& baseRequest);
        // This feature is present in MM protobuf but can not be used inside scenario code without extra permition
        std::chrono::milliseconds GetServerTime() const {
            return ServerTime_;
        }
    private:
        TString RequestId_;
        mutable TRng Random_;
        std::chrono::milliseconds ServerTime_;
        const TCommonResources& CommonResources_;
        TFastData& FastData_;
        TApphostNodeInfo ApphostInfo_;
        NMetrics::ISensors& Sensors_;
    };

    const TSystem& System() const {
        return System_;
    }

    //
    // Debug subclass of TRequest
    // Contains logging information and additional functions
    //
    class TDebug: public NNonCopyable::TNonCopyable {
    friend class TRequest;
    public:
        TDebug(TRTLogger& logger)
            : Logger_(logger) {
        }
        TRTLogger& Logger() const {
            return Logger_;
        }
    private:
        TRTLogger& Logger_;
    };

    const TDebug& Debug() const {
        return Debug_;
    }

    //
    // Client
    // Contains information about device
    //
    class TClient: public NNonCopyable::TNonCopyable {
    friend class TRequest;
    friend class NPrivate::TRequestHelper;
    public:
        class TDeviceInfo {
        friend class TClient;
        public:
            ELanguage GetDeviceLanguage() const {
                return DeviceLang_;
            }
        private:
            explicit TDeviceInfo(const NScenarios::TRequestMeta& requestMeta);
        private:
            ELanguage DeviceLang_;
        };

        const TDeviceInfo& GetDeviceInfo() const {
            return DeviceInfo_;
        }
        // Get current client time in GMT
        std::chrono::milliseconds GetClientTimeMsGMT() const;
        std::chrono::milliseconds GetClientTimeMs(TString& tz) const;
        const TString& GetTimezone() const;

        // Get TInterfaces object (supported features)
        const NScenarios::TInterfaces& GetInterfaces() const;

        // TODO This is a temporary reference, should be removed soon
        const TClientInfo& GetClientInfo() const {
            return ClientInfo_;
        }

        // TODO This is a temporary reference, should be removed soon
        // Try to get any message inside TScenarioBaseRequest
        template <class TBaseProto>
        TMaybe<TBaseProto> TryGetMessage() const {
            TBaseProto proto;
            if (TryGetMessageImpl(proto)) {
                return std::move(proto);
            }
            return Nothing();
        }

    private:
        explicit TClient(const NScenarios::TScenarioBaseRequest& baseRequestProto, const NScenarios::TRequestMeta& requestMeta);
        bool TryGetMessageImpl(google::protobuf::Message& object) const;

    private:
        const NScenarios::TScenarioBaseRequest& BaseRequestProto_;
        TDeviceInfo DeviceInfo_;
        TClientInfo ClientInfo_;
    };

    const TClient& Client() const {
        return Client_;
    }

    //
    // Experiments
    //
    class TFlags {
    friend class TRequest;
    public:
        // Check experiment exists
        bool IsExperimentEnabled(TStringBuf key) const;
        // Check is this key is a full key or contains subvalue
        bool IsKeyComplex(TStringBuf primaryKey) const;

        // Get experiment value (format "key":"value")
        template <typename T>
        TMaybe<T> GetValue(TStringBuf key) const {
            const auto& it = Experiments_.find(key);
            if (it == Experiments_.end() || !it->second.Defined()) {
                return Nothing();
             }
             T result;
             if (!TryFromString<T>(*(it->second), result)) {
                 return Nothing();
             }
             return result;
        }
        template <typename T>
        T GetValue(TStringBuf key, T defaultValue) const {
            const auto res = GetValue<T>(key);
            return res.Defined() ? *res : defaultValue;
        }

        // Versions for experiments "key=value":"1"
        template <typename T>
        TMaybe<T> GetSubValue(TStringBuf key) const {
            const auto* ptr = FindIfPtr(SubvalExperiments_, [key](const TExpWithSubvalues& it) {
                return it.MainKey == key;
            });
             T result;
             if (ptr == nullptr || !TryFromString<T>(ptr->MainKeySubvalue, result)) {
                 return Nothing();
             }
             // Blocks return subvalue if experiment is disabled
             if (!ptr->Value.Defined() || *ptr->Value == "0") {
                 return Nothing();
             }
             return result;
        }
        template <typename T>
        T GetSubValue(TStringBuf key, T defaultValue) const {
            const auto res = GetSubValue<T>(key);
            return res.Defined() ? *res : defaultValue;
        }

        /*
            Enumerate all existing flags
            User provides callback function to receive all pairs TString key, TMaybe<TString> value
            User callback function may return true to continue processing
            User callback function may return false to stop processing
        */
        template <typename TFunc>
        bool ForEach(TFunc fn) const {
            for (const auto& [key, value] : Experiments_) {
                if (!fn(key, value)) {
                    return false;
                }
            }
            return true;
        }
        template <typename TFunc>
        bool ForEachSubval(TFunc fn) const {
            for (const auto& [key, subval, value] : SubvalExperiments_) {
                if (!fn(key, subval, value)) {
                    return false;
                }
            }
            return true;
        }

    private:
        explicit TFlags(const NScenarios::TScenarioBaseRequest& baseRequestProto, TRTLogger& logger);

    private:
        // Precached structure with subkeys
        struct TExpWithSubvalues {
            TString MainKey;         // Main part (i.e. 1st part in key=subvalue)
            TString MainKeySubvalue; // Second part (i.e. subvalue)
            TMaybe<TString> Value;   // Final value (i.e. "key=subvalue":"value"). Usually 0 or 1.
        };
        // Map of normal experiments (key:value)
        THashMap<TString, TMaybe<TString>> Experiments_;
        // Map of experiments with subvalue (key=value:1)
        TVector<TExpWithSubvalues> SubvalExperiments_;
    };

    const TFlags& Flags() const {
        return Flags_;
    }

    //
    // Input
    //
    class TInput: public NNonCopyable::TNonCopyable {
    friend class TRequest;
    public:
        ~TInput();

        enum class EInputType {
            Unknown,
            Text,
            Voice,
            Image,
            Music,
            Callback,
            TypedCallback
        };
        // Get type of user input
        EInputType GetInputType() const {
            return InputType_;
        }

        ELanguage GetUserLanguage() const {
            return UserLang_;
        }
        // Return either text or voice utterance to scenario
        // For music, image and callback input types return empty string
        const TString& GetUtterance() const {
            return Utterance_;
        }

        //
        // Working with semantic frames (direct access, OBSOLETE!!!)
        //
        /*
            Find and create new semantic frame
            Note frame pointer will be alive till end of scenario call
            @params: [IN] frameName - semantic frame name
            @return pointer to old style frame
        */
        bool HasSemanticFrame(TStringBuf frameName) const;
        const NHollywood::TPtrWrapper<NHollywood::TFrame> FindSemanticFrame(TStringBuf frameName) const;

        // Working with semantic frames (simplified access)
        const NScenarios::TInput& GetInputProto() const {
            return InputProto_;
        }

        //
        // Working with typed semantic frames
        //
        /*
            Enumerate all semantic frames and find a TSF of given type
            @param [IN] frameName
                   [OUT] proto
            @return false, if TSF is not found

            Note: you may pass TSF from dispatcher directly to scene selector using following code:
            TProto1 proto1;
            if (request.Input().FindTSF("name1"), proto1)
                return TReturnValueScene<proto1>(&MyScene1::Main);
            TProto2 proto2;
            if (request.Input().FindTSF("name2"), proto2)
                return TReturnValueScene<proto2>(&MyScene2::Main);
        */
        template <class TTsfProto>
        bool FindTSF(TStringBuf frameName, TTsfProto& proto) const {
            return FindTSFImpl(frameName, proto);
        }
        // New version of FindTSF() function. Can be used without frameName
        template <class TTsfProto>
        bool FindTSF(TTsfProto& proto) const {
            return FindTSFImpl(proto);
        }

        //
        // Working with callback directives
        //
        const NHollywood::TPtrWrapper<NScenarios::TCallbackDirective> FindCallback() const;

    private:
        TInput(const NScenarios::TScenarioBaseRequest& baseRequest, const NScenarios::TInput& input, const NScenarios::TRequestMeta& requestMeta);
        bool FindTSFImpl(TStringBuf frameName, google::protobuf::Message& proto) const;
        bool FindTSFImpl(google::protobuf::Message& proto) const;
    private:
        ELanguage UserLang_;
        TString Utterance_;
        const NScenarios::TScenarioBaseRequest& BaseRequest_;
        const NScenarios::TInput& InputProto_;
        mutable TVector<NHollywood::TFrame*> Frames_;
        EInputType InputType_;
    };

    const TInput& Input() const {
        return Input_;
    }

    //
    // User
    // Contains information about user data
    //
    class TUser {
    friend class TRequest;
    public:
        // FiltrationMode - see https://a.yandex-team.ru/arc_vcs/alice/megamind/protos/scenarios/request.proto?rev=r9040723#L501
        enum class EFiltrationMode {
            NoFilter = 0,
            Moderate = 1,
            FamilySearch = 2,
            Safe = 3
        };
        EFiltrationMode GetFiltrationMode() const {
            return FiltrationMode_;
        }
        EContentSettings GetContentSettings() const {
            return ContentSettings_;
        }
        bool IsNoFilterMode() const {
            return FiltrationMode_ == EFiltrationMode::NoFilter;
        }
        bool IsModerateMode() const {
            return FiltrationMode_ == EFiltrationMode::Moderate;
        }
        bool IsFamilySearchMode() const {
            return FiltrationMode_ == EFiltrationMode::FamilySearch;
        }
        bool IsSafeMode() const {
            return FiltrationMode_ == EFiltrationMode::Safe;
        }
    private:
        TUser(const NScenarios::TScenarioBaseRequest& baseRequest, TRTLogger& logger);
    private:
        EFiltrationMode FiltrationMode_;
        EContentSettings ContentSettings_;
    };
    const TUser& User() const {
        return User_;
    }

    //
    // Render
    // Small helper to use render NLG functions in Main() node
    //
    class TNlgHelper {
    friend class TRequest;
    public:
        NNlg::TRenderPhraseResult RenderPhrase(TStringBuf nlgName, TStringBuf phraseName, const google::protobuf::Message& msgProto) const;
        NNlg::TRenderPhraseResult RenderPhrase(TStringBuf nlgName, TStringBuf phraseName, const NJson::TJsonValue& jsonContext) const;
        bool HasPhrase(TStringBuf nlgName, TStringBuf phraseName) const;

    private:
        TNlgHelper(const TRequest& request, TMaybe<NHollywood::TCompiledNlgComponent> nlg);
    private:
        const TRequest& Request_;
        const TMaybe<NHollywood::TCompiledNlgComponent> Nlg_;
    };

    const TNlgHelper& Nlg() const {
        return Nlg_;
    }

    //
    // AnalyticsInfo
    // Contains data to customize/fill standard and custom analytics info
    //
    class TAnalyticsInfo {
    friend class TRequest;
    public:
        ~TAnalyticsInfo();

        // Customize PSN for the specific request
        void OverrideProductScenarioName(TStringBuf productScenarioName) {
            Psn_ = productScenarioName;
        }
        // Working with intent
        void OverrideIntent(const TString& intent) {
            Intent_ = intent;
        }
        // Internal function, used to prevent default settings in case if user calls OverrideIntent() inside dispatcher
        void OverrideIntentIfEmpty(const TString& intent) {
            if (Intent_.Empty()) {
                Intent_ = intent;
            }
        }
        const TString& GetIntent() const {
            return Intent_;
        }
        // Override semantic frame for TScenarioResponseBody::SemanticFrame
        void OverrideResultSemanticFrame(const TString& sfName) {
            OutputFrameName_ = sfName;
        }
        // Add TAnalyticsInfo.TAction intoi AI
        void AddAction(NScenarios::TAnalyticsInfo_TAction&& action);
        void AddObject(NScenarios::TAnalyticsInfo_TObject&& object);
        void AddEvent(NScenarios::TAnalyticsInfo_TEvent&& event);

        // Internal function
        void BuildAnswer(NScenarios::TScenarioResponseBody* response, NPrivate::TAICustomData& data) const;

    private:
        TAnalyticsInfo(const TProtoHwScene* protoHwScene);
        void ExportToProto(TProtoHwScene& hwSceneProto) const;

    private:
        TString Psn_;
        TString Intent_;
        TString OutputFrameName_;
        TVector<NScenarios::TAnalyticsInfo_TAction> Actions_;
        TVector<NScenarios::TAnalyticsInfo_TObject> Objects_;
        TVector<NScenarios::TAnalyticsInfo_TEvent> Events_;
    };
    // Get access to analitics info (note this access is mutable)
    TAnalyticsInfo& AI() const {
        return AI_;
    }

    //
    // Internal functions
    //
    void ExportToProto(TProtoHwScene& hwSceneProto) const;

private:
    // Old request sources data (deprecated, used only for compatibility)
    const NScenarios::TRequestMeta& RequestMeta_;
    const NAppHost::IServiceContext& ServiceCtx_;
    TSystem                 System_;
    TDebug                  Debug_;
    TClient                 Client_;
    TFlags                  Flags_;
    TInput                  Input_;
    TUser                   User_;
    TNlgHelper              Nlg_;
    mutable TAnalyticsInfo  AI_;
    TApphostNodeInfo        ApphostInfo_;
};

//
// Run Request
//
class TRunRequestImpl;
class TRunRequest final: public TRequest {
public:
    TRunRequest(const NScenarios::TRequestMeta& requestMeta,
                const NScenarios::TScenarioRunRequest& runRequest,
                const NAppHost::IServiceContext& serviceCtx,
                const TProtoHwScene* protoHwScene,
                NAlice::NHollywood::IGlobalContext& globalCtx,
                TRTLogger& logger,
                const TApphostNodeInfo& apphostNodeInfo,
                TMaybe<NHollywood::TCompiledNlgComponent> nlg);
    ~TRunRequest();

    const NScenarios::TScenarioRunRequest& GetRunRequest() const {
        return RunRequest_;
    }
    NAlice::TUserLocation GetUserLocation() const;
    const NScenarios::TDataSource* GetDataSource(NAlice::EDataSourceType sourceType, bool logError = true) const;
    // Get user identifier (UID)
    // Note this function requires datasources and can be available in Run-stage only!
    const TString GetPUID() const;
private:
    const NScenarios::TScenarioRunRequest& RunRequest_;
    const NAppHost::IServiceContext& ServiceCtx_;
    TRunRequestImpl* Impl_;
};

//
// Apply request
//
class TApplyRequest final: public TRequest {
public:
    TApplyRequest(const NScenarios::TRequestMeta& requestMeta,
                  const NScenarios::TScenarioApplyRequest& applyRequest,
                  const NAppHost::IServiceContext& serviceCtx,
                  const TProtoHwScene* protoHwScene,
                  NAlice::NHollywood::IGlobalContext& globalCtx,
                  TRTLogger& logger,
                  const TApphostNodeInfo& apphostNodeInfo,
                  TMaybe<NHollywood::TCompiledNlgComponent> nlg);
    const NScenarios::TScenarioApplyRequest& GetApplyRequest() const {
        return ApplyRequest_;
    }
    template <class TProto>
    bool GetArguments(TProto& proto) const {
        if (!Arguments_.Is<TProto>()) {
            return false;
        }
        return Arguments_.UnpackTo(&proto);
    }
private:
    const NScenarios::TScenarioApplyRequest& ApplyRequest_;
    google::protobuf::Any Arguments_;
};

//
// Continue request
//
class TContinueRequest final: public TRequest {
public:
    TContinueRequest(const NScenarios::TRequestMeta& requestMeta,
                     const NScenarios::TScenarioApplyRequest& continueRequest,
                     const NAppHost::IServiceContext& serviceCtx,
                     const TProtoHwScene* protoHwScene,
                     NAlice::NHollywood::IGlobalContext& globalCtx,
                     TRTLogger& logger,
                     const TApphostNodeInfo& apphostNodeInfo,
                     TMaybe<NHollywood::TCompiledNlgComponent> nlg);
    const NScenarios::TScenarioApplyRequest& GetContinueRequest() const {
        return ContinueRequest_;
    }
    template <class TProto>
    bool GetArguments(TProto& proto) const {
        if (!Arguments_.Is<TProto>()) {
            return false;
        }
        return Arguments_.UnpackTo(&proto);
    }
private:
    const NScenarios::TScenarioApplyRequest& ContinueRequest_;
    google::protobuf::Any Arguments_;
};

//
// Commit request
//
class TCommitRequest final: public TRequest {
public:
    TCommitRequest(const NScenarios::TRequestMeta& requestMeta,
                   const NScenarios::TScenarioApplyRequest& commitRequest,
                   const NAppHost::IServiceContext& serviceCtx,
                   const TProtoHwScene* protoHwScene,
                   NAlice::NHollywood::IGlobalContext& globalCtx,
                   TRTLogger& logger,
                   const TApphostNodeInfo& apphostNodeInfo,
                   TMaybe<NHollywood::TCompiledNlgComponent> nlg);
    const NScenarios::TScenarioApplyRequest& GetCommitRequest() const {
        return CommitRequest_;
    }
    template <class TProto>
    bool GetArguments(TProto& proto) const {
        if (!Arguments_.Is<TProto>()) {
            return false;
        }
        return Arguments_.UnpackTo(&proto);
    }
private:
    const NScenarios::TScenarioApplyRequest& CommitRequest_;
    google::protobuf::Any Arguments_;
};

} // namespace NAlice::NHollywoodFw
