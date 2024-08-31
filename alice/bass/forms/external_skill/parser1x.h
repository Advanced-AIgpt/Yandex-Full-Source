#pragma once

#include "skill.h"

#include <alice/bass/forms/context/fwd.h>

#include <alice/bass/util/error.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>

namespace NBASS {
namespace NExternalSkill {

constexpr std::array<TStringBuf, 8> SkillsWhiteList { // Skills' ids that are approved for billing and authorization
    TStringBuf("2729c1c6-fedb-4d3a-92c7-5a90204ef4ac"),
    TStringBuf("2ca8abdc-0628-4569-bb27-9158a2f31981"), // @pazus test skill
    TStringBuf("10d63dd6-2afa-4f90-ad09-5d89f551a216"), // @emvolkov buy an elephant
    TStringBuf("ce7d6f09-122c-4ec8-b0b2-1d05131ac015"), // @emvolkov buy an elephant
    TStringBuf("20f23525-6f70-48e2-99a2-0bbff598b4be"), // @seralexeev test skill
    TStringBuf("f951a481-d037-4de7-afc0-ed73c9621ad1"), // MosEnergoSbyt
    TStringBuf("c193cf56-1e7e-4a52-97c8-9ee932b51e8c"), // PapaJohnsTest
    TStringBuf("8a4e4146-61b6-4e1c-8a48-3a1525a2d625"), // PapaJohnsTest
};

enum ESkillRequestType {
    Default,
    AccountLinkingComplete,
    SkillsPurchaseComplete
};

class TSkillValidateHelper;

class TSkillParserVersion1x: public ISkillParser {
public:
    using TSkillResponseScheme = NBASSExternalSkill::TSkillResponse1x<TSchemeTraits>::TConst;
    using TApiSkillResponseScheme = TApiSkillResponseScheme::TConst;

    static const TStringBuf Version;

public:
    TSkillParserVersion1x(const TSession& session, const TSkillDescription& skill, NSc::TValue response);

    /** Prepare json to request a skill
     */
    static TErrorBlock::TResult PrepareRequest(const TContext& ctx,
                                               const TSkillDescription& skill,
                                               const TSession& session,
                                               ESkillRequestType requetsType,
                                               NSc::TValue* request);

    /** Validate skill answer, prepare output context (if it is switched to a new form or use original one).
     * And finally call DrawVinsAnswer() for real creating vins slots/actions/blocks.
     * @param[in] ctx is the request context
     * @param[out] respCtx is needed to put pointer to response context (where all slots/actions/blocks are created)
     * @param[in] breforeCb is a callback which is used just after the skill validation is successfully finshed but before putting any data into context
     */
    // TODO separate it into two really disctinct public function (validate skill data/create vins answer)
    TErrorBlock::TResult CreateVinsAnswer(TContext& ctx, TContext** respCtx, std::function<void(TContext&)> beforeCb) override;

private:
    /** Check if slot slot has utterance and if true, full in <request> accordingly and return true.
     * Set <rval> to error if there is one.
     * @param[out] request is filled in if utterance is found and no error happened
     * @param[out] rval is filled in if utterance is found but error is happened (also fundction returns true)
     * @return true if utterance is found (no matter if error is happened or no), false - otherwise
     */
    static bool PrepareUtterance(const TContext& ctx, const TSlot& slot, NSc::TValue* request, TErrorBlock::TResult* rval);
    static bool PrepareButton(const TContext& ctx, const TSlot& slot, NSc::TValue* request, TErrorBlock::TResult* rval);

    static void AddMarkup(const TContext& ctx, NSc::TValue* request);

private:
    /** Do the real job of creating the vins answer. Draw divcard, text, buttons...
     */
    TErrorBlock::TResult DrawVinsAnswer(const TSkillResponseScheme& responseScheme, TContext* ctx) const;

    // TODO move out it from this class to the interface or somewhere else
    void ApplyAbuse(const TSkillValidateHelper& helper, TContext& ctx);

private:
    const TSession& Session;
    const TSkillDescription& Skill;
    NSc::TValue Response;
};

} // namespace NExternalSkill
} // namespace NBASS
