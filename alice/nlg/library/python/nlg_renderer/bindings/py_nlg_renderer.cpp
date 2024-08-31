#include "py_nlg_renderer.h"
#include <alice/nlg/library/nlg_renderer/create_nlg_renderer_from_nlg_library_path.h>
#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>
#include <alice/library/util/rng.h>
#include <library/cpp/pybind/v2.h>
#include <library/cpp/json/yson/json2yson.h>
#include <library/cpp/yson/node/node_visitor.h>

namespace {

void ConvertPyObjectToJsonValue(PyObject* source, NJson::TJsonValue& target) {
    // There is not way too convert PyObject -> NJson::TJsonValue directly,
    // so we use NYT::TNode as intermediate format
    Y_ENSURE(source);

    auto node = NYT::TNode();
    Y_ENSURE(::NPyBind::FromPyObject(source, node));

    auto parserCallbacks = NJson::TParserCallbacks(target, true, true);
    auto consumer = NJson2Yson::TJsonBuilder(&parserCallbacks);
    auto visitor = NYT::TNodeVisitor(&consumer);
    visitor.Visit(node);
}

} // namespace

namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl {

class TPyAliceRng {
public:
    explicit TPyAliceRng(const TMaybe<ui64> seed);
    ::NAlice::IRng& Rng() {
        return Rng_;
    }
private:
    ::NAlice::TRng Rng_;
};

TPyAliceRng::TPyAliceRng(const TMaybe<ui64> seed)
    : Rng_(seed.Defined() ? TRng(seed.GetRef()) : TRng())
{
}

auto ExportAliceRng() {
    return ::NPyBind::TPyClass<TPyAliceRng, ::NPyBind::TPyClassConfigTraits<true>, TMaybe<ui64>>("AliceRng")
        .Complete();
}

} // namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl

DEFINE_TRANSFORMERS(::NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl::ExportAliceRng);

namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl {

class TPyRenderContextData {
public:
    explicit TPyRenderContextData(::NPyBind::TPyObjectPtr context, ::NPyBind::TPyObjectPtr form, ::NPyBind::TPyObjectPtr reqInfo);
    const TRenderContextData& RenderContextData() const {
        return RenderContextData_;
    }
private:
    TRenderContextData RenderContextData_;
};

TPyRenderContextData::TPyRenderContextData(::NPyBind::TPyObjectPtr context, ::NPyBind::TPyObjectPtr form, ::NPyBind::TPyObjectPtr reqInfo) {
    ConvertPyObjectToJsonValue(context.Get(), RenderContextData_.Context);
    ConvertPyObjectToJsonValue(form.Get(), RenderContextData_.Form);
    ConvertPyObjectToJsonValue(reqInfo.Get(), RenderContextData_.ReqInfo);
}

auto ExportRenderContextData() {
    return ::NPyBind::TPyClass<TPyRenderContextData, ::NPyBind::TPyClassConfigTraits<true>,
        ::NPyBind::TPyObjectPtr, ::NPyBind::TPyObjectPtr, ::NPyBind::TPyObjectPtr>("RenderContextData")
        .Complete();
}

} // namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl

DEFINE_TRANSFORMERS(::NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl::ExportRenderContextData);

namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl {

class TPyRenderPhraseResult {
public:
    explicit TPyRenderPhraseResult() = default;
    explicit TPyRenderPhraseResult(TRenderPhraseResult renderPhraseResult);

    const TString& GetText() const {
        return RenderPhraseResult_.Text;
    }
    void SetText(TString& value) {
        RenderPhraseResult_.Text = value;
    }
    const TString& GetVoice() const {
        return RenderPhraseResult_.Voice;
    }
    void SetVoice(TString& value) {
        RenderPhraseResult_.Voice = value;
    }
private:
    TRenderPhraseResult RenderPhraseResult_;
};

TPyRenderPhraseResult::TPyRenderPhraseResult(TRenderPhraseResult renderPhraseResult)
    : RenderPhraseResult_(std::move(renderPhraseResult))
{
}

auto ExportRenderPhraseResult() {
    return ::NPyBind::TPyClass<TPyRenderPhraseResult, ::NPyBind::TPyClassConfigTraits<true>>("RenderPhraseResult")
        .AsProperty("Text", &TPyRenderPhraseResult::GetText, &TPyRenderPhraseResult::SetText)
        .AsProperty("Voice", &TPyRenderPhraseResult::GetVoice, &TPyRenderPhraseResult::SetVoice)
        .Complete();
}

DEFINE_CONVERTERS(ExportRenderPhraseResult);

} // namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl

namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl {

class TPyNlgRenderer {
public:
    explicit TPyNlgRenderer();
    explicit TPyNlgRenderer(::NAlice::NNlg::INlgRendererPtr nlgRenderer);

    TPyRenderPhraseResult RenderPhrase(
        const TStringBuf templateId, const TStringBuf phraseId, const TStringBuf language,
        TPyAliceRng* rng, TPyRenderContextData* renderContextData) const;
private:
    ::NAlice::NNlg::INlgRendererPtr NlgRenderer_;
};

TPyNlgRenderer::TPyNlgRenderer() {
    ythrow yexception() << "Default .ctor should not be called. Use factory functions instead";
}

TPyNlgRenderer::TPyNlgRenderer(::NAlice::NNlg::INlgRendererPtr nlgRenderer)
    : NlgRenderer_(std::move(nlgRenderer))
{
    Y_ENSURE(NlgRenderer_, "Nlg renderer cannot be nullptr");
}

TPyRenderPhraseResult TPyNlgRenderer::RenderPhrase(
    const TStringBuf templateId, const TStringBuf phraseId, const TStringBuf language,
    TPyAliceRng* rng, TPyRenderContextData* renderContextData) const
{
    const ELanguage langEnum = LanguageByName(language);
    Y_ENSURE(langEnum > ELanguage::LANG_UNK && langEnum < ELanguage::LANG_MAX,
        "Unknown language " << language << " -> " << static_cast<int>(langEnum));

    Y_ENSURE(rng);
    Y_ENSURE(renderContextData);

    auto result = NlgRenderer_->RenderPhrase(templateId, phraseId, langEnum, rng->Rng(), renderContextData->RenderContextData());
    return TPyRenderPhraseResult(std::move(result));
}

auto ExportNlgRenderer() {
    return ::NPyBind::TPyClass<TPyNlgRenderer, ::NPyBind::TPyClassConfigTraits<true>>("NlgRenderer")
        .Def("render_phrase", &TPyNlgRenderer::RenderPhrase)
        .Complete();
}

DEFINE_CONVERTERS(ExportNlgRenderer);

} // namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl


namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl {

TPyNlgRenderer PyCreateNlgRendererFromNlgLibraryPath(const TStringBuf nlgLibraryPath, TPyAliceRng* rng) {
    Y_ENSURE(rng);
    auto nlgRenderer = ::NAlice::NNlg::CreateNlgRendererFromNlgLibraryPath(nlgLibraryPath, rng->Rng());
    return TPyNlgRenderer(std::move(nlgRenderer));
}

} // namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings::NImpl

namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings {

void InitNlgRendererCreateFunctions() {
    DefFunc("create_nlg_renderer_from_nlg_library_path", NImpl::PyCreateNlgRendererFromNlgLibraryPath);
}

void InitNlgRendererClasses() {
    NImpl::ExportAliceRng();
    NImpl::ExportRenderContextData();
    NImpl::ExportRenderPhraseResult();
    NImpl::ExportNlgRenderer();
}

} // namespace NAlice::NNlg::NLibrary::NPython::NNlgRenderer::NBindings
