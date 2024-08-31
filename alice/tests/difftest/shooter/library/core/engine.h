#pragma once

#include <alice/tests/difftest/shooter/library/core/apps/app.h>
#include <alice/tests/difftest/shooter/library/core/apps/impl/joker_app.h>
#include <alice/tests/difftest/shooter/library/core/apps/impl/megamind_app.h>
#include <alice/tests/difftest/shooter/library/core/context.h>
#include <alice/tests/difftest/shooter/library/core/run_settings.h>

#include <alice/tests/difftest/shooter/library/core/factory/hollywood_bass_factory.h>
#include <alice/tests/difftest/shooter/library/core/factory/hollywood_factory.h>
#include <alice/tests/difftest/shooter/library/core/factory/megamind_factory.h>

#include <util/generic/map.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

namespace NAlice::NShooter {

class IEngine {
public:
    virtual ~IEngine() = default;

    virtual void Run() = 0;
    virtual void ForceShutdown() = 0;

    virtual const THolder<IPorts>& Ports() const = 0;
    virtual const TRunSettings& RunSettings() const = 0;
};

class TEngine : public IEngine {
public:
    TEngine(const TContext& ctx, TRunSettings runSettings);
    ~TEngine();

    void Run() override;
    void ForceShutdown() override;

    const THolder<IPorts>& Ports() const override {
        return Ports_;
    }

    const TRunSettings& RunSettings() const override {
        return RunSettings_;
    }

private:
    void Shoot();

private:
    const TContext& Ctx_;
    const TRunSettings RunSettings_;
    const THolder<IThreadPool> ThreadPool_;
    const THolder<IPorts> Ports_;

    THolder<IFactory> Factory_;
    TIntrusivePtr<IApp> App_;
};

} // namespace NAlice::NShooter
