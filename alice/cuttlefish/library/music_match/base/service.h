#pragma once

#include "callbacks_handler.h"
#include "fake.h"

#ifndef DLOG
#include <alice/cuttlefish/library/logging/dlog.h>
#endif

#include <alice/cuttlefish/library/protos/audio.pb.h>
#include <alice/cuttlefish/library/logging/apphost_log.h>

#include <apphost/api/service/cpp/service.h>

#include <util/generic/ptr.h>
#include <util/stream/str.h>

namespace NAlice::NMusicMatch {
    class TService {
    public:
        virtual ~TService() = default;

    public:
        class TRequestProcessor: public TThrRefBase {
        public:
            TRequestProcessor(TService& service)
                : Service_(service)
            {
            }

            // Main entry-point for processing next message from apphost graph
            virtual void ProcessInput(NAppHost::TServiceContextPtr, NThreading::TPromise<void> promise);

        protected:
            // Return false if processing request finished
            virtual bool ProcessContextLoadResponseItems(NAppHost::TServiceContextPtr& ctx);
            virtual bool ProcessSessionContextItems(NAppHost::TServiceContextPtr& ctx);
            virtual bool ProcessTvmServiceTicket(NAppHost::TServiceContextPtr& ctx);
            virtual bool ProcessCuttlefishAudioItems(NAppHost::TServiceContextPtr& ctx);
            virtual bool ProcessUnknownRequestItems(NAppHost::TServiceContextPtr& ctx);

            virtual bool OnContextLoadResponse(const NProtobuf::TContextLoadResponse& contextLoadResponse);
            virtual bool OnSessionContext(const NProtobuf::TSessionContext& sessionContext);
            virtual bool OnTvmServiceTicket(const TString& tvmServiceTicket);
            virtual bool OnCuttlefishAudio(const NAliceProtocol::TAudio& audio);
            virtual bool OnCuttlefishSpotterAudioChunk(const NAliceProtocol::TAudioChunk& audioChunk);
            virtual bool OnInitRequest(const NProtobuf::TInitRequest& initRequest);
            virtual bool OnStreamRequest(const NProtobuf::TStreamRequest& streamRequest);

            virtual void InitializeMusicMatch(TIntrusivePtr<NMusicMatch::TInterface::TCallbacks> callbacks) {
                // By default create fake music_match (emulator for testing/debug)
                MusicMatchInitialized_ = true;
                MusicMatch_.Reset(new NMusicMatch::TFake(callbacks));
            }
            virtual TIntrusivePtr<NMusicMatch::TInterface::TCallbacks> CreateMusicMatchCallbacks(NAppHost::TServiceContextPtr& ctx) {
                return new TCallbacksHandler(ctx);
            }
            virtual void OnAppHostEmptyInput() {
                DLOG("RequestProcessor.OnAppHostEmptyInput");
            }
            virtual void OnAppHostClose(NThreading::TPromise<void>& promise, bool requestCancelled = false);
            virtual void OnUnknownItemType(const TString& tag, const TString& type) {
                DLOG("RequestProcessor.OnUnknownItemType tag=" << tag << " type=" << type);

                Y_UNUSED(tag);
                Y_UNUSED(type);
            }
            virtual void OnWarning(const TString& warning) {
                DLOG("RequestProcessor.OnWarning " << warning);
                Y_UNUSED(warning);
            }
            virtual void OnUnknownCuttlefishAudioMessageType(const NAliceProtocol::TAudio& audio) {
                DLOG("RequestProcessor.OnUnknownCuttlefishAudio " << audio.ShortUtf8DebugString());
                Y_UNUSED(audio);
            }
            virtual void OnError(const TString& error);

        protected:
            TService& Service_;

            bool MusicMatchInitialized_ = false;
            bool HasContextLoadResponse_ = false;
            bool HasSessionContext_ = false;
            bool HasTvmServiceTicket_ = false;
            bool HasInitRequest_ = false;
            bool HasCuttlefishAudioBeginStream_ = false;
            bool HasCuttlefishAudioEndStream_ = false;
            bool ProcessSpotter_ = false;

            TIntrusivePtr<NMusicMatch::TInterface> MusicMatch_;
        };

        static TString Path() {
            return "/music_match";
        }

        virtual TIntrusivePtr<TRequestProcessor> CreateProcessor(NAppHost::IServiceContext&, NAlice::NCuttlefish::TRtLogClient*) {
            return MakeIntrusive<TRequestProcessor>(*this);
        }
    };
}
