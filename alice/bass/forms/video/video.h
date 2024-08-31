#pragma once

#include "billing_api.h"
#include "defs.h"

#include <alice/bass/forms/vins.h>
#include <alice/bass/libs/globalctx/globalctx.h>

namespace NBASS {

namespace NVideo {
class IVideoClipsProvider;
class TVideoSlots;
} // namespace NVideo

// Wrapper in NBASS namespace for SelectVideoFromState
TResultValue ContinueLastWatchedVideo(NVideo::TVideoItemConstScheme item, TContext& ctx);

IContinuation::TPtr PreparePlayNextVideo(TContext& ctx);
IContinuation::TPtr PreparePlayPreviousVideo(TContext& ctx);
TResultValue PlayVideo(TContext& ctx, NVideo::TVideoItemConstScheme item);


class TBaseVideoHandler : public TContinuableHandler {
protected:
    IContinuation::TPtr Prepare(TRequestHandler& r) override;
    virtual IContinuation::TPtr PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) = 0;
};

class TVideoSearchFormHandler : public TBaseVideoHandler {
public:
    static TContext::TPtr SetAsResponse(TContext& ctx, bool callbackSlot, TStringBuf searchText);
    static void Register(THandlersMap* handlers);

protected:
    virtual IContinuation::TPtr PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) override;
};

class TSelectVideoFromGalleryHandler : public TBaseVideoHandler {
public:
    static void Register(THandlersMap* handlers);

protected:
    virtual IContinuation::TPtr PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) override;
};

class TVideoGoToScreenFormHandler : public TBaseVideoHandler {
public:
    static void Register(THandlersMap* handlers);

protected:
    virtual IContinuation::TPtr PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) override;
};

class TVideoPaymentConfirmedHandler : public TBaseVideoHandler {
public:
    static void Register(THandlersMap* handlers);
    static TContext::TPtr SetAsResponse(TContext& ctx, bool callbackSlot = false);

protected:
    virtual IContinuation::TPtr PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) override;
};

class TOpenCurrentVideoHandler : public TBaseVideoHandler {
public:
    static TContext::TPtr SetAsResponse(TContext& ctx, bool callbackSlot);
    static void Register(THandlersMap* handlers);

protected:
    virtual IContinuation::TPtr PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) override;
};

class TOpenCurrentTrailerHandler : public TBaseVideoHandler {
public:
    static void Register(THandlersMap* handlers);

protected:
    virtual IContinuation::TPtr PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) override;
};

class TNextVideoTrackHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

class TPlayVideoActionHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

class TPlayVideoFromDescriptorActionHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

class TVideoFinishedTrackHandler : public TBaseVideoHandler {
public:
    static void Register(THandlersMap* handlers);

protected:
    virtual IContinuation::TPtr PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) override;
};

class TVideoRecommendationHandler : public TContinuableHandler {
public:
    IContinuation::TPtr Prepare(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

class TShowVideoSettingsHandler final : public TContinuableHandler {
public:
    IContinuation::TPtr Prepare(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

class TSkipVideoFragmentHandler final : public TContinuableHandler {
public:
    IContinuation::TPtr Prepare(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

class TChangeTrackHandler final : public TContinuableHandler {
public:
    IContinuation::TPtr Prepare(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

class TVideoHowLongHandler final : public TContinuableHandler {
public:
    IContinuation::TPtr Prepare(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

void RegisterVideoContinuations(TContinuationParserRegistry& registry);

} // namespace NBASS
