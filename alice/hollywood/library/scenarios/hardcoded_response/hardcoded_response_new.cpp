#include <alice/hollywood/library/scenarios/hardcoded_response/proto/hardcoded_response.pb.h>

#include <alice/hollywood/library/framework/framework.h>

namespace NAlice::NHollywoodFw::NHardcodedResponse {

class THardcodedResponseScenario : public TScenario {
public:
    class THardcodedResponseFakeScene : public TScene<NHollywood::THardcodedResponseSceneArgs> {
    public:
        THardcodedResponseFakeScene(const TScenario* owner)
            : TScene(owner, "fake")
        {
        }
        TRetMain Main(const NHollywood::THardcodedResponseSceneArgs&, const TRunRequest&, TStorage&, const TSource&) const override {
            HW_ERROR("This scene never called");
        }
    };

    THardcodedResponseScenario()
        : TScenario("hardcoded_response")
    {
        Register(&THardcodedResponseScenario::Dispatch);
        RegisterScene<THardcodedResponseFakeScene>([this]() {
            RegisterSceneFn(&THardcodedResponseFakeScene::Main);
        });
    }
private:
    TRetScene Dispatch(const TRunRequest&,
                       const TStorage&,
                       const TSource&) const {
        // Switch to old scenario immediately
        return TReturnValueDo();
    }
};

HW_REGISTER(THardcodedResponseScenario);

} // namespace NAlice::NHollywoodFw
