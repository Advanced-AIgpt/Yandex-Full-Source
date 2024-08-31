#pragma once

#include <alice/tests/difftest/shooter/library/core/fwd.h>
#include <alice/tests/difftest/shooter/library/core/status.h>
#include <alice/tests/difftest/shooter/library/core/apps/app.h>

#include <util/generic/noncopyable.h>
#include <util/system/shellcommand.h>
#include <util/stream/file.h>

#include <library/cpp/neh/neh.h>
#include <library/cpp/uri/uri.h>

namespace NAlice::NShooter {

/**
 * Base class for apps that contain child apps on that the concrete app depends
 */
class TDecoratedApp : public IApp {
public:
    TDecoratedApp();
    TDecoratedApp(TIntrusivePtr<IApp> wrappee);

    TStatus Run() override;
    TStatus Ping() override;
    void Stop() override;

protected:
    virtual TStatus ExtraRun() = 0;
    virtual TStatus ExtraPing() = 0;

protected:
    THolder<TShellCommand> ShellCommand_;
    TShellCommandOptions ShellCommandOptions_;
    TIntrusivePtr<IApp> Wrappee_;
};

/**
 * Base class for apps that launched locally and have output streams
 */
class TLocalDecoratedApp : public TDecoratedApp {
public:
    TLocalDecoratedApp(const IEngine& engine);
    TLocalDecoratedApp(const IEngine& engine, TIntrusivePtr<IApp> wrappee);

    void Init() override;

protected:
    virtual void ExtraInit();

protected:
    const IEngine& Engine_;

private:
    THolder<IOutputStream> OutputStream_;
    THolder<IOutputStream> ErrorStream_;
};

NNeh::TResponseRef AppRequest(TStringBuf appName, const NUri::TUri& uri);

} // namespace NAlice::NShooter
