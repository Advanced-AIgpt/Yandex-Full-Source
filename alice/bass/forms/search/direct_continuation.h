#pragma once

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/continuations.h>
#include <alice/bass/forms/vins.h>

#include <alice/bass/libs/fetcher/neh.h>

namespace NBASS::NDirectGallery {

class IUidProvider {
public:
    virtual ~IUidProvider() = default;
    virtual TMaybe<TString> GetUid(TContext& ctx) const = 0;
};

class TBlackBoxUidProvider final : public IUidProvider {
public:
    TMaybe<TString> GetUid(TContext& ctx) const override;
};

class TDirectGalleryHitConfirmContinuation final : public IContinuation {
public:
    TDirectGalleryHitConfirmContinuation(
        TContext::TPtr ctx, TString hitCounter, TString linkHead, TVector<TString> linkTails,
        std::unique_ptr<NHttpFetcher::IRequestAPI> requestAPI =
            std::make_unique<NHttpFetcher::TRequestAPI>(NHttpFetcher::BassRequestAPI()),
        std::unique_ptr<IUidProvider> uidProvider = std::make_unique<TBlackBoxUidProvider>());

    TStringBuf GetName() const override {
        return NAME;
    }

    static TMaybe<TDirectGalleryHitConfirmContinuation> FromJson(NSc::TValue value, TGlobalContextPtr globalContext,
                                                                 NSc::TValue meta, const TString& authHeader,
                                                                 const TString& appInfoHeader,
                                                                 const TString& fakeTimeHeader,
                                                                 const TMaybe<TString>& userTicketHeader,
                                                                 const NSc::TValue& configPatch);

    TContext& GetContext() const override {
        return *Context;
    }

public:
    static constexpr TStringBuf NAME = "TDirectGalleryHitConfirmContinuation";

protected:
    NSc::TValue ToJsonImpl() const override;
    TResultValue Apply() override;

private:
    TContext::TPtr Context;
    std::unique_ptr<NHttpFetcher::IRequestAPI> RequestAPI;
    std::unique_ptr<IUidProvider> UidProvider;
    TString HitCounter;
    TString LinkHead;
    TVector<TString> LinkTails;
};

void RegisterDirectGalleryContinuation(TContinuationParserRegistry& registry);

} // namespace NBASS:NDirectGallery
