#pragma once

#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/hollywood/library/scenarios/video/proto/video_scene_args.pb.h>
#include <google/protobuf/empty.pb.h>

namespace NAlice::NHollywoodFw::NVideo {

    inline constexpr TStringBuf CARD_DETAILS_SCENE = "card_details";
    inline constexpr TStringBuf CARD_DETAILS_THIN_SCENE = "card_details_thin";

    inline constexpr TStringBuf GET_CARD_DETAIL_FRAME = "alice.video.get_full_card_detail";
    inline constexpr TStringBuf GET_CARD_DETAIL_THIN_FRAME = "alice.video.get_thin_card_detail";

    struct TFrameGetCardDetails : public NHollywoodFw::TFrame {
        TFrameGetCardDetails(const NHollywoodFw::TRequest::TInput& input)
            : TFrame(input, GET_CARD_DETAIL_FRAME)
            , ContentId(this, "content_id")
            , ContentType(this, "content_type")
            , OntoId(this, "onto_id")
        {
        }
        NHollywoodFw::TOptionalSlot<TString> ContentId;
        NHollywoodFw::TOptionalSlot<TString> ContentType;
        NHollywoodFw::TOptionalSlot<TString> OntoId;
    };

    struct TFrameGetCardDetailsThin : public NHollywoodFw::TFrame {
        TFrameGetCardDetailsThin(const NHollywoodFw::TRequest::TInput& input)
            : TFrame(input, GET_CARD_DETAIL_THIN_FRAME)
            , ContentId(this, "content_id")
        {
        }
        NHollywoodFw::TOptionalSlot<TString> ContentId;
    };

    class TBaseCardDetailsScene : public TScene<TCardDetailSceneArgs> {
        public:
            // Card details base class with common methods
            TBaseCardDetailsScene(const TScenario* owner, TStringBuf sceneName)
                : TScene(owner, sceneName)
            {}

            inline TRetMain Main(const TCardDetailSceneArgs&, const TRunRequest&, TStorage&, const TSource&) const {
                return TReturnValueContinue(google::protobuf::Empty{});
            }

            inline TRetSetup ContinueSetup(const TCardDetailSceneArgs& args, const TContinueRequest& request, const TStorage&) const {
                TSetup setup(request);

                NHollywood::THttpProxyRequestBuilder httpRequestBuilder(args.GetPath(), request.GetRequestMeta(), request.Debug().Logger());
                httpRequestBuilder.AddHeaders(MakeDroidekaHeaders(request));
                // Authorization is neccessary for receiving WillWatch state from OTT
                if (!request.GetRequestMeta().GetOAuthToken().Empty()) {
                    httpRequestBuilder.SetUseOAuth();
                }
                setup.Attach(httpRequestBuilder.Build());
                LOG_INFO(request.Debug().Logger()) << "Packed HTTP " << args.GetPath() << " request to Droideka";
                return setup;
            }

            // separate implementation
            virtual TRetContinue Continue(
                const TCardDetailSceneArgs&,
                const TContinueRequest&,
                TStorage&,
                const TSource&) const = 0;

            // separate implementation
            virtual TRetResponse Render(
                const NScenarios::TCallbackDirective&,
                TRender&) const = 0;

            static THttpHeaders MakeDroidekaHeaders(const TContinueRequest& request);
    };


    class TVideoCardDetailScene: public TBaseCardDetailsScene {
    public:
        TVideoCardDetailScene(const TScenario* owner)
            : TBaseCardDetailsScene(owner, CARD_DETAILS_SCENE)
        {
            RegisterRenderer(&TVideoCardDetailScene::Render);
        }

        TRetContinue Continue(
            const TCardDetailSceneArgs&,
            const TContinueRequest&,
            TStorage&,
            const TSource&) const override;

        TRetResponse Render(
            const NScenarios::TCallbackDirective&,
            TRender&) const override;

        static TCardDetailSceneArgs MakeVideoCardDetailSceneArgs(const TFrameGetCardDetails& frame);
    private:
        static constexpr TStringBuf URI = "/api/v7/card_detail?";
        static constexpr TStringBuf ANALYTICS_ACTION_ID = "video_get_card_details";
        static constexpr TStringBuf CALLBACK_DIRECTIVE_NAME = "grpc_response.card_detail";
    };


    class TVideoCardDetailThinScene: public TBaseCardDetailsScene {
    public:
        TVideoCardDetailThinScene(const TScenario* owner)
            : TBaseCardDetailsScene(owner, CARD_DETAILS_THIN_SCENE)
        {
            RegisterRenderer(&TVideoCardDetailThinScene::Render);
        }

        TRetContinue Continue(
            const TCardDetailSceneArgs&,
            const TContinueRequest&,
            TStorage&,
            const TSource&) const override;

        TRetResponse Render(
            const NScenarios::TCallbackDirective&,
            TRender&) const override;

        static TCardDetailSceneArgs MakeVideoCardDetailThinSceneArgs(const TFrameGetCardDetailsThin& frame);
    private:
        static constexpr TStringBuf URI = "/api/v7/card_detail/thin?";
        static constexpr TStringBuf ANALYTICS_ACTION_ID = "video_get_thin_card_details";
        static constexpr TStringBuf CALLBACK_DIRECTIVE_NAME = "grpc_response.thin_card_detail";
    };

} // namespace NAlice::NHollywoodFw::NVideo
