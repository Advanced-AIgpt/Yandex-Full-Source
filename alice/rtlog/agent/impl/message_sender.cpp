#include "message_sender.h"

#include <alice/rtlog/agent/protos/config.pb.h>

#include <kikimr/persqueue/sdk/deprecated/cpp/v2/persqueue.h>

#include <util/folder/dirut.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/printf.h>
#include <util/system/env.h>
#include <util/system/hostname.h>
#include <util/random/random.h>

namespace NRTLogAgent {
    namespace {
        using namespace NPersQueue;
        using namespace NThreading;
        using namespace google::protobuf;

        class TLogbrokerMessageSender: public IMessageSender {
        public:
            explicit TLogbrokerMessageSender(const TPQConfig& queueConfig)
                : PQLib(nullptr)
                , Logger(MakeIntrusive<TCerrLogger>(TLOG_WARNING))
                , CredentialsProvider(CreateCredentialsProvider(queueConfig, Logger))
                , ProducerSlots(queueConfig.GetSourceIds())
            {
                TPQLibSettings pqLibSettings;
                pqLibSettings.ThreadsCount = 3;
                PQLib = MakeHolder<TPQLib>(pqLibSettings);
                const auto ts = TInstant::Now().MilliSeconds();
                for (size_t i = 0; i < ProducerSlots.size(); ++i) {
                    auto& slot = ProducerSlots[i];
                    TProducerSettings settings;
                    settings.Server = TServerSetting{queueConfig.GetServer(),
                                                     static_cast<ui16>(queueConfig.GetPort())};
                    settings.Topic = queueConfig.GetTopic();
                    settings.SourceId = Sprintf("%s_%lu_%lu", GetFQDNHostName(), ts, i);
                    settings.ReconnectOnFailure = true;
                    settings.CredentialsProvider = CredentialsProvider;
                    slot.Producer = PQLib->CreateProducer(settings, Logger, false);
                    slot.StartFuture = slot.Producer->Start();
                    slot.SeqNo = 0;
                }
                for (auto& slot: ProducerSlots) {
                    slot.StartFuture.Wait();
                    const auto& response = slot.StartFuture.GetValue().Response;
                    Y_VERIFY(!response.HasError(), "start producer error, code [%d], description [%s]",
                             response.GetError().GetCode(), response.GetError().GetDescription().c_str());
                    slot.SeqNo = response.GetInit().GetMaxSeqNo();
                }
            }

            TFuture<void> Send(const Message& message) override {
                auto& slot = ProducerSlots[RandomNumber<size_t>(ProducerSlots.size())];
                ++slot.SeqNo;
                return slot.Producer->Write(slot.SeqNo, TData(message.SerializeAsString(), TInstant::Now()))
                    .Apply([](const auto& f) {
                        const auto& response = f.GetValue().Response;
                        Y_VERIFY(!response.HasError(), "write error, code [%d], description [%s]",
                               response.GetError().GetCode(), response.GetError().GetDescription().c_str());
                        Y_VERIFY(response.HasAck());
                    });
            }

        private:
            static std::shared_ptr<ICredentialsProvider> CreateCredentialsProvider(const TPQConfig& queueConfig,
                                                                                   TIntrusivePtr<TCerrLogger> logger) {
                auto tvmSecret = GetEnv("RTLOG_TVM_SECRET");
                if (tvmSecret.empty()) {
                    const TFsPath secretPath(GetHomeDir() + "/.rtlog_tvm_secret");
                    if (secretPath.Exists()) {
                        tvmSecret = TFileInput(secretPath.GetPath()).ReadAll();
                    }
                }
                Y_VERIFY(!tvmSecret.empty(), "can't find tvm secret");
                const ui32 lbClientId = 2001059;
                NTvmAuth::NTvmApi::TClientSettings tvmSettings;
                tvmSettings.SetSelfTvmId(queueConfig.GetTvmClientId());
                tvmSettings.EnableServiceTicketsFetchOptions(tvmSecret, {{GetTvmPqServiceName(), lbClientId}});
                auto tvmLogger = MakeIntrusive<NTvmAuth::TCerrLogger>(TLOG_WARNING);
                auto tvmClient = std::make_shared<NTvmAuth::TTvmClient>(tvmSettings, tvmLogger);
                return CreateTVMCredentialsProvider(tvmClient, std::move(logger));
            }

        private:
            struct TProducerSlot {
                THolder<IProducer> Producer;
                TFuture<TProducerCreateResponse> StartFuture;
                ui64 SeqNo;
            };

        private:
            THolder<TPQLib> PQLib;
            TIntrusivePtr<TCerrLogger> Logger;
            std::shared_ptr<ICredentialsProvider> CredentialsProvider;
            TVector<TProducerSlot> ProducerSlots;
        };
    }

    THolder<IMessageSender> MakeLogbrokerMessageSender(const TPQConfig& queueConfig) {
        return MakeHolder<TLogbrokerMessageSender>(queueConfig);
    }
}
