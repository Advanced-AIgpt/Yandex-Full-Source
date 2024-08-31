#pragma once

#include "news_fast_data.h"

#include <alice/hollywood/library/framework/framework.h>

#include <alice/hollywood/library/scenarios/news/proto/news.pb.h>

namespace NAlice::NHollywoodFw {

class TNewsScenarioDispatch : public TScenario {
public:
    //
    // TODO: This is a temporary fake scene required to start new dispatcher
    // It will be converted to actual scene outside dispatcher class with future refactor
    //
    class TNewsFakeScene : public TScene<NHollywood::TNewsData> {
    public:
        TNewsFakeScene(const TScenario* owner)
            : TScene(owner, "fake_scene")
        {
        }
        TRetSetup MainSetup(const NHollywood::TNewsData&, const TRunRequest&, const TStorage&) const override {
            return TReturnValueDo();
        }
        TRetMain Main(const NHollywood::TNewsData&, const TRunRequest&, TStorage&, const TSource&) const override {
            HW_ERROR("Impossible here");
        }
    };

    TNewsScenarioDispatch()
        : TScenario("news")
    {
        Register(&TNewsScenarioDispatch::Dispatch);
        RegisterScene<TNewsFakeScene>([this]() {
            RegisterSceneFn(&TNewsFakeScene::MainSetup);
            RegisterSceneFn(&TNewsFakeScene::Main);
        });

        AddFastData<NHollywood::TNewsFastDataProto, NHollywood::TNewsFastData>("news/news.pb");
        SetApphostGraph(ScenarioRequest() >>
                        NodeRun("prepare") >>
                        NodeMain("render") >>
                        ScenarioResponse());
    }

    TRetScene Dispatch(const TRunRequest& request,
                       const TStorage&,
                       const TSource&) const {
        LOG_DEBUG(request.Debug().Logger()) << "New flow (Dispatch) receives control!";
        return TReturnValueScene<TNewsFakeScene>(NHollywood::TNewsData{});
    }
};

}
