#pragma once

#include <alice/hollywood/library/scenarios/time_capsule/context/context.h>
#include <alice/hollywood/library/scenarios/time_capsule/scene/scene.h>
#include <alice/hollywood/library/scenarios/time_capsule/util/util.h>

namespace NAlice::NHollywood::NTimeCapsule {

class ISceneCreator;
using TSceneCreatorPtr = THolder<ISceneCreator>;

class ISceneCreator {
public:
    virtual THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const = 0;

    virtual ~ISceneCreator() = default;
};

class TTimeCapsuleQuestionSceneCreator : public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleSaveApproveSceneCreator : public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleSaveSceneCreator : public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleStartSceneCreator : public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleStopSceneCreator : public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleInterruptSceneCreator : public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleApproveRetrySceneCreator : public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleTextRetrySceneCreator : public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleInformationSceneCreator: public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleOpenSceneCreator: public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleHowLongSceneCreator: public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleDeleteSceneCreator: public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleRerecordSceneCreator: public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

class TTimeCapsuleIrrelevantSceneCreator : public ISceneCreator {
public:
    THolder<TScene> TryCreate(const TTimeCapsuleContext& ctx) const override final;
};

} // namespace NAlice::NHollywood::NTimeCapsule
