#pragma once

#include "providers.h"

#include <alice/bass/forms/vins.h>

namespace NBASS::NMusic {

enum class EClientType {
    SmartSpeaker /* "smart_speaker" */,
    SearchApp /* "search_app" */,
    Navi /* "navi" */,
    Auto /* "auto" */,
    Other /* "other" */,
};

bool IsYaMusicSupported(TContext& ctx);
bool FillParams(TContext& ctx, const TVector<TStringBuf>& slotNames, bool needArray, NSc::TValue* params);
void FillSlotData(TContext& ctx, NSc::TValue& slotData);

class TSearchMusicHandler: public TContinuableHandler {
public:
    TResultValue Do(TRequestHandler& r) override;
    IContinuation::TPtr Prepare(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
    static TResultValue DoWithoutCallback(TContext& ctx);
    static TContext::TPtr SetAsResponse(TContext& ctx, bool callbackSlot, NSc::TValue snippet);
    static TContext::TPtr SetAsResponse(TContext& ctx, bool callbackSlot);
    static NSc::TValue CreateMusicDataFromSnippet(TContext& ctx, const NSc::TValue& snippet);
};

class TMusicAnaphoraHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static TResultValue Run(TContext& ctx);
    static void Register(THandlersMap* handlers);
};

class TMusicPlayObjectActionHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

void RegisterMusicContinuations(TContinuationParserRegistry& registry);

TResultValue GetAnswer(IMusicProvider& provider,
                       TContext& ctx,
                       NSc::TValue* out,
                       const NSc::TValue& slotData,
                       const NSc::TValue& actionData,
                       const bool alarm = false);

} // namespace NBASS::NMusic
