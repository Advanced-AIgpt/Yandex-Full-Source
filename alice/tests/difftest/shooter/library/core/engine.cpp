#include "engine.h"

#include <alice/joker/library/log/log.h>
#include <alice/rtlog/evloganalize/split_by_reqid/library/splitter.h>

#include <alice/tests/difftest/shooter/library/core/requester/hollywood_bass_requester.h>
#include <alice/tests/difftest/shooter/library/core/requester/hollywood_requester.h>
#include <alice/tests/difftest/shooter/library/core/requester/megamind_requester.h>

#include <util/system/fs.h>
#include <util/thread/pool.h>

#include <thread>

namespace NAlice::NShooter {

namespace {

THolder<IFactory> MakeFactory(const TContext& ctx, const TEngine& engine) {
    if (engine.RunSettings().EnableHollywoodMode) {
        return MakeHolder<THollywoodFactory>(engine);
    } else if (engine.RunSettings().EnableHollywoodBassMode) {
        return MakeHolder<THollywoodBassFactory>(ctx, engine);
    } else {
        return MakeHolder<TMegamindFactory>(ctx, engine);
    }
}

void HealthCheckThread(const THolder<IFactory>& factory, TIntrusivePtr<IApp>& app, TMutex& lock, NAtomic::TBool& finished) {
    while (true) {
        Sleep(TDuration::Seconds(10));
        if (finished) {
            break;
        }
        if (const auto error = app->Ping()) {
            LOG(INFO) << "Waiting for lock..." << Endl;
            auto g = Guard(lock);
            LOG(INFO) << "Reload apps" << Endl;

            // Stop old apps
            app->Stop();

            // Run new apps
            app = factory->MakeApp();
            app->Init();
            app->Run();
        }
    }
}

void SleepUpToSecond(TDuration passed = TDuration::Zero()) {
    if (passed.SecondsFloat() < 1.0) {
        auto sleepDuration = TDuration::Seconds(1) - passed;
        LOG(INFO) << "Sleep " << sleepDuration << " before next request" << Endl;
        Sleep(sleepDuration);
    }
}

void SleepOnError() {
    Sleep(TDuration::Seconds(3));
}

} // namespace

TEngine::TEngine(const TContext& ctx, TRunSettings runSettings)
    : Ctx_{ctx}
    , RunSettings_{std::move(runSettings)}
    , ThreadPool_{CreateThreadPool(Ctx_.Config().Threads())}
    , Ports_{MakeHolder<TPorts>()}
    , Factory_{MakeFactory(Ctx_, *this)}
    , App_{Factory_->MakeApp()}
{
    TFsPath{RunSettings_.LogsPath}.MkDirs();
    if (!RunSettings_.ResponsesPath.Empty()) {
        TFsPath{RunSettings_.ResponsesPath}.MkDirs();
    }

    App_->Init();
}

TEngine::~TEngine() {
    ForceShutdown();
}

void TEngine::Run() {
    if (const auto error = App_->Run()) {
        ythrow yexception() << error->Msg();
    }
    Shoot();
    //AppsHandler_->AfterShooting();

    if (RunSettings_.EnableIdleMode) {
        LOG(INFO) << "Idle mode, apps are working..." << Endl;
        Sleep(TDuration::Max()); // TODO (sparkle): wait for signal instead of sleep
    }
}

void TEngine::Shoot() {
    int limit = Ctx_.Config().RequestsLimit();
    if (limit == 0) {
        return;
    }

    LOG(INFO) << "Request shooting started" << Endl;

    TVector<TString> requestFiles;
    TFsPath requestsFolder{Ctx_.Config().RequestsPath()};
    requestsFolder.ListNames(requestFiles);

    std::atomic_int count = 0;
    std::atomic_int errorsCount = 0;

    THolder<IRequester> requester = Factory_->MakeRequester();
    TMutex healthLock;

    THashMap<TString, double> requestsDurations;

    NAtomic::TBool finished(false);
    std::thread healthThread(HealthCheckThread, std::ref(Factory_), std::ref(App_), std::ref(healthLock), std::ref(finished));

    for (const auto& requestFile : requestFiles) {
        --limit;
        if (limit < 0) {
            break;
        }

        ThreadPool_->SafeAddFunc([&]() {
            {
                auto g = Guard(healthLock);
            }

            TLogging::InitTlsUniqId();
            LOG(INFO) << "Working with request #" << ++count << ": " << requestFile << Endl;

            auto response = requester->Request(requestsFolder / requestFile);
            if (response.Defined()) {
                auto duration = response->Duration;
                LOG(INFO) << "Request " << requestFile << " completed in " << duration << Endl;

                if (RunSettings_.EnablePerfMode) {
                    requestsDurations.emplace(requestFile, duration.SecondsFloat());
                } else {
                    // Save output data to folder
                    TFsPath requestPath = response->OutputPath;
                    requestPath.Parent().MkDirs();
                    TFileOutput{requestPath}.Write(response->Data);
                }

                // One thread = 1 RPS
                SleepUpToSecond(duration);
            } else {
                LOG(INFO) << "Error requests count: " << ++errorsCount << Endl;
                if (RunSettings_.EnablePerfMode) {
                    requestsDurations.emplace(requestFile, std::nan(""));
                }
                SleepOnError();
            }
        });
    }
    ThreadPool_->Stop();
    finished = true;
    healthThread.join();

    LOG(INFO) << "Request shooting finished" << Endl;

    if (RunSettings_.EnablePerfMode) {
        TFsPath resultPath = TFsPath{RunSettings_.ResponsesPath} / "result.txt";
        LOG(INFO) << "Save times to " << resultPath << Endl;
        TFileOutput fs{TFile{resultPath, OpenAlways | WrOnly}};
        for (const auto& p : requestsDurations) {
            fs << p.first << " " << p.second << Endl;
        }
    }
}

void TEngine::ForceShutdown() {
    if (!RunSettings_.DontClose) {
        LOG(INFO) << "Need to shutdown engine" << Endl;
        App_->Stop();
    }
}

} // namespace NAlice::NShooter
