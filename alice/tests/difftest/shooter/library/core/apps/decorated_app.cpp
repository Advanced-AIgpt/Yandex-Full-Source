#include <alice/tests/difftest/shooter/library/core/apps/app.h>

#include <alice/joker/library/log/log.h>
#include <alice/tests/difftest/shooter/library/core/context.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>

#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/folder/path.h>

namespace NAlice::NShooter {

namespace {

constexpr int RESPONSE_LOG_LIMIT = 400;
constexpr TDuration PING_INTERVAL = TDuration::Seconds(3);

} //

TDecoratedApp::TDecoratedApp() {
    ShellCommandOptions_.SetAsync(true);
}

TDecoratedApp::TDecoratedApp(TIntrusivePtr<IApp> wrappee)
    : Wrappee_{wrappee}
{
    ShellCommandOptions_.SetAsync(true);
}

TStatus TDecoratedApp::Ping() {
    if (Wrappee_) {
        if (const auto error = Wrappee_->Ping()) {
            return error;
        }
    }
    return ExtraPing();
}

TStatus TDecoratedApp::Run() {
    if (Wrappee_) {
        if (const auto error = Wrappee_->Run()) {
            return error;
        }
    }

    if (const auto error = ExtraRun()) {
        return error;
    }

    while (true) {
        if (const auto error = ExtraPing()) {
            // Check for failed process
            TShellCommand::ECommandStatus status = ShellCommand_->GetStatus();
            if (status != TShellCommand::ECommandStatus::SHELL_RUNNING) {
                LOG(ERROR) << "App " << Name() << " is NOT running. Status is " << static_cast<int>(status) << ", look at ECommandStatus" << Endl;
                return TError() << "App " << TString{Name()} << " not running";
            }

            // Sleep before next check
            LOG(INFO) << "Unsuccessful " << Name() << " ping" << Endl;
            LOG(INFO) << "Sleeping " << PING_INTERVAL  << " seconds for next ping" << Endl;
            Sleep(PING_INTERVAL);
        } else {
            LOG(INFO) << "Successful " << Name() << " ping, app started" << Endl;
            break;
        }
    }

    return Success();
}

void TDecoratedApp::Stop() {
    if (ShellCommand_) {
        LOG(INFO) << "Stopping app " << Name() << Endl;
        ShellCommand_->Terminate();
        ShellCommand_->Wait();
    }
    if (Wrappee_) {
        Wrappee_->Stop();
    }
}

TLocalDecoratedApp::TLocalDecoratedApp(const IEngine& engine)
    : Engine_{engine}
{
}

TLocalDecoratedApp::TLocalDecoratedApp(const IEngine& engine, TIntrusivePtr<IApp> wrappee)
    : TDecoratedApp{wrappee}
    , Engine_{engine}
{
}

void TLocalDecoratedApp::Init() {
    if (Wrappee_) {
        Wrappee_->Init();
    }

    const auto& runSettings = Engine_.RunSettings();
    TString outFileName{TString::Join(Name(), ".out")};
    TString errFileName{TString::Join(Name(), ".err")};
    TFsPath logsPath{runSettings.LogsPath};
    auto fileMask = OpenAlways | WrOnly | ForAppend;

    OutputStream_ = MakeHolder<TUnbufferedFileOutput>(TFile{logsPath / outFileName, fileMask});
    ErrorStream_ = MakeHolder<TUnbufferedFileOutput>(TFile{logsPath / errFileName, fileMask});

    // Shell options
    ShellCommandOptions_.SetOutputStream(OutputStream_.Get());
    ShellCommandOptions_.SetErrorStream(ErrorStream_.Get());
    ShellCommandOptions_.SetClearSignalMask(true);

    ExtraInit();
}

void TLocalDecoratedApp::ExtraInit()
{
}

NNeh::TResponseRef AppRequest(TStringBuf appName, const NUri::TUri& uri) {
    LOG(INFO) << appName << " request: " << uri << Endl;

    NNeh::TMessage msg{NNeh::TMessage::FromString(uri.PrintS())};
    NNeh::TResponseRef r = NNeh::Request(msg)->Wait(TDuration::Seconds(5));

    if (r) {
        if (r->Data.Size() <= RESPONSE_LOG_LIMIT) {
            LOG(INFO) << appName << " request response: " << r->Data.Quote() << Endl;
        } else {
            LOG(INFO) << appName << " request responsed with size " << r->Data.Size() << Endl;
        }

        if (r->IsError()) {
            LOG(ERROR) << appName << " request error: " << r->GetErrorText().Quote() << Endl;
        }
    } else {
        LOG(WARNING) << appName << " request timed out" << Endl;
    }

    return r;
}

} // namespace NAlice::NShooter
