#pragma once

#include <functional>
#include <memory>

namespace NAlice::NNlg {

struct TRenderContextData;
struct TRenderPhraseResult;
struct TRenderCardResult;

struct INlgRenderer;
using INlgRendererPtr = std::shared_ptr<INlgRenderer>;

class TEnvironment;
using TRegisterFunction = std::function<void(TEnvironment&)>;

struct ITranslationsContainer;
using ITranslationsContainerPtr = std::shared_ptr<ITranslationsContainer>;

} // namespace NAlice::NNlg
