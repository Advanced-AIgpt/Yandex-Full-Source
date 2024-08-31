#include "env.h"

#include "exceptions.h"
#include "text_stream.h"
#include <util/string/join.h>

namespace NAlice::NNlg {

namespace {

constexpr TStringBuf RAW_FORM_KEY = "raw_form";

TString GetSlotNameOrThrow(const TValue::TDict& slotDict) {
    for (const TStringBuf key : {TStringBuf("slot"), TStringBuf("name")}) {
        if (const auto* ptr = slotDict.FindPtr(key)) {
            if (!ptr->IsString()) {
                ythrow TTypeError() << "Raw form slot's name must be a string, got "
                                    << ptr->GetTypeName() << " (key = \"" << key << "\")";
            }
            const auto name = ptr->GetString().GetStr();
            if (name == RAW_FORM_KEY) {
                ythrow TValueError() << "Slot name \"" << RAW_FORM_KEY << "\" is reserved";
            }
            return name;
        }
    }

    ythrow TValueError() << "Raw form slot must have either the \"slot\" or the \"name\" key";
}

TValue GetSlotValueOrThrow(const TValue::TDict& slotDict) {
    if (const auto* ptr = slotDict.FindPtr(TStringBuf("value"))) {
        return *ptr;
    }

    ythrow TValueError() << "Raw form slot must have the \"value\" key";
}

std::pair<TString, TString> BuildLocalizedKey(const TStringBuf templateId, const TStringBuf macroId, const ELanguage language) {
    auto localizedTemplateId = static_cast<TString>(Join('_', templateId, IsoNameByLanguage(language)));
    return std::make_pair(std::move(localizedTemplateId), TString(macroId));
}

std::pair<TString, TString> BuildUnlocalizedKey(const TStringBuf templateId, const TStringBuf macroId) {
    return std::make_pair(TString(templateId), TString(macroId));
}

}  // namespace

TValue TransformForm(TValue&& rawForm) {
    if (!rawForm.IsDict()) {
        ythrow TTypeError() << "Raw form must be a dict, got " << rawForm.GetTypeName();
    }

    auto& rawFormDict = rawForm.GetMutableDict();

    TValue::TDict slotsByName;
    TValue::TDict result;
    if (const auto* slotsPtr = rawFormDict.FindPtr(TStringBuf("slots"))) {
        if (!slotsPtr->IsList()) {
            ythrow TTypeError() << "Raw form slots must be a list, if present, got " << slotsPtr->GetTypeName();
        }

        const auto& slots = slotsPtr->GetList();
        slotsByName.reserve(slots.size());
        result.reserve(slots.size() + 1);  // one for each slot, one for rawForm itself

        for (const auto& slot : slots) {
            if (!slot.IsDict()) {
                ythrow TTypeError() << "Raw form slots elements must be dicts, got " << slot.GetTypeName();
            }

            const auto& slotDict = slot.GetDict();
            TString name = GetSlotNameOrThrow(slotDict);
            auto value = GetSlotValueOrThrow(slotDict);
            slotsByName[name] = slot;
            result[name] = std::move(value);  // XXX(a-square): removed duplication check here
        }
    }

    rawFormDict[TStringBuf("slots_by_name")] = TValue::Dict(std::move(slotsByName));
    result[RAW_FORM_KEY] = std::move(rawForm);
    return TValue::Dict(std::move(result));
}

const TGlobals& TEnvironment::GetGlobals(TStringBuf module) const {
    const auto* globals = Globals.FindPtr(module);
    if (!globals) {
        ythrow TImportError() << "Module not registered: " << module;
    }
    return *globals;
}

TGlobals& TEnvironment::InitializeGlobals(const TMutableCallCtx& ctx, TStringBuf module) {
    auto* globals = Globals.FindPtr(module);
    if (!globals) {
        globals = &Globals[module];
        (*Inits.at(module))(ctx, *globals);
    }

    return *globals;
}

void TEnvironment::InitializeAllGlobals(IRng& rng) {
    TCallStack callStack;
    callStack.push_back({TStringBuf("<environment init>"), {}, Nothing()});
    TMutableCallCtx ctx{*this, rng, callStack, TStringBuf()};

    try {
        for (const auto& [module, callback] : Inits) {
            InitializeGlobals(ctx, module);
        }

        for (auto& [module, globals] : Globals) {
            globals.Freeze();
        }
    } catch (...) {
        TRuntimeError::ThrowWrapped(std::move(callStack));
    }
}

TPhraseOutput TEnvironment::RenderPhrase(const TStringBuf templateId, const TStringBuf phrase, const ELanguage language, IRng& rng,
                                         TPhraseCardParams&& params) const {
    params.Context.GetMutableDict()[TStringBuf("phrase_id")] = TValue::String(phrase);
    const auto phrasePtr = GetPhrase(templateId, phrase, language);
    Y_ENSURE_EX(phrasePtr, TPhraseNotFoundError() <<
        "templateId = " << templateId << ", phrase = " << phrase << ", language = " << IsoNameByLanguage(language));
    const auto result = RenderMacro(templateId, phrasePtr, language, TStringBuf("<render phrase>"), rng, std::move(params));
    return PostprocessPhrase(result);
}

NJson::TJsonValue TEnvironment::RenderCard(const TStringBuf templateId, const TStringBuf card, const ELanguage language, IRng& rng,
                                           TPhraseCardParams&& params, const bool reduceWhitespace) const {
    params.Context.GetMutableDict()[TStringBuf("card_id")] = TValue::String(card);
    const auto cardPtr = GetCard(templateId, card, language);
    Y_ENSURE_EX(cardPtr, TCardNotFoundError() <<
        "templateId = " << templateId << ", card = " << card << ", language = " << IsoNameByLanguage(language));
    const auto result = RenderMacro(templateId, cardPtr, language, TStringBuf("<render card>"), rng, std::move(params));
    return PostprocessCard(result, reduceWhitespace);
}

TText TEnvironment::RenderMacro(const TStringBuf templateId, const TMacroZeroArgsPtr macro, const ELanguage language,
                                const TStringBuf frameName, IRng& rng, TPhraseCardParams&& params) const {
    const TStringBuf languageStr = IsoNameByLanguage(language);
    params.Context.GetMutableDict()["lang"] = TValue::String(languageStr);

    TGlobals extraGlobals;
    extraGlobals.ResolveStore(TStringBuf("context")) = std::move(params.Context);
    extraGlobals.ResolveStore(TStringBuf("form")) = TransformForm(std::move(params.Form));
    extraGlobals.ResolveStore(TStringBuf("req_info")) = std::move(params.ReqInfo);

    // XXX(a-square): legacy, intent-to-template correspondence only holds in VINS
    extraGlobals.ResolveStore(TStringBuf("intent_name")) = TValue::String(Join('_', templateId, languageStr));

    TGlobalsChain globalsChain{nullptr, &extraGlobals};

    TCallStack callStack;
    callStack.push_back({frameName, {}, {}});

    TCallCtx ctx{*this, rng, callStack, languageStr};

    try {
        TText result;
        TTextOutput out(result);
        (*macro)(ctx, /* caller = */ nullptr, &globalsChain, out);

        return result;
    } catch (...) {
        TRuntimeError::ThrowWrapped(std::move(callStack));
    }
}

TEnvironment::TMacroZeroArgsPtr TEnvironment::GetPhrase(const TStringBuf templateId, const TStringBuf phrase, const ELanguage language) const {
    if (const auto* result = Phrases.FindPtr(BuildLocalizedKey(templateId, phrase, language))) {
        return *result;
    }
    if (const auto* result = Phrases.FindPtr(BuildUnlocalizedKey(templateId, phrase))) {
        return *result;
    }
    return nullptr;
}

TEnvironment::TMacroZeroArgsPtr TEnvironment::GetCard(const TStringBuf templateId, const TStringBuf card, const ELanguage language) const {
    if (const auto* result = Cards.FindPtr(BuildLocalizedKey(templateId, card, language))) {
        return *result;
    }
    if (const auto* result = Cards.FindPtr(BuildUnlocalizedKey(templateId, card))) {
        return *result;
    }
    return nullptr;
}

} // namespace NAlice::NNlg
