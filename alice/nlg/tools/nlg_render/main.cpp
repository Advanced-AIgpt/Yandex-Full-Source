#include <alice/library/util/rng.h>
#include <alice/nlg/library/nlg_renderer/create_nlg_renderer_from_register_function.h>
#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>
#include <alice/nlg/tools/nlg_render/proto/options.pb.h>
#include <library/cpp/getoptpb/getoptpb.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/file.h>
#include <util/stream/str.h>

namespace {

NJson::TJsonValue ParseJson(const TStringBuf json) {
    return json ? NJson::ReadJsonFastTree(json) : NJson::TJsonMap();
}

void RegisterNlg(::NAlice::NNlg::TEnvironment& env) {
    Y_UNUSED(env);
    // Register your NLG here
}

void RenderMain(const TOptions& options) {
    NAlice::TRng rng;

    const auto nlg = ::NAlice::NNlg::CreateNlgRendererFromRegisterFunction(RegisterNlg, rng);

    const auto& render = options.GetRender();
    Y_ENSURE(render.HasPhrase() ^ render.HasCard());

    const auto& intent = render.GetIntent();

    auto renderContextData = ::NAlice::NNlg::TRenderContextData{
        .Context = ParseJson(render.GetContext()),
        .Form = ParseJson(render.GetForm()),
        .ReqInfo = ParseJson(render.GetReqInfo()),
    };

    const auto lang = ELanguage::LANG_RUS;

    if (const auto& phrase = render.GetPhrase()) {
        const auto result = nlg->RenderPhrase(intent, phrase, lang, rng, std::move(renderContextData));
        Cout << "Text: " << result.Text << Endl;
        Cout << "Voice: " << result.Voice << Endl;
    } else {
        const auto& card = render.GetCard();
        Y_ENSURE(card);
        const auto result = nlg->RenderCard(intent, card, lang, rng, std::move(renderContextData));
        NJson::WriteJson(&Cout, &result.Card, /* formatOutput = */ true);
        Cout << Endl;
    }
}

void DispatchMain(const TOptions& options) {
    if (options.HasRender()) {
        RenderMain(options);
    } else {
        ythrow yexception() << "Unknown command";
    }
}

}  // namespace

int main(int argc, const char* argv[]) {
    try {
        TOptions options = NGetoptPb::GetoptPbOrAbort(argc, argv);
        DispatchMain(options);
        return 0;
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
