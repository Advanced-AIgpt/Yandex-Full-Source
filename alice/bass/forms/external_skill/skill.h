#pragma once

#include "error.h"

#include <alice/bass/forms/external_skill/proto/skill.pb.h>

#include <alice/bass/forms/context/fwd.h>

#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/source_request/source_request.h>

#include <library/cpp/monlib/metrics/labels.h>
#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/util/scheme_holder.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <alice/bass/forms/external_skill/scheme.sc.h>

namespace NUri {
class TUri;
} // namespace NUri

namespace NBASS {
namespace NExternalSkill {

inline constexpr TStringBuf PROCESS_EXTERNAL_SKILL = "personal_assistant.scenarios.external_skill";
inline constexpr TStringBuf PROCESS_EXTERNAL_SKILL_CONTINUE = "personal_assistant.scenarios.external_skill__continue";
inline constexpr TStringBuf PROCESS_EXTERNAL_SKILL_DEACTIVATE = "personal_assistant.scenarios.external_skill__deactivate";
inline constexpr TStringBuf PROCESS_EXTERNAL_SKILL_ACTIVATE_ONLY = "personal_assistant.scenarios.external_skill__activate_only";

inline constexpr TStringBuf ACTION_SKILL_PURCHASE_COMPLETE = "external_skill__purchase_complete";
inline constexpr TStringBuf ACTION_SKILL_ACCOUNT_LINKING_COMPLETE = "external_skill__account_linking_complete";

inline constexpr TStringBuf IMAGE_TYPE_BIG = "one-x";
inline constexpr TStringBuf IMAGE_TYPE_ORIG = "orig";
inline constexpr TStringBuf IMAGE_TYPE_MOBILE_LOGO = "mobile-logo-x";
inline constexpr TStringBuf IMAGE_TYPE_SMALL = "menu-list-x";
inline constexpr TStringBuf IMAGE_TYPE_LOGO_FG_IMG = "logo-fg-image-x";
inline constexpr TStringBuf IMAGE_TYPE_LOGO_BG_IMG = "logo-bg-image-x";


inline constexpr TStringBuf AVATAR_NAMESPACE_SKILL_IMAGE = "dialogs-skill-card";
inline constexpr TStringBuf AVATAR_NAMESPACE_SKILL_LOGO = "dialogs";

// For internal skills' counters
const THashMap<TString, TString> INTERNAL_SKILLS = {
    {"c9ad9252-2425-448b-829e-20a438560839", "alice_test_skill"},
    {"bd7c3799-5947-41d0-b3d3-4a35de977111", "boltalka"},
    {"672f7477-d3f0-443d-9bd5-2487ab0b6a4c", "game_of_cities"},
    {"8262beae-e2be-4f4c-bbfc-8916b810f718", "market_present"},
    {"bf005220-a1ff-431b-83c2-2ab54a986ccc", "guess_the_city_by_photo"},

    {"96be710a-b9e9-4072-afdb-feadc8f634d2", "faq"},
    {"2f3c5214-bc3e-4bd2-9ae9-ff39d286f1ae", "interactive_fiction"},
    {"6a166c37-8756-46b3-88ce-e22ab3ccc3ef", "detroit_quest"},
    {"fcc7c2fc-2b28-4578-822a-9be1120ad82e", "chernovik_quest"},
    {"8197850d-9305-4f63-9104-6a5cea388f4a", "laowai"},
    {"b44605c9-5d7c-4ef1-b167-5b643c8fa912", "pushkin"},
    {"216de9f2-f9ff-4c17-8140-527181e6ae71", "scientists"},
    {"d8bbc8bc-ef09-4277-aedf-66ae0b2f779c", "dal"},
    {"90e53403-0da3-409f-b4dd-4cd7a516a393", "socrat"},
    {"96021ef8-ce16-4b68-828e-1033c8e66c3c", "zahoder"},
    {"e0daaf63-6071-4155-9a9a-d560b2696628", "ilf_petrov"},
    {"c5a2ed9f-88cf-4fea-8548-9bb859506e2f", "tolstoy"},
    {"5fe5410d-2d5b-4cc2-bceb-3e6f4b65a4e3", "ostrovsky"},

    {"ebdde2bc-485d-4602-a8ff-f252d3e619ca", "afisha_news_bot"},
    {"166d0296-b282-4c43-bc0a-e0e5b4e12121", "echo_news_bot"},
    {"d6073b1c-107e-4c40-a035-fd533740c114", "fontanka_news_bot"},
    {"34ac4293-e3e3-411b-a30c-9c4d96562346", "kanobu_news_bot"},
    {"34eaa318-5bb7-475a-b40d-3d5755c17f7a", "kommersant_news_bot"},
    {"f2c53a12-6178-41a4-8bd1-b0b11c5ffeb0", "meduza_news_bot"},
    {"a9c961ec-3fc0-44df-bf9d-42e1bee2809c", "novayagazeta_news_bot"},
    {"9a047d43-d086-4216-a211-e610d0b7b96b", "rbc_news_bot"},
    {"2f863bd9-18dd-4a4c-95d6-7778c640f4c3", "vedomosti_news_bot"},
    {"66159359-d60f-46a4-968c-a9e7935b9b1f", "dtf_news_bot"},
    {"9410cb37-5842-4963-8fd2-e1f7c7d07c0a", "tjournal_news_bot"},
    {"70a0c6bf-7330-4ab9-8ac3-e238c858808e", "vc_news_bot"},
    {"94135a9c-2a61-49d4-a06b-8aba9958d5a2", "mediazona_news_bot"},
};

using TApiSkillResponseScheme = NBASSExternalSkill::TAdminApiResponse<TSchemeTraits>::TConst;
using TApiSkillResponse = TSchemeHolder<TApiSkillResponseScheme>;

class ISkillResolver {
public:
    using TSkillResponsePtr = THolder<TApiSkillResponse>;

    virtual ~ISkillResolver() = default;

    virtual TSkillResponsePtr ResolveSkillId(TContext& ctx, TStringBuf skillId, const TConfig& config,
                                             TErrorBlock::TResult* error) const = 0;

    virtual TVector<TSkillResponsePtr> ResolveSkillIds(TContext& ctx, const TVector<TStringBuf>& skillIds, const TConfig& config,
                                                       TErrorBlock::TResult* error) const = 0;

    static const ISkillResolver& GlobalResolver();

    /** It is changed interal global resolver to the given one or use defaul one.
     * Beware that this is not thread-safe.
     * @param[in] skillResolver is a new global skill resolver
     */
    static void ResetGlobalResolver(std::unique_ptr<ISkillResolver> skillResolver);

private:
    static std::unique_ptr<ISkillResolver> Global;
};

class ISkillParser;

class TSession {
public:
    explicit TSession(TContext& ctx);

    void UpdateContext(TContext& ctx) const;

    ui64 SeqNum() const {
        return SeqNumber;
    }

    TStringBuf Id() const {
        return Guid;
    }

    bool IsNew() const {
        return 0 == SeqNumber;
    }

private:
    ui64 SeqNumber;
    TString Guid;
};

/** This respresents skill info/description itself.
 * It does request to skill platform api to know if script is ok!
 * Do some checks and allow you to request a this skill and obtain its response
 */
class TSkillDescription {
public:
    using TStyleScheme = NBASSExternalSkill::TStyle<TSchemeTraits>;

public:
    TSkillDescription(TContext& ctx);

    TSkillDescription(const TSlot& slot, TContext& ctx, NHttpFetcher::TRequest* fakeRequest);//for UNITTESTs

    void Init(const TSlot& slot, TContext& ctx);

    /** Check if skill is ok (means no error during construction)
     * @see Error()
     */
    operator bool() const {
        return !ResultValue;
    }

    const TErrorBlock::TResult& Result() const {
        return ResultValue;
    }

    /** If user create request as ${skill_id} it turns on the developer mode.
     * Which means that all errors will be returned as text back to vins
     * The flag are put into 'skill_info' slot.
     */
    bool IsDeveloperMode() const {
        return DeveloperMode;
    }

    /** Create a request to skill parse it and create response parser.
     * @param[in] session is use to create request for skill
     * @param[out] parser will be filled with response parse if request succeeded
     * @return result of reqest and response parsing (if error <parser> is not modified)
     */
    TErrorBlock::TResult RequestSkill(TContext& ctx,
                                      TSession& session,
                                      std::unique_ptr<ISkillParser>* parser,
                                      TSkillDiagnosticInfo& diagnosticInfo) const;

    /** Writes info data into the given context.
     */
    void WriteInfo(TContext* ctx) const;

    bool IsTabSkill() const;
    bool OpenInNewTab(const TContext& ctx) const;

    bool IsFromConsole() const {
        return IsConsole;
    }

    /** Add update_dialog_info command into the given context.
     */
    void WriteUpdateDialogInfo(TContext* ctx) const;

    const TString& GetSkillNameForMetrics() const {
        return SkillNameForMetrics;
    }

    TApiSkillResponseScheme::TResultConst Scheme() const {
        return Data->Scheme().Result();
    }

    TStyleScheme Style() const;
    TStyleScheme DarkStyle() const;
    TStyleScheme FindStyleByName(const TStringBuf styleName) const;

    const NMonitoring::TLabels& SignalLabels() const {
        return Labels;
    }

    NMonitoring::TLabels ConstructSignalLabels(const TString& counterName) const {
        NMonitoring::TLabels labels = Labels;
        labels.Add(TStringBuf("sensor"), counterName);
        return labels;
    }

    const NSc::TValue* GetNerInfo() const {
        return NerInfo.Get();
    }

public:
    /** Inits local styles cache from config.
     */
    static void InitStylesFromConfig(const TConfig& config);

    /** Create image url regards avatar id and image type
     * @param[in] ctx is a context
     * @param[in] imageId is a string representation avatar image id
     * @param[in] imageType is an actually size of image
     * @param[in] ns is an avatar's namespace
     * @return a valid avatar's image url
     */
    static TString CreateImageUrl(const TContext& ctx, TStringBuf imageId, TStringBuf imageType, TStringBuf ns);

private:
    TErrorBlock::TResult ProcessSkillResponse(TContext& ctx, TSession& session, std::unique_ptr<ISkillParser>* parser,
                                              NHttpFetcher::TResponse::TRef resp, TSkillDiagnosticInfo& diagnosticInfo,
                                              const TString& uri, const TString& skillNameNorm,
                                              const TApiSkillResponseScheme::TResultConst& skillResult,
                                              const NSc::TValue& requestJson) const;

    void WriteCounters(TContext& ctx, const TString& counterName, const TString& skillNameNorm) const;
    void WriteDebugSlot(TContext& ctx, const NSc::TValue& request, TStringBuf response) const;
private:
    TMaybe<NSc::TValue> NerInfo;
    /// It depends on slot 'skill_description'.
    bool IsConsole;
    TErrorBlock::TResult ResultValue;
    THolder<const TApiSkillResponse> Data;
    TString SkillNameForMetrics;
    bool DeveloperMode;
    NMonitoring::TLabels Labels;

private:
    using TStylesMap = THashMap<TString, TSchemeHolder<TStyleScheme>>;
    static TStylesMap Styles;
};

// put here a forward declaration for current active skill parser class
class TSkillParserVersion1x;
/** Base class for skill parser (dependable on versions)
 * FIXME doc!!!!
 */
class ISkillParser {
public:
    using TCurrentSkillParser = TSkillParserVersion1x;

public:
    virtual ~ISkillParser() = default;

    /** Analyze skill response and return its response parser dependable on version.
     */
    static std::unique_ptr<ISkillParser> CreateParser(const TSession& session, const NSc::TValue& response,
                                                      const TSkillDescription& skill);

    /** Create answer in context for vins.
     * All the errors appear during parsing skill response will be returned by this method.
     * @param[in] ctx is the input context
     * @param[in|out] respCtx is a reponse context which ususally be the same as <ctx> but sometimes when form changed it is switched to the new one
     * @param[in] beforeCb is a callback which is called just before writing answer into form (usefull if you want add sudgests before skill do this)
     */
    virtual TErrorBlock::TResult CreateVinsAnswer(TContext& ctx, TContext** respCtx, std::function<void(TContext&)> beforeCb) = 0;
};

NMonitoring::TCountersChanger Sensors();
NMonitoring::TCountersChanger SkillSensors(TContext& ctx);

} // namespace NExternalSkill
} // namespace NBASS
