#pragma once

#include <alice/bass/forms/special_location.h>
#include <alice/bass/forms/common/saved_address.h>
#include <alice/bass/forms/navigator/user_bookmarks.h>

#include <alice/bass/http_request.h>
#include <alice/bass/libs/analytics/analytics.h>
#include <alice/bass/libs/analytics/builder.h>
#include <alice/bass/libs/client/client_info.h>
#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/globalctx/fwd.h>
#include <alice/bass/libs/request/request.sc.h>
#include <alice/bass/libs/source_request/source_request.h>
#include <alice/bass/util/error.h>

#include <alice/library/blackbox/blackbox_http.h>
#include <alice/library/geo/user_location.h>
#include <alice/library/restriction_level/restriction_level.h>
#include <alice/library/util/rng.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/data/scenario/data.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/iterator/functools.h>
#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/semver/semver.h>

#include <util/generic/flags.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/noncopyable.h>
#include <util/generic/set.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/typetraits.h>
#include <util/generic/vector.h>
#include <util/random/fast.h>

#include <variant>

struct TAvatar;

namespace NAlice {
class TAnalyticsTrackingModule;
class TTypedSemanticFrame;

namespace NScenarios {
class TMementoChangeUserObjectsDirective;
class TStackEngine;
} // namespace NScenarios

} // namespace NAlice

namespace NBASS {
class TContext;
} // namespace NBASS

namespace NTestingHelpers {
bool IsAttentionInContext(NBASS::TContext& ctx, TStringBuf attentionType);
} // namespace NTestingHelpers

namespace NBASS {

//BE CAREFUL! Make sure all scale factors are supported by avatars.mds!
const TVector<float> ALLOWED_SCREEN_SCALE_FACTORS_DEFAULT = { 1, 1.5, 2, 3, 3.5, 4 };
const TVector<float> ALLOWED_SCREEN_SCALE_FACTORS_LARGE_IMAGES = { 1, 1.5, 2, 3 };

constexpr TStringBuf UNIPROXY_ACTION_UPDATE_DATASYNC = "update_datasync";

using NAlice::EContentRestrictionLevel;

constexpr int BEGEMOT_EXTERNAL_MARKUP_TYPE = NAlice::EDataSourceType::BEGEMOT_EXTERNAL_MARKUP;
constexpr int BLACK_BOX_TYPE = NAlice::EDataSourceType::BLACK_BOX;
constexpr int ENVIRONMENT_STATE_TYPE = NAlice::EDataSourceType::ENVIRONMENT_STATE;
constexpr int TANDEM_ENVIRONMENT_STATE_TYPE = NAlice::EDataSourceType::TANDEM_ENVIRONMENT_STATE;
constexpr int WEB_SEARCH_DOCS_TYPE = NAlice::EDataSourceType::WEB_SEARCH_DOCS;

enum class EClientPermission {
    Location /* "location" */,
    PushNotifications /* "push_notifications" */,
    ScheduleExactAlarm /* "schedule_exact_alarm" */,
};

enum class EPermissionType {
    Permitted,
    Forbidden,
    Unknown
};

struct TTestContextParams;

struct TSlot {
    using TScheme      = NBASSRequest::TForm<TSchemeTraits>::TSlot;
    using TSchemeConst = NBASSRequest::TForm<TSchemeTraits>::TSlotConst;

    /** Construct slot from name and type (with Value as null and Optional as true).
     * @see Optional
     * @see Value
     * @param[in] name is a name for slot
     * @param[in] type is slot type
     */
    TSlot(TStringBuf name, TStringBuf type);

    /** Construct a fully initialized slot from scheme.
     * @param[in] a validated slot scheme (the consturctor itself does not validate scheme)
     */
    explicit TSlot(const TSchemeConst& scheme);

    /** Serialize slot to json.
     * { name: "some_name", type: "num", optional: false, value: 12345 }
     * no validation performed, for guaranteed valid slot json use TSlot::ToJson(TResultValue* result)
     * @return slot json as is (can be invalid)
     */
    NSc::TValue ToJson() const;

    /** Serialize slot to json.
     * { name: "some_name", type: "num", optional: false, value: 12345 }
     * @param[out] result set to the real error in case the slot is invalid (can be nullptr)
     * @return NSc::Null if slot is invalid otherwise a valid slot json
     */
    NSc::TValue ToJson(TResultValue* result) const;

    /** Serialize slot to json.
     * { name: "some_name", type: "num", optional: false, value: 12345 }
     * @param[out] out is an output json
     * @param[out] result set to the real error in case the slot is invalid
     * @return false if error (also set error) otherwise true
     */
    bool ToJson(NSc::TValue* out, TResultValue* result) const;

    /** Reset slot state:
     * set null Value,
     * empty SourceText,
     * true Optional
     */
    void Reset();

    /** Name of slot. It is a mandatory field.
     */
    TString Name;
    /** Type of slot, could be 'num', 'string', etc...
     * It is a mandatory field to serizalize to json.
     */
    TString Type;

    /** Original user text captured while parsing the slot (optional).
     * May be string or array of strings
     */
    NSc::TValue SourceText;

    /** Set false if the slot is mandatory for operation.
     * The field is mandatory and default value is true.
     */
    bool Optional;

    /** A json value of the slot, can be null (so, the default value is also null)
     */
    NSc::TValue Value;
};

class TDiv2BlockBuilder {
public:
    enum class ETextSource {
        None,
        Bass,
        VinsNlg,
    };

public:
    TDiv2BlockBuilder(NSc::TValue preRenderedCard, bool hideBorders);
    TDiv2BlockBuilder(const TString& cardTemplateName, NSc::TValue data, bool hideBorders);

    TDiv2BlockBuilder& UseTemplate(const TString& templateName);
    TDiv2BlockBuilder& UseTemplate(const TString& templateName, NSc::TValue preRenderedTemplate);
    TDiv2BlockBuilder& SetHideBorders(bool hideBorders);
    TDiv2BlockBuilder& SetText(const TString& text);
    TDiv2BlockBuilder& SetTextFromNlgPhrase(const TString& type);

    void ToBlock(NSc::TValue& block) &&;

private:
    TMaybe<TString> CardName_;
    NSc::TValue Data_;

    TVector<TString> TemplateNames_;
    NSc::TValue PreRenderedTemplates_;

    ETextSource TextSource_ = ETextSource::None;
    TString Text_;
    bool HideBorders_ = false;
};

struct TInputAction {
    using TActionConst = NBASSRequest::TRequest<TSchemeTraits>::TActionConst;

    /** Construct a fully initialized slot from scheme.
     * @param[in] a validated slot scheme (the consturctor itself does not validate scheme)
     */
    explicit TInputAction(const TActionConst& scheme);

    /** Name of the action, required
     */
    TString Name;

    /** A json value of the action data, optional
     */
    NSc::TValue Data;
};

class TContext : public TThrRefBase {
public:
    Y_HAS_MEMBER(Log, SetFormIndex);
public:
    /** Json output options for ToJson()
     * @see ToJson()
     */
    enum class EJsonOut {
        Resubmit = 1ULL << 1,   /// add resubmit flag to form
        FormUpdate = 1ULL << 2, /// change name of from 'form' to 'form_update'
        TopLevel = 1ULL << 3,   /// do not create form/form_update dict, just put all the data into top level json (it
                                /// cancels the flag FormUpdate)
        ServerAction = 1ULL << 4, /// creates json as server command
        DataSources = 1ULL << 5,  /// Saves data sources
    };

    Y_DECLARE_FLAGS(TJsonOut, EJsonOut)

    struct TLocale {
        TString Lang;
        TString FullLocale;
        const TString& ToString() const { return FullLocale; }
    };

    struct TInitializer {
        explicit TInitializer(const TContext& ctx);
        TInitializer(TGlobalContextPtr globalCtx, TStringBuf reqId, TStringBuf marketReqId, TStringBuf authHeader,
                     TStringBuf appInfoHeader, TStringBuf fakeTimeHeader, TMaybe<TString> userTicketHeader,
                     TMaybe<NAlice::TEvent>&& speechKitEvent);
        TInitializer(TGlobalContextPtr globalCtx, TStringBuf reqId, TStringBuf authHeader,
                     TStringBuf appInfoHeader, TStringBuf fakeTimeHeader, TMaybe<TString> userTicketHeader,
                     TMaybe<NAlice::TEvent>&& speechKitEvent)
            : TInitializer(globalCtx, reqId, Default<TStringBuf>() /*marketReqId*/, authHeader,
                           appInfoHeader, fakeTimeHeader, std::move(userTicketHeader),
                           std::move(speechKitEvent))
        {
        };

        TGlobalContextPtr GlobalCtx;
        TString ReqId;
        TString MarketReqId;
        TString AuthHeader;
        TString AppInfoHeader;
        TString FakeTimeHeader;
        TMaybe<TString> UserTicketHeader;
        NSc::TValue ConfigPatch;
        /// Form identifier. It is needed for logging.
        TString Id{};
        TMaybe<NAlice::TEvent> SpeechKitEvent;
    };

    /** An alias for smartpointer to Context.
     * It is needed to refcount in case we handle everything async.
     */
    using TPtr = TIntrusivePtr<TContext>;

    // For backward compatibility. Please use NBASS::TSlot
    using TSlot            = NBASS::TSlot;
    using TSlotSchemeConst = TSlot::TSchemeConst;

    using TBlock        = NSc::TValue;

    using TMeta         = NBASSRequest::TMetaConst<TSchemeTraits>;
    using TDeviceState  = TMeta::TDeviceStateConst;
    using TScheme       = NBASSRequest::TRequest<TSchemeTraits>;
    using TSessionState = TScheme::TSessionState;

    using TDataSource   = NSc::TValue;
    using TDataSources  = THashMap<int, TDataSource>;

public:
    TContext(const TContext& parentContext, const TStringBuf formName);
    ~TContext() override; // TThrRefBase

    TContext::TPtr Clone() const;

    virtual void AddSourceResponse(TStringBuf /*name*/, NHttpFetcher::TResponse::TRef /*response*/) {
    }

    virtual void AddSourceFactorsData(TStringBuf /*source*/, const NSc::TValue& /*data*/) {
    }

    /** A requested form from request!
     */
    const TString& FormName() const {
        return FName;
    }

    /** The form name kept after form change.
     */
    const TString& ParentFormName() const {
        return ParentFName;
    }

    const TString& OriginalFormName() const {
        return OriginalFName;
    }

    TStringBuf FormId() const {
        return FId;
    }

    bool HasForm() const {
        return !FName.empty();
    }

    /** Action from request (if present)
     */
    const TMaybe<TInputAction>& InputAction() const {
        return InputActionParam;
    }

    bool HasInputAction() const {
        return InputActionParam.Defined();
    }

    /** Get config for this request
     * it may be patched with meta params
     */
    const TConfig& GetConfig() const;

    TSourcesRequestFactory GetSources() const;

    /** Create Context from json request.
     * @param[in] request a source json
     * @param[in] initData is an initialization data
     * @param[out] context is a newly created context
     * @return error object in case there were errors or empty maybe in case of success
     */
    static TResultValue FromJson(const NSc::TValue& request, TInitializer& initData, TPtr* context);
    /** Same as TContext::FromJson, but does not validate input data. Use it only for deserialization of safe data.
     */
    static TResultValue FromJsonUnsafe(const NSc::TValue& request, TInitializer& initData, TPtr* context);

    /** Returns slot by name and type.
     * @param[in] name of slot to search by
     * @param[in] type of slot to search by (if type is empty, search performs by name only)
     * @return null in case slot with name and type not found or pointer to the existed one
     */
    TSlot* GetSlot(TStringBuf name, TStringBuf type = TStringBuf());
    const TSlot* GetSlot(TStringBuf name, TStringBuf type = TStringBuf()) const;
    TVector<TSlot*> GetSlots() const;

    /** Create a new or overwrite/reset existed slot.
     * This it not thread safe.
     * @param[in] name is a slot name for overwriting/creating (also is used to search slot)
     * @param[in] type is a slot type for overwriting/creating
     * @param[in] optional is a state for a new slot
     * @param[in] value is a value for a new slot
     * @param[in] sourceText is a source (raw) text for a new slot
     * @return a poiner to a newly created or overwritten slot
     */
    TSlot* CreateSlot(TStringBuf name, TStringBuf type, bool optional = true, const NSc::TValue& value = NSc::Null(),
                      const NSc::TValue& sourceText = NSc::Null());

    /** Create a new slot with name and type in case no slots found with given name, otherwise return the existed one
     * (by name only). If a new slot is created param TSlot::Value sets to null and param TSlot::Optional sets to true
     * the same as in TSlot constructor.
     * @see TSlot::TSlot(TStringBuf, TStringBuf)
     * Not thread safe.
     * @param[in] name of slot for search/create
     * @param[in] type of slot for creation only
     */
    TSlot* GetOrCreateSlot(TStringBuf name, TStringBuf type);

    /** Delete slot with given name.
     * Not thread safe
     * @param[in] name of slot to delete
     * @return true if slot was deleted from both SlotList and SlotName
     */
    bool DeleteSlot(TStringBuf name);

    /** Creates a new block and return pointer to it.
     * This is not thread safe.
     * @return pointer to a newly created block
     */
    TBlock* Block();

    const TDataSources& DataSources() const {
        return ReqDataSources;
    }

    const TMeta& Meta() const;

    const TLocale& MetaLocale() const {
        return Locale;
    }

    const TClientInfo& MetaClientInfo() const {
        return ClientInfo;
    }

    const TClientFeatures& ClientFeatures() const {
        return Features;
    }

    const TSessionState& SessionState() const {
        return RequestSessionStateScheme;
    }

    TSessionState& SessionState() {
        return RequestSessionStateScheme;
    }

    /** Returns matching screen_scale_factor from allowedScales array
     * BE CAREFUL! Make sure all scale factors are supported by avatars.mds!
     * @param ascending list of screen_scale_factor values to select from
     * @return matching screen_scale_factor
     */
    float MatchScreenScaleFactor(TConstArrayRef<float> allowedScaleFactors) const {
        const float desiredScale = Meta().ScreenScaleFactor() - std::numeric_limits<float>::epsilon();
        for (float s: allowedScaleFactors) {
            if (s >= desiredScale) {
                return s;
            }
        }
        return allowedScaleFactors.back();
    }

    /** Adds new block with type suggest and fill in the defaults.
     * @param[in] type is a suggest type
     * @param[in] data is a json for data in suggest block
     * @param[in] formUpdate is a json for form_update in suggest block
     * @return a newly created suggest block
     */
    TBlock* AddSuggest(TStringBuf type, NSc::TValue data = NSc::Null(), NSc::TValue formUpdate = NSc::Null());

    /** Remove suggest block with given type.
     * @param[in] type suggest type
     * @return removed element or NSc::Null()
     */
    NSc::TValue DeleteSuggest(TStringBuf type);

    /** Add suggest for search fallback. VINS initiate search scenario
     * when user clicks on such suggest.
     * @param query custom search text
     */
    void AddSearchSuggest(TStringBuf query = TStringBuf());

    /** Add suggest for onboarding scenario "what can you do".
     * @return a newly create suggest block
     */
    void AddOnboardingSuggest();

    /** Add block describing a frame action
     * @return a newly created "frame_action" block
     */
    void AddFrameActionBlock(const TString& actionId, const NAlice::NScenarios::TFrameAction& frameAction);

    /** Add block describing a scenario data
     * @return a newly created "scenario_data" block
     */
    void AddScenarioDataBlock(const NAlice::NData::TScenarioData& scenarioData);

    /** Adding block from json value.
    * Needed to parse TContext from json.
    */
    void AddBlock(NSc::TValue value);

    /** Returns true if context has any block of specified type.
     * @return true if context has any block of specified type
     */
    bool HasAnyBlockOfType(TStringBuf type) const;

    /** Adds new block with type attention and fill in the defaults.
     * @param[in] type is a attention type
     * @param[in] data is a json for data in attention block
     * @return a new created attention block
     */
    TBlock* AddAttention(TStringBuf type, NSc::TValue data);

    TBlock* AddAttention(TStringBuf type) {
        return AddAttention(type, NSc::Null());
    }

    TBlock* AddIrrelevantAttention(NSc::TValue data = NSc::Null()) {
        return AddAttention("irrelevant", std::move(data));
    }

    TBlock* AddIrrelevantAttention(const TStringBuf relevantIntent, const TStringBuf reason = {}) {
        NSc::TValue payload{};
        payload.SetDict();
        payload[TStringBuf("relevant_intent")] = relevantIntent;
        if (!reason.empty()) {
            payload[TStringBuf("reason")] = reason;
        }
        return AddAttention("irrelevant", std::move(payload));
    }

    /** Adds new block with type attention and fill in the defaults. Also update metrics counter
     * @param[in] type is a attention type
     * @param[in] data is a json for data in attention block
     * @return a new created attention block
     */
    TBlock* AddCountedAttention(TStringBuf type, NSc::TValue data);

    TBlock* AddCountedAttention(TStringBuf type) {
        return AddCountedAttention(type, NSc::Null());
    }

    /** Remove attention block with given type.
     * @param[in] type attention type
     * @return removed element or NSc::Null()
     */
    NSc::TValue DeleteAttention(TStringBuf type);

    /** Checks if an attention with a given type exists in the current context.
     * @param[in] type is an attention type
     * @return true if the context contains an attention of a given type, false otherwise.
     */
    bool HasAttention(TStringBuf type) const;

    /** Checks if an attention with one of the given typea exists in the current context.
     * @param[in] types is a set of attention types
     * @return true if the context contains an attention of one of the given types, false otherwise.
     */
    bool HasAnyAttention(const TSet<TStringBuf>& types) const;

    /** Add server action block.
     * @param[in] type server action type (i.e. update_form)
     * @param[in] data is a server action data (i.e. context.ToJson(...))
     * @return a newly created server action block
     */
    TBlock* AddServerAction(TStringBuf type, NSc::TValue data);

    TBlock* AddMementoUpdateBlock(NAlice::NScenarios::TMementoChangeUserObjectsDirective&& directive);

    TBlock* AddRawServerDirective(NAlice::NScenarios::TServerDirective&& directive);

    TBlock* AddTypedSemanticFrame(const NAlice::TTypedSemanticFrame& tsf, const NAlice::TAnalyticsTrackingModule& atm);

    TBlock* AddStackEngine(const NAlice::NScenarios::TStackEngine& stackEngine);

    /** Adds command block and fill in the defaults. Read more in README.md about TDirective
     * @param[in] type is a attention type
     * @param[in] data is a json for data in attention block
     * @param[in] beforeTts is a flag to play tts after this block
     * @return a new created command block
     */
    template<class TDirective>
    TBlock* AddCommand(TStringBuf type, NSc::TValue data, bool beforeTts = false) {
        const auto analyticsTag = TDirectiveFactory::Get()->GetAnalyticsTag<TDirective>();
        return AddCommandImpl("command" /* type */, type, analyticsTag, data, beforeTts);
    }

    /** Adds command block and fill in the defaults.
     * @param[in] type is a attention type
     * @param[in] analyticsDirective is a data for analytics. Read more in README.md
     * @param[in] data is a json for data in attention block
     * @return a new created command block
     */
    TBlock* AddCommand(TStringBuf type, const TDirectiveFactory::TDirectiveIndex& analyticsDirective, NSc::TValue data);

    /** Adds uniproxy-action block and fills in the defaults.
     * @param[in] type is a action type
     * @param[in] data is a json for data in action block
     * @return a new created action block
     */
    TBlock* AddUniProxyAction(TStringBuf type, NSc::TValue data);

    /** Get attention block with given type.
     * @param[in] type attention type
     * @return existing block or null
     */
    TBlock* GetAttention(TStringBuf type);

    /** Get command block with given type.
     * @param[in] type command type
     * @return existing block or null
     */
    TBlock* GetCommand(TStringBuf type);

    /** Remove command block with given type.
     * @param[in] type command type
     * @return removed element or NSc::Null()
     */
    NSc::TValue DeleteCommand(TStringBuf type);

    /** Adds new block with type silent_reponse
     * @param[in] data is a json for data in this block
     * @return a new created block
     */
    TBlock* AddSilentResponse(NSc::TValue data);

    TBlock* AddSilentResponse() {
        return AddSilentResponse(NSc::Null());
    }

    /** Adds Div-card block with the <code>data</code>.
     * @param[in] type is a card template
     * @param[in] data is a data
     * @return a new created div-card block
     */
    TBlock* AddDivCardBlock(TStringBuf type, NSc::TValue data);

    /** Adds Div2-card block.
     * @param[in] builder is div2 card builder (don't forget std::move()).
     * @return a new created div2-card block
     */
    TBlock* AddDiv2CardBlock(TDiv2BlockBuilder builder);

    template <typename... TArgs>
    TBlock* AddDiv2CardBlock(TArgs&& ...args) {
        return AddDiv2CardBlock(TDiv2BlockBuilder{std::forward<TArgs>(args)...});
    }

    /** Adds Div-card block with the <code>data</code>.
     * @param[in] data card payload
     * @return a new created div-card block
     */
    TBlock* AddPreRenderedDivCardBlock(NSc::TValue data);

    /** Adds Div-card block with text only message.
     * @param[in] id is a text card phrase_id
     * @param[in] data is an additional data for text card rendering
     * @return a new created div-card block
     */
    TBlock* AddTextCardBlock(TStringBuf id, NSc::TValue data = NSc::TValue());

    /** Add block which requests vins to stop listening.
     */
    TBlock* AddStopListeningBlock();

    /** Add new block (or replaces existing block's 'data') with client features info (see client_features.cpp for details)
     */
    TBlock* AddClientFeaturesBlock();

    /** Add block with special button customization
     * @param[in] type is a button type
     * @param[in] data is a json for special button customization
     */
    TBlock* AddSpecialButtonBlock(TStringBuf type, NSc::TValue data);

    TBlock* AddAutoactionDelayMsBlock(int delayMs);

    /** Add block with video text similarity factors
     * @param[in] data is a json with encoded factors
     */
    TBlock* AddVideoFactorsBlock(NSc::TValue data);

    /** Create and push a new error block.
     * @param[in] error is an error description
     * @return a newly created block
     */
    TBlock* AddErrorBlock(const TError& error, const NSc::TValue& data);
    TBlock* AddErrorBlock(const TError& error);
    TBlock* AddErrorBlock(TError::EType type);
    TBlock* AddErrorBlock(TError::EType type, TStringBuf msg);
    TBlock* AddErrorBlockWithCode(const TError& error, TStringBuf code);
    TBlock* AddErrorBlockWithCode(TError::EType type, TStringBuf code);

    /** Returns true if context has any block with 'error' type
     * @return true if context has any block with 'error' type
     */
    bool HasAnyErrorBlock() const;

    /** Add block with commit stage arguments.
     * @param[in] data is a json with a serialized continuation.
     */
    TBlock* AddCommitCandidateBlock(NSc::TValue data);

    /** Remove error block with given type.
     * @param[in] types error types
     * @return removed elements
     */
    TVector<NSc::TValue> DeleteErrorBlocks(const TVector<TError::EType>& types);

    /** Get stats block.
     * @return existing stats block (creates it if necessary)
     */
    TBlock* GetStats();

    /** Adds new block with type stats and fill in the defaults.
     * If block already exists, it appends new data to the existing block.
     * @param[in] name is a name of stats param
     * @param[in] value is a value of stats param
     * @return a new created or exiting stats block
     */
    TBlock* AddStatsCounter(TStringBuf name, i64 value);

    TBlock* AddPlayerFeaturesBlock(const bool restorePlayer, const ui64 lastPlayTimestampMillis);

    /** Returns the slots' list of specified type.
     * @param[in] type of slots you want to find
     */
    TVector<TSlot*> Slots(TStringBuf type);

    /** Serialize context to json as described in ASSISTANT-12.
     * { form: { slots: [], name: ... }, blocks: [] }
     * @param[out] out is a destination json
     * @param[in] flags are params which changed an output json (@see EJsonOut)
     */
    void ToJson(NSc::TValue* out, TJsonOut flags = TJsonOut(0)) const;

    /** Serialize context to json as described in ASSISTANT-12.
     * { form: { slots: [], name: ... }, blocks: [] }
     * @param[in] flags are params which changed an output json (@see EJsonOut)
     * @return a serialized context
     */
    NSc::TValue ToJson(TJsonOut flags = TJsonOut(0)) const;

    /** Serialize context to json as described in ASSISTANT-12, ignoring response form if it is set.
     * { form: { slots: [], name: ... }, blocks: [] }
     * @param[in] flags are params which changed an output json (@see EJsonOut)
     * @return a serialized context
     */
    NSc::TValue TopLevelToJson(TJsonOut flags = TJsonOut(0)) const;

    /** Make a default request to a reqwizard with action 'markup', UserRegion as lr and given text.
     * The same a ReqWizard(text, UserRegion(), "markup");
     * @param[in] text is a request
     * @return reqwizard response
     */
    const NSc::TValue& ReqWizard(TStringBuf text) {
        return ReqWizard(text, UserRegion(), {{"action", "markup"}});
    }

    /** Make (and cache) a request to reqwizard with given text (query), lr (user region) and custom cgi parameters;
     * @param[in] text is a request
     * @param[in] lr is a user region which is going to pass to request to reqwizard
     * @param[in] cgi is an additional cgi params
     * @param[in] wizclient is shard of wizard
     * @return reqwizard response
     */
    const NSc::TValue& ReqWizard(TStringBuf text, NGeobase::TId lr, TCgiParameters cgi, TStringBuf wizclient = TStringBuf("assistant.yandex"));

    /** For backward compatibility with bass and megamind
     * @return TUserLocation: object with UserRegion, UserTimeZone, UserTld
     */
    const NAlice::TUserLocation& UserLocation();
    const TMaybe<NAlice::TUserLocation>& UserLocation() const;

    /** It goes to the geocoder with given (in meta.location) lan/lot
     * and returns a region (also save it in the context).
     * @return a region which is returned from geocoder or NGeobase::UNKNOWN_REGION in case of any error
     */
    NGeobase::TId UserRegion();

    /** It uses a user region (see UserRegion method)
     * and returns corresponding timezone from Geobase (if it exists and only one).
     * If user location is unknown, it takes timezone from meta.timezone.
     * @return a name of timezone in format "Europe/Moscow" or empty string.
     */
    TString UserTimeZone();

    /** Detects user's tld from context data (uses meta.tld if it exists, or detects tld from meta.location)
     * NOTE: it may call UserRegion, so it may be a request to geocoder
     * @return tld as string
     */
    const TString& UserTld();

    /** Returns authorization header which was recieved with request
     * @return authirization header in 'OAuth oauth_token' format or empty string
     */
    const TString& UserAuthorizationHeader() const;

    /** Returns app info header which was recieved with request
     * @return app info header or empty string
     */
    const TString& GetAppInfoHeader() const;

    /** Returns true if authorization header was received with request
     */
    bool IsAuthorizedUser() const;

    /** Returns true if the request is from test user.
     */
    bool IsTestUser() const;

    /** Returns IP address which was recieved with request
     * @return IP as a string
     */
    const TString& UserIP() const;

    /** Some smart speakers send device id in the wrong field.
     * This function returns the correct one
     * @return device id as a string
     */
    TString GetDeviceId() const;

    /** Some smart speakers send wrong device model.
     * This function returns the correct one
     * @return device model as a string
     */
    TString GetDeviceModel() const;

    /** Return named address saved for user.
     */
    TSavedAddress GetSavedAddress(TSpecialLocation name, TStringBuf searchText = "");

    /** Remove named address saved for user from storage.
     * Method added for debug purposes
     */
    TResultValue DeleteSavedAddress(TSpecialLocation name);

    /** Save named address for user.
     */
    TResultValue SaveAddress(TSpecialLocation name, const NSc::TValue& addr);

    /** Get user bookmarks helper (maintaining bookmarks from Navigator device state)
     * will init bookmaks helper if needed
     * @return bookmarks helper
     */
    TMaybe<TUserBookmarksHelper> GetUserBookmarksHelper();

    /** Get analytics info builder
     * will init analytics info builder if needed
     * @return analytics info builder
     */
    NAlice::NScenarios::TBassAnalyticsInfoBuilder& GetAnalyticsInfoBuilder();

    /** Check if meta experiments has given name.
     * @return true if the name exists no matter if it has value or not ("experiments": { "name1": "" } - returns true)
     */
    bool HasExpFlag(TStringBuf name) const;

    /** Return value of the given experiment flag.
     * @param[in] name is a name of experiment flag
     * @return maybe is defined if the given name exists within flags and its value (can be empty)
     */
    TMaybe<TString> ExpFlag(TStringBuf name) const;

    bool IsIntentForbiddenByExperiments(TStringBuf intentName) const {
        return ForbiddenIntents.contains(intentName);
    }

    /** Calls fn to each key from Experiments
     * @param[in] fn function that takes TStringBuf
     */
    void OnEachExpFlag(const std::function<void(TStringBuf)>& fn) const;

    NAlice::TRawExpFlags ExpFlags() const;

    /** Return suffix of expFlag for given prefix (return last met if there are many)
     * @param[in] expPrefix is a prefix of experiment flag
     * @return maybe is defined if the given prefix exists within flags and its suffix (can be empty)
    */
    TMaybe<TStringBuf> GetValueFromExpPrefix(const TStringBuf expPrefix) const {
        TMaybe<TStringBuf> result;
        OnEachExpFlag(
            [&](auto expFlag) {
                if (expFlag.StartsWith(expPrefix)) {
                    result = expFlag.SubStr(expPrefix.size());
                }
            });
        return result;
    }

    /** Return request id
     */
    const TString& ReqId() const {
        return RequestId;
    }

    const TString& MarketReqId() const {
        return MarketRequestId;
    }

    const TMaybe<TString>& UserTicket() const {
        return UserTicketHeader;
    }

    const TString& GetRngSeed() const {
        return RngSeed_;
    }

    NAlice::IRng& GetRng() {
        return *Rng.get();
    }

    const TMaybe<NAlice::TEvent>& GetSpeechKitEvent() const {
        return SpeechKitEvent;
    }

    /** Updates ReqInfo in accordance with ReqId and HypothesysNumber
     */
    void UpdateLoggingReqInfo() const;

    /** Returns true if auth token for provider is set
     */
    bool IsProviderTokenSet(TStringBuf name) const;

    /** Sets auth token for the provider
     */
    void SetProviderToken(TStringBuf name, TMaybe<TString> token);

    /** Returns auth token which was set for the provider
     */
    const TMaybe<TString> GetProviderToken(TStringBuf name) const;

    TMaybe<NAlice::TBlackBoxHttpFetcher>& GetBlackBoxRequest();
    void SetBlackBoxRequest(NAlice::TBlackBoxHttpFetcher&& request);

    /** Return current content restriction level
     * For backward compatibility with bass and megamind
     */
    NAlice::EContentSettings ContentRestrictionLevel() const {
        return ContentRestrictionLevel_;
    }

    bool GetIsClassifiedAsChildRequest() const {
        return IsClassifiedAsChildRequest_;
    }

    /** Return current content restriction level
     */
    EContentRestrictionLevel GetContentRestrictionLevel() const;

    /** Changes context to return new form instead of current in response.
     * Current form will be saved in callback_form slot if setCurrentFormAsCallback is true.
     * Also create and return context for new form.
     * @param[in] newFormName is a new response form name
     * @param[in] setCurrentFormAsCallback is a callback_form slot creation condition
     * @return new response form context
     */
    TContext::TPtr SetResponseForm(TStringBuf newFormName, bool setCurrentFormAsCallback);

    /** Copy specified slots from given context object into this one.
     * If a slot not found it silently not copied!
     * @param[in] ctx is a given context from which slots will be copied
     * @param[in] slotNames is a list of slot names which you would like to copy
     */
    void CopySlotsFrom(const TContext& ctx, std::initializer_list<TStringBuf> slotNames);

    void CopySlotFrom(const TContext& ctx, TStringBuf slotName) {
        CopySlotsFrom(ctx, {slotName});
    }

    auto GetBlocks() const {
        return NFuncTools::Map([](const std::unique_ptr<TBlock>& block) { return std::cref(*block); }, BlockList);
    }

    /** Computed goto of BASS, avoid using at any cost!
     * Change context to return callback form (if any) instead of current in response.
     * Will not add callback_form slot with current form to response
     * Also create and return context for new form.
     * @return callback response form context, or nullptr if form changes are disabled.
     */
    TContext::TPtr SetCallbackAsResponseForm();

    /** Computed goto of BASS, avoid using at any cost!
     */
    TContext::TPtr SetCallbackAsResponseFormAndCopySlots();

    /** Return response form if any.
     * @return response form context
     */
    TContext::TPtr GetResponseForm() {
        return ResponseForm;
    }

    /** Runs handler for ResponseForm if any
     * @return handler result or error if ResponseForm was not specified
     */
    TResultValue RunResponseFormHandler();

    /** Returns true if client supports skill styles
     * TODO move this check to client_info.h after ASSISTANT-2320
     */
    bool ShouldAddSkillStyle() const {
        return ClientFeatures().SupportsMultiTabs() || MetaClientInfo().IsElariWatch();
    }

    /** Search for the avatar's url in given namespace and name
     * with respect of scale factor from context.
     * By default scale factor is 1.
     * @param[in] ns is a namespace (not from avatarnica, but internal one)
     * @param[in] name is a name of picture in the given namespace
     * @param[in] suffix is a picture file suffix (default is ".png")
     * @return nullptr if avatar not found, otherwise a valid pointer to to avatar
     */
    const TAvatar* Avatar(TStringBuf ns, TStringBuf name, TStringBuf suffux = TStringBuf(".png")) const;

    /** Get information about whether a feature is permitted on the client.
     */
    EPermissionType GetPermissionInfo(EClientPermission permission) const;

    TGlobalContextPtr::TValueType& GlobalCtx() {
        return *GlobalContext;
    }

    const TGlobalContextPtr::TValueType& GlobalCtx() const {
        return *GlobalContext;
    }

    const NHttpFetcher::TResponse::TConstRef FindSetupResponse(const TString& name) const {
        if (auto ptr = SetupResponses.FindPtr(name)) {
            return *ptr;
        }
        return nullptr;
    }

    TInstant GetRequestStartTime() const {
        return RequestStartTime;
    }

    TFastRng<ui32> GetRandGeneratorInitializedWithEpoch() {
        return Meta().HasEpoch() ? Meta().Epoch() : 0;
    }

    TInstant Now() const {
        return FakeTime.GetOrElse(TInstant::Now());
    }

    TInstant ServerTimeMs() const {
        return TInstant::MilliSeconds(Meta().ServerTimeMs());
    }

    void MarkSensitive();
    void MarkSensitive(const TSlot& slot);

    void SendPushRequest(const TStringBuf service, const TStringBuf event, TMaybe<TStringBuf> yandexUid, NSc::TValue serviceData);

    void SetDontResubmit() {
        DontResubmit = true;
    }

    void SetIsMusicVinsRequest() {
        IsMusicVinsRequest = true;
    }

    bool GetIsMusicVinsRequest() {
        return IsMusicVinsRequest;
    }

protected:
    using TSlotList = TVector<std::unique_ptr<TSlot>>;
    using TSlotMap = TMap<TStringBuf, TSlot*>;
    using TSetupResponses = THashMap<TString, NHttpFetcher::TResponse::TRef>;

private:
    TBlock* AddCommandImpl(TStringBuf type, TStringBuf commandType, NSc::TValue data);
    TBlock* AddCommandImpl(TStringBuf type, TStringBuf commandType, TStringBuf analyticsType, NSc::TValue data, bool beforeTts = false);
    TBlock DeleteBlockImpl(TStringBuf type, TStringBuf subtype);
    TBlock* GetSensitiveBlock();

    NAlice::EContentSettings CalculateContentRestrictionLevel() const;
    bool IsClassifiedAsChildRequest() const;

    friend bool NTestingHelpers::IsAttentionInContext(TContext&, TStringBuf);

private:
    TGlobalContextPtr GlobalContext;
    const TString FName;
    const TString ParentFName;
    const TString OriginalFName;
    const TMaybe<TInputAction> InputActionParam;
    const std::unique_ptr<TConfig> Config;

    TSlotList SlotList;
    TSlotMap SlotMap;

    using TBlockList = TVector<std::unique_ptr<TBlock>>;
    TBlockList BlockList;
    TBlock* ClientFeaturesBlock;
    TBlock* StatsBlock;

    const NSc::TValue RequestMeta;
    TMeta RequestMetaScheme;

    const TClientFeatures Features;
    const TClientInfo ClientInfo;
    const TLocale Locale;
    TMap<TString, NSc::TValue> ReqWizardData;
    TDataSources ReqDataSources;
    THashSet<TString> ForbiddenIntents;

    TMaybe<NAlice::TUserLocation> UserLocation_;

    const TString AuthorizationHeader;
    const TString AppInfoHeader;
    const TString UserIPAddress;
    const TString RequestId;
    const TString MarketRequestId;
    const TMaybe<TString> UserTicketHeader;
    /// Is used for logging.
    const TString FId;

    // keeps auth tokens for created providers
    THashMap<TString, TMaybe<TString>> AuthTokens;

    TMaybe<NAlice::TBlackBoxHttpFetcher> BlackBoxRequest;

    // will save requested user addresses here
    using TAddressMap = TMap<TSpecialLocation, TSavedAddress>;
    TAddressMap UserAddresses;

    // user bookmarks from Navigator device state
    TMaybe<TUserBookmarksHelper> UserBookmarks;

    TMaybe<NAlice::NScenarios::TBassAnalyticsInfoBuilder> AnalyticsInfoBuilder;

    // If not null, contains form that should be returned instead of current one.
    TContext::TPtr ResponseForm;

    NSc::TValue RequestSessionState;
    TSessionState RequestSessionStateScheme;

    TSetupResponses SetupResponses;

    TBlock* SensitiveBlock = nullptr;

    THashMap<TStringBuf, bool> Permissions;

    TInstant RequestStartTime;

    TString RngSeed_;
    std::unique_ptr<NAlice::IRng> Rng;

    TMaybe<NAlice::TEvent> SpeechKitEvent;

    TMaybe<TInstant> FakeTime;

    bool IsClassifiedAsChildRequest_;
    NAlice::EContentSettings ContentRestrictionLevel_;

    bool DontResubmit = false;
    bool IsMusicVinsRequest = false;
    ui8 ChangeFormCounter = 0;

protected:
    TContext(TStringBuf formName, TMaybe<TInputAction>&& inputAction, TSlotList&& slots, const NSc::TValue& meta,
             TInitializer initData, const NSc::TValue& sessionState, TSetupResponses&& setupResponses,
             TMaybe<NSc::TValue> blocks, TDataSources&& dataSources, TStringBuf originalFormName,
             const TStringBuf parentFormName = TStringBuf("no_parent"));
    void ToJsonImpl(NSc::TValue* out, TJsonOut flags) const;

    using TConstructor = std::function<TResultValue(TStringBuf formName, TMaybe<TInputAction>&& inputAction,
                                                    TSlotList&& slots, const NSc::TValue& meta, TInitializer initData,
                                                    const NSc::TValue& sessionState, TSetupResponses&&,
                                                    TMaybe<NSc::TValue> blocks, TDataSources&& dataSources)>;
    static TResultValue FromJsonImpl(const NSc::TValue& request, TInitializer initData, TConstructor creator,
                                     bool shouldValidate);

    /** Serialize slots to json.
     * If <slotsKey> is empty then all the slots push into <dst>.
     * If <slotsKey> is not empty then all the slots push under this key in <dst>.
     */
    TResultValue SlotsToJson(NSc::TValue* dst, TStringBuf slotsKey = TStringBuf{}) const;

    /** Serialize blocks to json. If no blocks exist it doesn't create create blocks key in the destination json.
     */
    void BlocksToJson(NSc::TValue* formJson) const;
};


Y_DECLARE_OPERATORS_FOR_FLAGS(TContext::TJsonOut)

/** Check if slot pointer is null or value is null.
 */
bool IsSlotEmpty(const TContext::TSlot* slot);

} // namespace NBASS
