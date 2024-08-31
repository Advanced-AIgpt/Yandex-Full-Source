#pragma once

#include "context.h"

#include <alice/bass/forms/vins.h>

#include <library/cpp/scheme/scheme.h>

namespace NBASS {

class IComputerVisionBaseHandler: public IHandler {
public:
    virtual ~IComputerVisionBaseHandler() = default;
    virtual TResultValue Do(TRequestHandler& r) override;

    virtual bool ShouldShowIntentButtons() const {
        return false;
    }

protected:
    virtual TResultValue WrappedDo(TComputerVisionContext& cvContext) const = 0;
    virtual TMaybe<TString> GetForcingString() const;
};

class TComputerVisionMainHandler: public IComputerVisionBaseHandler {
public:
    TComputerVisionMainHandler(ECaptureMode captureMode = ECaptureMode::PHOTO, bool frontal = false);
    static void Register(THandlersMap* handlers);
    static void Init();
    static void SetAsResponse(TContext& ctx, TStringBuf subtypeCmd);
    static bool IsSupported(const TContext& ctx);

    bool ShouldShowIntentButtons() const override {
        return true;
    }

protected:
    virtual bool MakeBestAnswer(TComputerVisionContext& cvContext) const;

    TResultValue WrappedDo(TComputerVisionContext& cvContext) const override;

    virtual bool IsNeedOcrData(const IComputerVisionAnswer* answer) const;
    virtual bool IsNeedOfficeLensData(const IComputerVisionAnswer* answer) const;
    virtual const TStringBuf GetAdditionalFlag(const IComputerVisionAnswer* answer) const;

private:
    bool MakeRandomAnswer(TComputerVisionContext& cvContext) const;
    const IComputerVisionAnswer* TryExtractForceAnswer(TComputerVisionContext& cvContext,
                                                       bool& needSimilarAnswer) const;

private:
    static const TAnswersList StubAnswers;
    static const TFlagAnswerPairs ForcingAnswers;
    static const TCaptureModeToAnswerMap CaptureModes;
    const ECaptureMode CaptureMode;
    const bool Frontal;
};

class TComputerVisionOnboardingHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;
    static void Register(THandlersMap* handlers);
};

class TComputerVisionClothesBoxHandler: public IComputerVisionBaseHandler {
public:
    static void Register(THandlersMap* handlers);

protected:
    TResultValue WrappedDo(TComputerVisionContext& cvContext) const override;
};

class TComputerVisionEllipsisClothesHandler: public TComputerVisionMainHandler {
public:
    TComputerVisionEllipsisClothesHandler()
            : TComputerVisionMainHandler(ECaptureMode::CLOTHES, false)
    {}
    static void Register(THandlersMap* handlers);

protected:
    bool MakeBestAnswer(TComputerVisionContext& cvContext) const override;
};

class TComputerVisionEllipsisDetailsHandler: public TComputerVisionMainHandler {
public:
    TComputerVisionEllipsisDetailsHandler()
        : TComputerVisionMainHandler(ECaptureMode::DETAILS, false)
    {}

    static void Register(THandlersMap* handlers);

protected:
    bool MakeBestAnswer(TComputerVisionContext& cvContext) const override;
};

class TComputerVisionEllipsisMarketHandler: public TComputerVisionMainHandler {
public:
    TComputerVisionEllipsisMarketHandler()
        : TComputerVisionMainHandler(ECaptureMode::MARKET, false)
    {}

    static void Register(THandlersMap* handlers);

protected:
    bool MakeBestAnswer(TComputerVisionContext& cvContext) const override;
};

class TComputerVisionEllipsisOcrHandler: public TComputerVisionMainHandler {
public:
    TComputerVisionEllipsisOcrHandler()
            : TComputerVisionMainHandler(ECaptureMode::OCR, false)
    {}

    static void Register(THandlersMap* handlers);

protected:
    bool MakeBestAnswer(TComputerVisionContext& cvContext) const override;
};

class TComputerVisionNegativeFeedbackHandler: public IComputerVisionBaseHandler {
public:
    static void Register(THandlersMap* handlers);

protected:
    TResultValue WrappedDo(TComputerVisionContext& cvContext) const override;
};

class TComputerVisionFrontalSimilarPeople: public TComputerVisionMainHandler {
public:
    TComputerVisionFrontalSimilarPeople();
    static void Register(THandlersMap* handlers);

protected:
    bool MakeBestAnswer(TComputerVisionContext& cvContext) const override;
    TMaybe<TString> GetForcingString() const override;
};

class TComputerVisionSimilarPeople: public TComputerVisionMainHandler {
public:
    TComputerVisionSimilarPeople();
    static void Register(THandlersMap* handlers);

protected:
    bool MakeBestAnswer(TComputerVisionContext& cvContext) const override;
    TMaybe<TString> GetForcingString() const override;
};

class TComputerVisionEllipsisSimilarPeopleHandler: public TComputerVisionMainHandler {
public:
    TComputerVisionEllipsisSimilarPeopleHandler()
    {}

    static void Register(THandlersMap* handlers);

    const TStringBuf GetAdditionalFlag(const IComputerVisionAnswer* answer) const override;

protected:
    bool MakeBestAnswer(TComputerVisionContext& cvContext) const override;
};

} // NBASS
