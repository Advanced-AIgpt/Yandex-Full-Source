#include "translate.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/scheduler/scheduler.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <dict/dictutil/dictutil.h>
#include <dict/dictutil/scripts.h>
#include <dict/dictutil/str.h>
#include <dict/dictutil/text_reader.h>
#include <dict/lang_detector/libs/core/lang_detector.h>
#include <dict/libs/alphabet/alphabet.h>
#include <dict/libs/crf/model.h>
#include <dict/libs/fst/lib/fst.h>
#include <dict/libs/langmodels/char_trigram_model/model.h>
#include <dict/misspell/spellchecker/lib/kernel/query_parser.h>
#include <dict/mt/libs/libmt/token.h>
#include <dict/translit/libs/translit/translit.h>

#include <kernel/lemmer/core/language.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/token/charfilter.h>

#include <util/generic/xrange.h>
#include <util/memory/blob.h>
#include <util/random/mersenne.h>
#include <util/random/shuffle.h>
#include <util/stream/file.h>
#include <util/stream/mem.h>
#include <util/string/split.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/strip.h>
#include <util/system/tempfile.h>

namespace NBASS {

namespace {

// scenarios
constexpr TStringBuf TRANSLATE_SCENARIO = "personal_assistant.scenarios.translate";
constexpr TStringBuf TRANSLATE_ELLIPSIS_SCENARIO = "personal_assistant.scenarios.translate__ellipsis";
constexpr TStringBuf TRANSLATE_QUICKER_SCENARIO = "personal_assistant.scenarios.translate__quicker";
constexpr TStringBuf TRANSLATE_SLOWER_SCENARIO = "personal_assistant.scenarios.translate__slower";

// Error & Attention Constants
constexpr TStringBuf ERROR_FAILED_TO_TRANSLATE = "failed_to_translate";
constexpr TStringBuf ERROR_FAILED_TO_PROCESS = "failed_to_process";
constexpr TStringBuf ERROR_WRONG_LANGUAGE = "wrong_language_translate";
constexpr TStringBuf ERROR_NOT_SUPPORTED_LANGUAGE = "not_supported_language";
constexpr TStringBuf ATTENTION_OPEN_SEARCH = "open_search";
constexpr TStringBuf ATTENTION_OPEN_TRANSLATE = "open_translate";
constexpr TStringBuf ATTENTION_RESULT_CARD = "result_card";
constexpr TStringBuf ATTENTION_SWEAR_UNLIMITED = "swear_unlimited";
constexpr TStringBuf ATTENTION_SWEAR_CHILDREN = "swear_children";
constexpr TStringBuf ATTENTION_NO_VOICE = "no_voice";
constexpr TStringBuf ATTENTION_REPEAT_VOICE = "repeat_voice";
constexpr TStringBuf YANDEX_TRANSLATE_TEXT = "Яндекс.Переводчик — 95 языков\ntranslate.yandex.ru";
constexpr TStringBuf GOOGLE_TRANSLATE_TEXT = "Google Переводчик — быстрый перевод на 103 языка и обратно\ntranslate.google.ru";

// other constants
constexpr int DICT_MAX_WORDS = 3;
constexpr int DICT_MAX_CHARS = 100;
constexpr size_t POS_MAX_NUM = 2;
constexpr size_t TR_MAX_NUM = 2;
constexpr int MEAN_MAX_NUM = 2;
constexpr int SUGGESTS_NUM = 3;
constexpr double DEFAULT_SPEED = 0.9;
constexpr double LOWER_SPEED_BOUND = 0.6;
constexpr double UPPER_SPEED_BOUND = 1.2;
constexpr double SPEED_STEP = 0.1;
constexpr TStringBuf UNSUPPORTED = "unsupported ";

const THashSet<ELanguage>& GetNumeralTransducerLangs() {
    static const THashSet<ELanguage> langs = {
        LANG_GER, LANG_ENG, LANG_EPO, LANG_SPA, LANG_FRE, LANG_ITA, LANG_GEO, LANG_KIR,
        LANG_RUS, LANG_TUR, LANG_TAT, LANG_UKR };
    return langs;
}

// Function to return set of languages that ASR could not recognize at all
const THashSet<ELanguage>& GetWrongSuggestLangs() {
    static const THashSet<ELanguage> langs = {
        LANG_AMH, LANG_WEL, LANG_GLG, LANG_KAN, LANG_LAO, LANG_MAL, LANG_PAP, LANG_CEB, LANG_SUN,
        LANG_GUJ, LANG_PAN, LANG_SWA, LANG_URD, LANG_AFR, LANG_YID
    };
    return langs;
}

// Functions for obtaining different set of languages for generating voice representations
const THashSet<ELanguage>& GetTranslitLangs() {
    static const THashSet<ELanguage> langs = {
        LANG_CHI, LANG_KOR, LANG_THA, LANG_GEO, LANG_GRE, LANG_SJN, LANG_ARM, LANG_AMH, LANG_JPN,
        LANG_TAM, LANG_BEN, LANG_GUJ, LANG_HIN, LANG_KAN, LANG_MAL, LANG_MAR, LANG_NEP, LANG_PAN,
        LANG_SIN, LANG_TEL, LANG_YID, LANG_HEB, LANG_ARA, LANG_MYA, LANG_KHM, LANG_LAO, LANG_PER,
        LANG_URD };
    return langs;
}

const THashSet<ELanguage>& GetPhonemeLangs() {
    static const THashSet<ELanguage> langs = {
        LANG_SPA, LANG_FRE, LANG_GER, LANG_ITA, LANG_POR, LANG_CZE };
    return langs;
}

const THashSet<ELanguage>& GetSynthesLangs() {
    static const THashSet<ELanguage> langs = { LANG_ENG, LANG_UKR, LANG_TUR };
    return langs;
}

}  // anonymous namespace


// static fields
THolder<const NDict::NLangDetector::ILangDetector> TTranslateFormHandler::LangDetector;
THashSet<TUtf16String> TTranslateFormHandler::LanguageStems;
THashMap<TUtf16String, ELanguage> TTranslateFormHandler::ShortLanguageStems;
THashMap<TUtf16String, TUtf16String> TTranslateFormHandler::StopLanguageStems;
THashMap<ELanguage, THolder<NDict::NFst::TTransducer>> TTranslateFormHandler::NumeralTransducers;
THolder<const TCharTrigramModel> TTranslateFormHandler::RuTrigrams;
THolder<const ITransliterator> TTranslateFormHandler::DefaultBeamSearchTransliterator;
THolder<const ITransliterator> TTranslateFormHandler::NonStandardBeamSearchTransliterator;
THashMap<ELanguage, TString> TTranslateFormHandler::LanguageNames;
TVector<ELanguage> TTranslateFormHandler::Languages;
THashSet<TUtf16String> TTranslateFormHandler::SwearWords;


void TTranslateFormHandler::Register(THandlersMap* handlers) {
    handlers->GlobalCtx.Scheduler().Schedule([]() { Init(); return TDuration(); });
    auto handler = [] () {
        return MakeHolder<TTranslateFormHandler>();
    };
    handlers->emplace(TRANSLATE_SCENARIO, handler);
    handlers->emplace(TRANSLATE_ELLIPSIS_SCENARIO, handler);
    handlers->emplace(TRANSLATE_QUICKER_SCENARIO, handler);
    handlers->emplace(TRANSLATE_SLOWER_SCENARIO, handler);
}

void TTranslateFormHandler::Init() {
    LoadLanguageDetector();
    LoadNumeralTransducers();
    LoadTransliterators();
    LoadLanguageStems();
    LoadLanguages();
    LoadSwear();
}

TResultValue TTranslateFormHandler::SetAsResponse(TContext& ctx, TStringBuf query) {
    TIntrusivePtr<TContext> newContext = ctx.SetResponseForm(TRANSLATE_SCENARIO, false /* setCurrentFormAsCallback */);
    Y_ENSURE(newContext);
    newContext->CreateSlot("query", "string", true, query);
    return TResultValue();
}

void TTranslateFormHandler::LoadLanguageDetector() {
    LOG(INFO) << "Loading language detector module" << Endl;
    TString resource = NResource::Find("lang_detector_model");
    LangDetector.Reset(
        NDict::NLangDetector::CreateVowpalWabbitLangDetector(TBlob::FromString(resource)).release());
}

void TTranslateFormHandler::LoadLanguageStems() {
    LOG(INFO) << "Loading file with language stems" << Endl;
    const TString stemsResource = NResource::Find("lang_stems");
    TMemoryInput stemsInp(stemsResource);
    for (TString line; stemsInp.ReadLine(line); ) {
        LanguageStems.emplace(UTF8ToWide(line));
    }
}

void TTranslateFormHandler::LoadLanguages() {
    LOG(INFO) << "Loading file with languages" << Endl;
    const TString resource = NResource::Find("languages");
    TMemoryInput inp(resource);
    for (TString line; inp.ReadLine(line); ) {
        TStringBuf langStem, langCode, langName;
        StringSplitter(line).Split('\t').CollectInto(&langStem, &langCode, &langName);
        if (langCode == "STOP") {
            StopLanguageStems.emplace(UTF8ToWide(langStem), UTF8ToWide(langName));
        } else {
            ELanguage lang = LanguageByNameOrDie(langCode);
            ShortLanguageStems.emplace(UTF8ToWide(langStem), lang);
            LanguageNames[lang] = langName;
        }
    }
    for (const auto& lang : LanguageNames) {
        Languages.push_back(lang.first);
    }
}

void TTranslateFormHandler::LoadNumeralTransducers() {
    LOG(INFO) << "Loading numeral transducers" << Endl;
    const TString suffix("_numeral_yandex");
    for (ELanguage lang : GetNumeralTransducerLangs()) {
        TString rawData = NResource::Find(IsoNameByLanguage(lang) + suffix);
        NumeralTransducers[lang] = MakeHolder<NDict::NFst::TTransducer>(TBlob::FromString(rawData));
    }
}

void TTranslateFormHandler::LoadTransliterators() {
    LOG(INFO) << "Loading transliteration models" << Endl;
    TBeamSearchTranslitParams params;
    params.DistanceMultiplier = 5.0;
    params.UnseenPenalty = 10.0;
    params.BeamSize = 20;
    params.BeginEndMarkers = u"^$";

    // memory input streams for transfemes should live only during creating transliterators
    TString resource = NResource::Find("en-ru.translit");
    TMemoryInput translitInput(resource);
    params.TransfemesStream = &translitInput;

    TString rawData = NResource::Find("rus.3gr");
    RuTrigrams.Reset(TCharTrigramModel::Load(TBlob::FromString(rawData)));
    params.Trigrams = RuTrigrams.Get();
    DefaultBeamSearchTransliterator.Reset(CreateBeamSearchTransliterator(params).release());

    resource = NResource::Find("en-ru.nonstandard.translit");
    TMemoryInput nonStandardTranslitInput(resource);
    params.TransfemesStream = &nonStandardTranslitInput;
    NonStandardBeamSearchTransliterator.Reset(CreateBeamSearchTransliterator(params).release());
}

void TTranslateFormHandler::LoadSwear() {
    LOG(INFO) << "Loading swear" << Endl;
    const TString resource = NResource::Find("swear");
    TMemoryInput inp(resource);
    for (TUtf16String line; inp.ReadLine(line); ) {
        SwearWords.insert(line);
    }
}

TResultValue TTranslateFormHandler::Do(TRequestHandler& request) {
    TContext& ctx = request.Ctx();

    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::TRANSLATE);

    // In case of repeating voice scenario -> just return and set attention
    TContext::TSlot* voiceRepeatSlot = ctx.GetSlot("repeat_voice", "bool");
    if (!IsSlotEmpty(voiceRepeatSlot) && voiceRepeatSlot->Value.GetBool()) {
        return ProcessVoiceRepeatScenario(ctx);
    }

    TSegmentation segmentation;
    if (!SegmentQuery(ctx, segmentation)) {
        return AddSearchOpenSuggests(ctx);
    }

    TString result, cardName;
    NSc::TValue card, factCard;
    if (!TranslateQuery(segmentation, result, cardName, card, factCard, ctx)) {
        return AddSearchOpenSuggests(ctx);
    }

    // Check for swear words (in src or dst).
    // Distinguish between children/medium mode (where we should block any swear results)
    // and mode 'without' any limits (where we can show swear results).
    TUtf16String lowerResult = ToLower(segmentation.To, UTF8ToWide(result));
    bool isUnlimitedMode = (ctx.GetContentRestrictionLevel() == EContentRestrictionLevel::Without);
    bool addSwearTextCard = false;
    if (IsSwear(segmentation.TextToTranslate) || IsSwear(lowerResult)) {
        if (isUnlimitedMode) {
            ctx.AddAttention(ATTENTION_SWEAR_UNLIMITED);
            addSwearTextCard = true;
        } else {
            ctx.AddAttention(ATTENTION_SWEAR_CHILDREN);
            return AddSearchOpenSuggests(ctx);
        }
    }

    FillPronunciation(segmentation, WideToUTF8(lowerResult), cardName, card, ctx);
    if (ctx.ClientFeatures().SupportsDivCards()) {
        ctx.AddAttention(ATTENTION_RESULT_CARD);
        if (addSwearTextCard) {
            ctx.AddTextCardBlock("swear_unlimited");
        } else if (segmentation.From != LANG_RUS) {
            ctx.AddTextCardBlock("foreign_translate");
        }
        ctx.AddDivCardBlock(cardName, card);
        if (!factCard.IsNull()) {
            ctx.AddDivCardBlock("fact_card", factCard);
        }
        ctx.AddSearchSuggest();
    }

    return TResultValue();
}

TResultValue TTranslateFormHandler::ProcessVoiceRepeatScenario(TContext& ctx) const {
    ctx.AddAttention(ATTENTION_REPEAT_VOICE);
    // re-add all suggests.
    ctx.AddSuggest("translate__repeat", "");
    TContext::TSlot* slotSuggestLangs = ctx.GetSlot("suggest_langs", "string");
    if (!IsSlotEmpty(slotSuggestLangs)) {
        TString suggestLangs(slotSuggestLangs->Value.GetString());
        for (const auto& it : StringSplitter(suggestLangs).Split(' ').SkipEmpty()) {
            ctx.AddSuggest("translate__change_language", NSc::TValue(it.Token()));
        }
    }
    TContext::TSlot* slotSuggestSpeed(ctx.GetSlot("suggest_speed", "string"));
    if (!IsSlotEmpty(slotSuggestSpeed)) {
        TString suggestSpeed(slotSuggestSpeed->Value.GetString());
        for (const auto& it : StringSplitter(suggestSpeed).Split(' ').SkipEmpty()) {
            if (it.Token() == "slower") {
                ctx.AddSuggest("translate__slower", "");
            } else if (it.Token() == "quicker") {
                ctx.AddSuggest("translate__quicker", "");
            }
        }
    }
    ctx.AddSearchSuggest();
    ctx.AddStopListeningBlock();
    return TResultValue();
}

bool TTranslateFormHandler::SegmentQuery(TContext& ctx,
                                         TSegmentation& segmentation) const {
    if (ctx.FormName() == TRANSLATE_SCENARIO) {
        return SegmentQueryInMainScenario(ctx, segmentation);
    }
    return SegmentQueryInEllipsisScenario(ctx, segmentation);
}

bool TTranslateFormHandler::SegmentQueryInMainScenario(TContext& ctx,
                                                       TSegmentation& segmentation) const {
    TString service = FindTranslateServiceInQuery(ctx.Meta().Utterance().Get());

    TUtf16String text = GetSlotValueIfExists(ctx, "text");
    TUtf16String inputLangSrc = GetSlotValueIfExists(ctx, "input_lang_src");
    TUtf16String inputLangDst = GetSlotValueIfExists(ctx, "input_lang_dst");

    TString unsupportedLangSrc, unsupportedLangDst;
    TString error = ParseQuerySegmentation(
            text, inputLangSrc, inputLangDst, segmentation, unsupportedLangSrc, unsupportedLangDst);

    // We should add error block in case of all errors.
    if (error != ERROR_FAILED_TO_PROCESS) {
        if (!DetectLangAndTranslit(segmentation.TextToTranslate, segmentation, ctx)) {
            error = ERROR_WRONG_LANGUAGE;
        }
    }

    if (error) {
        FillSlots(unsupportedLangSrc, unsupportedLangDst, service, segmentation, ctx);
        if (error == ERROR_NOT_SUPPORTED_LANGUAGE && ctx.ClientFeatures().SupportsDivCards()) {
            AddOpenSearchCommand(ctx.Meta().Utterance().Get(), ctx);
        } else {
            ctx.AddErrorBlock(TError(TError::EType::TRANSLATEERROR, error));
        }
        return false;
    }
    if (!segmentation.TextToTranslate) {
        // Empty text to translate -> add open translate uri if possible.
        FillSlots(unsupportedLangSrc, unsupportedLangDst, service, segmentation, ctx);
        AddOpenTranslateCommand(ForceTranslateService(service), ERROR_FAILED_TO_PROCESS, segmentation, ctx);
        return false;
    }
    FillSlots(unsupportedLangSrc, unsupportedLangDst, service, segmentation, ctx);
    AddOpenTranslateCommand(service, error, segmentation, ctx);
    return (!service || !ctx.ClientFeatures().SupportsDivCards()) && !error;
}

bool TTranslateFormHandler::SegmentQueryInEllipsisScenario(TContext& ctx,
                                                           TSegmentation& segmentation) const {
    TContext::TSlot* langSrcQuery = ctx.GetSlot("lang_src", "string");
    TContext::TSlot* langDstQuery = ctx.GetSlot("lang_dst", "string");
    TContext::TSlot* serviceQuery = ctx.GetSlot("translate_service", "string");
    if (IsSlotEmpty(langSrcQuery) || IsSlotEmpty(langDstQuery) || IsSlotEmpty(serviceQuery)) {
        ctx.AddErrorBlock(TError(TError::EType::TRANSLATEERROR, ERROR_FAILED_TO_PROCESS));
        return false;
    }
    TString service(serviceQuery->Value.GetString());
    Y_ENSURE(service.empty() || service == "yandex" || service == "google");
    TContext::TSlot* textToTranslateQuery = ctx.GetSlot("text_to_translate", "string");
    if (!IsSlotEmpty(textToTranslateQuery)) {
        segmentation.TextToTranslate = UTF8ToWide(textToTranslateQuery->Value.GetString());
    }
    // Parse languages.
    TString langSrc(langSrcQuery->Value.GetString()), langDst(langDstQuery->Value.GetString());
    TString unsupportedLangSrc, unsupportedLangDst;
    if (langSrc.find(UNSUPPORTED) != TString::npos) {
        langSrc = langSrc.substr(UNSUPPORTED.size());
    }
    if (langDst.find(UNSUPPORTED) != TString::npos) {
        langDst = langDst.substr(UNSUPPORTED.size());
    }
    // For detecting languages we should remove hyphens (if any).
    ReplaceAll(langDst, '-', ' ');
    // Currently lang name comes with preposition "c/на/по", so we should omit them
    ParseLangName(UTF8ToWide(Split(langSrc, " ").back()), segmentation.From, unsupportedLangSrc);
    ParseLangName(UTF8ToWide(Split(langDst, " ").back()), segmentation.To, unsupportedLangDst);
    // We should add error blocks in case of any errors except empty text to translate.
    if (!unsupportedLangSrc.empty() || !unsupportedLangDst.empty()) {
        FillSlots(unsupportedLangSrc, unsupportedLangDst, service, segmentation, ctx);
        ctx.AddErrorBlock(TError(TError::EType::TRANSLATEERROR, ERROR_NOT_SUPPORTED_LANGUAGE));
        return false;
    }
    FillSlots(service, segmentation, ctx);
    TString error;
    if (segmentation.To == segmentation.From && segmentation.To != LANG_UNK) {
        ctx.AddErrorBlock(TError(TError::EType::TRANSLATEERROR, ERROR_WRONG_LANGUAGE));
        return false;
    }
    if (segmentation.From == LANG_UNK || segmentation.To == LANG_UNK) {
        error = ERROR_FAILED_TO_PROCESS;
    }
    if (!segmentation.TextToTranslate) {
        if (!error) {
            service = ForceTranslateService(service);
        }
        error = ERROR_FAILED_TO_PROCESS;
    }
    AddOpenTranslateCommand(service, error, segmentation, ctx);
    return (!service || !ctx.ClientFeatures().SupportsDivCards()) && !error;
}

TUtf16String TTranslateFormHandler::GetSlotValueIfExists(TContext& ctx, TString slotName) const {
    TContext::TSlot* slot = ctx.GetSlot(slotName, "string");

    TUtf16String value = u"";
    if (!IsSlotEmpty(slot)) {
        value = UTF8ToWide(slot->Value.GetString());
    }

    return value;
}

TString TTranslateFormHandler::ParseQuerySegmentation(TUtf16String text, TUtf16String srcLang, TUtf16String dstLang,
                        TSegmentation& segmentation, TString& unsupportedLangSrc, TString& unsupportedLangDst) const {

    ToLower(text);
    ToLower(srcLang);
    ToLower(dstLang);

    ReplaceAll(dstLang, '-', ' ');
    dstLang = Split(dstLang, " ").back();

    segmentation.TextToTranslate = FixNumbers(text, LANG_RUS);
    if (!srcLang) {
        segmentation.From = LANG_UNK;
    } else {
        ParseLangName(srcLang, segmentation.From, unsupportedLangSrc);
    }

    if (!dstLang) {
        segmentation.To = LANG_UNK;
    } else {
        ParseLangName(dstLang, segmentation.To, unsupportedLangDst);
    }

    if (unsupportedLangSrc || unsupportedLangDst) {
        return TString(ERROR_NOT_SUPPORTED_LANGUAGE);
    }

    return TString();
}

TUtf16String TTranslateFormHandler::FixNumbers(const TUtf16String& text, ELanguage lang) const {
    const auto* transducer = NumeralTransducers.FindPtr(lang);
    if (!transducer)
        return text;

    TVector<TUtf16String> result;
    for (const auto& word : Split(text)) {
        if (IsNumber(word)) {
            const TVector<TUtf16String> textNumbers = transducer->Get()->Run(word);
            if (textNumbers) {
                TVector<TUtf16String> words = Split(textNumbers[0]);
                result.insert(result.end(), words.begin(), words.end());
                continue;
            }
        }
        result.emplace_back(word);
    }
    return Join(" ", result);
}

bool TTranslateFormHandler::DetectLangAndTranslit(const TUtf16String& text,
                                                  TSegmentation& segmentation,
                                                  TContext& ctx) const {
    bool needTranslit = (segmentation.From != LANG_RUS);
    bool srcSpecified = (segmentation.From != LANG_UNK);
    bool dstSpecified = (segmentation.To != LANG_UNK);
    bool langSpecified = (srcSpecified || dstSpecified);
    bool langDetected = DetectTranslateDirection(segmentation);

    if (!langDetected || needTranslit || HasNonRussianLetters(text)) {
        // when language (src or/and dst) is explicitly specified, we should force this language during transliteration
        bool forceTranslit = (langSpecified && langDetected);
        TTranslateFormHandler::TTranslitResult translitResult = TranslitQuery(ctx, text, segmentation.From, forceTranslit);

        // for queries with mixed script only non-cyrillic part is kept to be translated
        TUtf16String translitText = DropOddScript(translitResult.Translit);

        if (translitText && translitText != text) {
            segmentation.TextToTranslate = translitText;
            // if source language was not given explicitly, we infer it after transliteration
            if (!srcSpecified) {
                segmentation.From = translitResult.Lang;

                if (!dstSpecified) {
                    segmentation.To = (segmentation.From == LANG_RUS) ? LANG_ENG : LANG_RUS;
                }

                langDetected = (segmentation.From != segmentation.To);
            }
        }
    }
    return langDetected;
}

void TTranslateFormHandler::ParseLangName(const TWtringBuf segment, ELanguage& lang, TString& unsupportedLang) const {
    for (size_t len = 1; len <= segment.size(); ++len) {
        const auto it = ShortLanguageStems.find(segment.substr(0, len));
        if (it != ShortLanguageStems.end()) {
            lang = it->second;
            return;
        }
        const auto stopIt = StopLanguageStems.find(segment.substr(0, len));
        if (stopIt != StopLanguageStems.end()) {
            unsupportedLang = WideToUTF8(stopIt->second);
            return;
        }
    }
}

bool TTranslateFormHandler::DetectTranslateDirection(TSegmentation& segmentation, bool redetect) const {
    if (redetect || segmentation.From == LANG_UNK) {
        if (!segmentation.TextToTranslate) {
            segmentation.From = (segmentation.To == LANG_RUS) ? LANG_ENG : LANG_RUS;
        } else {
            segmentation.From = LangDetector->Detect(segmentation.TextToTranslate,
                                                     LangDetector->GetSupportedLangs());
            if (segmentation.From == LANG_UNK) {
                segmentation.From = LANG_RUS;
            }
        }
    }
    if (redetect || segmentation.To == LANG_UNK) {
        segmentation.To = (segmentation.From == LANG_RUS) ? LANG_ENG : LANG_RUS;
    }
    return segmentation.From != segmentation.To;
}

bool TTranslateFormHandler::HasNonRussianLetters(const TWtringBuf text) const {
    for (const wchar16 c : text) {
        if (IsAlpha(c) && !BelongsTo(c, LANG_RUS))
            return true;
    }
    return false;
}

TTranslateFormHandler::TTranslitResult TTranslateFormHandler::TranslitQuery(const TContext& ctx,
                                                  const TUtf16String& query,
                                                  ELanguage lang,
                                                  bool forceTranslit) const {
    TCgiParameters cgi;
    cgi.InsertEscaped("query", WideToUTF8(query));
    cgi.InsertEscaped("lang", IsoNameByLanguage(lang));
    forceTranslit ? cgi.InsertEscaped("force", "true") : cgi.InsertEscaped("force", "false");
    cgi.InsertEscaped("version", "new");
    NHttpFetcher::TRequestPtr req =
        ctx.GetSources().TranslateMtAlice("/translit/translit").Request();
    req->AddCgiParams(cgi);
    NHttpFetcher::TResponse::TRef resp = req->Fetch()->Wait();
    NSc::TValue respJson;
    if (resp->IsError() || !NSc::TValue::FromJson(respJson, resp->Data) ||
        !respJson.Has("result") || !respJson.Has("lang"))
        return {query, lang};

    return {UTF8ToWide(respJson["result"].GetString()), LanguageByName(respJson["lang"].GetString())};
}

TString TTranslateFormHandler::FindTranslateServiceInQuery(TStringBuf query) const {
    bool hasYandex = false;
    bool hasGoogle = false;
    bool hasTranslate = false;
    for (const auto& it : StringSplitter(query).Split(' ').SkipEmpty()) {
        const TUtf16String word = UTF8ToWide(it.Token());
        TVector<TYandexLemma> lemmas;
        NLemmer::AnalyzeWord(word.data(), word.size(), lemmas, TLangMask(LANG_RUS));
        const TString lemmaText = WideToUTF8(lemmas.front().GetText());
        if (lemmaText == "яндекс" || lemmaText == "yandex") {
            hasYandex = true;
        } else if (lemmaText == "гугл" || lemmaText == "google") {
            hasGoogle = true;
        } else if (lemmaText == "переводчик" || lemmaText == "перевод" || lemmaText == "translate") {
            hasTranslate = true;
        } else {
            hasTranslate = hasYandex = hasGoogle = false;
        }
        if (hasTranslate) {
            if (hasYandex)
                return "yandex";
            if (hasGoogle)
                return "google";
        }
    }
    return "";
}

void TTranslateFormHandler::FillSlots(TStringBuf service,
                                      const TSegmentation& segmentation,
                                      TContext& ctx) const {
    ctx.CreateSlot("translate_service", /* type */ "string", /* optional */ false,
                   NSc::TValue(service));
    TString text = WideToUTF8(segmentation.TextToTranslate);
    ctx.CreateSlot("text_to_translate", /* type */ "string", /* optional */ false,
                   NSc::TValue(text));
    if (text && segmentation.From != LANG_UNK) {
        TString voice, translit;
        ELanguage langFrom = segmentation.From;
        if (segmentation.From != LANG_RUS) {
            // we should check whether actual lang is really foreign in order to get proper transcription
            langFrom = LangDetector->Detect(segmentation.TextToTranslate, LangDetector->GetSupportedLangs());
            langFrom = (langFrom == LANG_RUS) ? LANG_RUS : segmentation.From;
        }
        GetVoiceFromText(ctx, text, langFrom, DEFAULT_SPEED, voice, translit);
        ctx.CreateSlot("text_to_translate_voice", /* type */ "string", /* optional */ false,
                       NSc::TValue(voice));
    }
    FillLangSlot("lang_src", segmentation.From, ctx);
    FillLangSlot("lang_dst", segmentation.To, ctx);
}

void TTranslateFormHandler::FillSlots(TStringBuf unsupportedLangSrc,
                                      TStringBuf unsupportedLangDst,
                                      TStringBuf service,
                                      TSegmentation& segmentation,
                                      TContext& ctx) const {
    if (unsupportedLangSrc) {
        ctx.CreateSlot("lang_src", /* type */ "string", /* optional */ false,
                       NSc::TValue(TString(UNSUPPORTED) + unsupportedLangSrc));
        segmentation.From = LANG_UNK;
    }
    if (unsupportedLangDst) {
        ctx.CreateSlot("lang_dst", /* type */ "string", /* optional */ false,
                       NSc::TValue(TString(UNSUPPORTED) + unsupportedLangDst));
        segmentation.To = LANG_UNK;
    }
    FillSlots(service, segmentation, ctx);
}

void TTranslateFormHandler::FillLangSlot(TStringBuf name,
                                         ELanguage lang,
                                         TContext& ctx) const {
    const auto iterator = LanguageNames.find(lang);
    if (iterator != LanguageNames.end())
        ctx.CreateSlot(name, /* type */ "string", /* optional */ false, NSc::TValue(iterator->second));
}

void TTranslateFormHandler::AddOpenTranslateCommand(TStringBuf service,
                                                    TStringBuf error,
                                                    TSegmentation& segmentation,
                                                    TContext& ctx) const {
    if (service && ctx.ClientFeatures().SupportsDivCards()) {
        // Add Command to open Yandex/Google Translate with given parameters.
        NSc::TValue data;
        if (segmentation.From == LANG_UNK || segmentation.To == LANG_UNK) {
            DetectTranslateDirection(segmentation);
        }
        TLangDir dir(segmentation.From, segmentation.To);
        NSc::TValue card;
        TString result;
        if (service == "yandex") {
            result = YANDEX_TRANSLATE_TEXT;
            data["uri"] = GenerateTranslateUri(ctx, WideToUTF8(segmentation.TextToTranslate), dir.ToString());
            ctx.AddCommand<TTranslateAsYandexDirective>(TStringBuf("open_uri"), data);
        } else if (service == "google") {
            result = GOOGLE_TRANSLATE_TEXT;
            TStringBuilder urlBuilder;
            urlBuilder << "https://translate.google.ru/#" << IsoNameByLanguage(dir.From) << "/" <<
                IsoNameByLanguage(dir.To) << "/";
            TString textEncoded = WideToUTF8(segmentation.TextToTranslate);
            Quote(textEncoded, "");
            urlBuilder << textEncoded;
            data["uri"] = urlBuilder;
            ctx.AddCommand<TTranslateAsGoogleDirective>(TStringBuf("open_uri"), data);
        } else {
            ctx.AddCommand<TTranslateAsUnknownDirective>(TStringBuf("open_uri"), data);
        }
        //TODO: condition 'ctx.ClientFeatures().SupportsDivCards()' is possibly unnecessary
        if (ctx.ClientFeatures().SupportsDivCards()) {
            card["translator_url"] = data["uri"];
            ctx.AddDivCardBlock("translator_card", card);
            ctx.AddAttention(ATTENTION_RESULT_CARD);
        }
        ctx.CreateSlot("result", /* type */ "string", /* optional */ false, NSc::TValue(result));
        ctx.AddAttention(ATTENTION_OPEN_TRANSLATE);
    } else if (error) {
        // otherwise in case of error -> add error
        ctx.AddErrorBlock(TError(TError::EType::TRANSLATEERROR, error));
    }
}

void TTranslateFormHandler::AddOpenSearchCommand(TStringBuf query, TContext& ctx) const {
    NSc::TValue data;
    TString searchUri = GenerateSearchUri(&ctx, query);
    data["uri"] = searchUri;
    ctx.AddCommand<TTranslateSearchDirective>(TStringBuf("open_uri"), data);
    ctx.AddTextCardBlock("unsupported_lang");
    NSc::TValue card;
    card["search_url"] = searchUri;
    ctx.AddDivCardBlock("open_search_card", card);
    ctx.AddAttention(ATTENTION_OPEN_SEARCH);
}

TString TTranslateFormHandler::ForceTranslateService(const TString& service) const {
    if (!service) {
        // We should open Translate in this case https://st.yandex-team.ru/DIALOG-3549
        return "yandex";
    }
    return service;
}

TUtf16String TTranslateFormHandler::DropOddScript(const TUtf16String& text) const {
    if (!HasNonRussianLetters(text)) {
        return text;
    }

    TVector<TUtf16String> words, spaces;
    NMisspell::SelectWords(text, words, spaces);

    TUtf16String selectedText;
    for (auto i : xrange(words.size())) {
        if (!ClassifyScript(words[i]).Intersects(TScriptMask::CYRILLIC)) {
            selectedText += words[i];
            selectedText += spaces[i + 1];
        }
    }

    return selectedText;
}

TResultValue TTranslateFormHandler::AddSearchOpenSuggests(TContext& ctx) const {
    if (ctx.ClientFeatures().SupportsDivCards()) {
        ctx.AddSuggest("translate__open_uri", "");
        ctx.AddSearchSuggest();
    }
    return TResultValue();
}

bool TTranslateFormHandler::TranslateQuery(TSegmentation& segmentation,
                                           TString& result,
                                           TString& cardName,
                                           NSc::TValue& card,
                                           NSc::TValue& factCard,
                                           TContext& ctx) const {
    TString textToTranslate = WideToUTF8(segmentation.TextToTranslate);
    TLangDir dir(segmentation.From, segmentation.To);
    if (IsAcceptableForDictionaries(textToTranslate) &&
        DictionariesLookup(ctx, textToTranslate, dir, result, card, factCard)) {
        ctx.CreateSlot("result", /* type */ "string", /* optional */ false, NSc::TValue(result));
        cardName = "dict_card";
    } else if (Translate(ctx, textToTranslate, dir, result)) {
        // try to reverse direction for short queries and find them in dictionary
        TLangDir reverseDir(segmentation.To, segmentation.From);
        TString src, dst;
        ELanguage translitLang;
        if (segmentation.From == LANG_RUS) {
            translitLang = segmentation.To;
            src = textToTranslate;
            dst = result;
        } else {
            translitLang = segmentation.From;
            src = result;
            dst = textToTranslate;
        }
        TString reverseResult;
        if (!ctx.HasExpFlag(NAlice::NExperiments::EXP_DISABLE_REVERSE_TRANSLATION) &&
            IsAcceptableForDictionaries(result) && IsTranslit(ctx, src, dst, translitLang) &&
            DictionariesLookup(ctx, result, reverseDir, reverseResult, card, factCard)) {
            ReverseTranslateResult(reverseResult, result, segmentation, ctx);
            cardName = "dict_card";
        } else {
            cardName = "translate_card";
            if (!ctx.HasExpFlag(NAlice::NExperiments::EXP_DISABLE_REVERSE_TRANSLATION) &&
                src == dst && Translate(ctx, result, reverseDir, reverseResult) && reverseResult != result) {
                card["translate_url"] = GenerateTranslateUri(ctx, result, reverseDir.ToString());
                ReverseTranslateResult(reverseResult, result, segmentation, ctx);
            } else {
                card["translate_url"] = GenerateTranslateUri(ctx, textToTranslate, dir.ToString());
            }
        }
        ctx.CreateSlot("result", /* type */ "string", /* optional */ false, NSc::TValue(result));
    } else {
        ctx.AddErrorBlock(TError(TError::EType::TRANSLATEERROR, ERROR_FAILED_TO_TRANSLATE));
        return false;
    }
    AddSuggestLanguages(segmentation, ctx);
    return true;
}

bool TTranslateFormHandler::IsAcceptableForDictionaries(TStringBuf text) const {
    return ((text.size() <= DICT_MAX_CHARS) &&
            StringSplitter(text).Split(' ').Count() <= DICT_MAX_WORDS);
}

bool TTranslateFormHandler::DictionariesLookup(TContext& ctx,
                                               TStringBuf text,
                                               const TLangDir& dir,
                                               TString& result,
                                               NSc::TValue& card,
                                               NSc::TValue& factCard) const {
    TCgiParameters cgi;
    cgi.InsertEscaped("lang", dir.ToString());
    // DEFINITIONS + MORPHO + POS_FILTER + SHORT_ATTRS
    cgi.InsertEscaped("flags", "142");
    cgi.InsertEscaped("srv", "vins");
    cgi.InsertEscaped("text", text);
    cgi.InsertEscaped("ui", "ru");
    NHttpFetcher::TRequestPtr req = ctx.GetSources().TranslateDict().Request();
    req->AddCgiParams(cgi);
    NHttpFetcher::TResponse::TRef resp = req->Fetch()->Wait();
    NSc::TValue respJson;
    if (resp->IsError() || !NSc::TValue::FromJson(respJson, resp->Data))
        return false;

    TString translateUrl = GenerateTranslateUri(ctx, text, dir.ToString());
    ParseDictResponse(respJson, translateUrl, result, card, factCard);
    result = WideToUTF8(FixNumbers(UTF8ToWide(result), dir.To));
    return !result.empty();
}

void TTranslateFormHandler::ParseDictResponse(const NSc::TValue& response,
                                              TStringBuf translateUrl,
                                              TString& result,
                                              NSc::TValue& card,
                                              NSc::TValue& factCard) const {
    if (!response.Has("def"))
        return;

    bool transcriptionFound = false;
    for (size_t i = 0; i < Min(response["def"].ArraySize(), POS_MAX_NUM); ++i)  {
        const auto def = response["def"][i];
        NSc::TValue definition;
        // add translate uri to the first item
        if (i == 0) {
            definition["translate_url"] = translateUrl;
        }
        // get text, pos and transcription
        definition["text"] = def["text"].GetString();
        if (def.Has("pos")) {
            definition["pos"] = def["pos"].GetString();
        }
        if (def.Has("ts") && !transcriptionFound) {
            definition["ts"] = "[" + TString{def["ts"].GetString()} + "]";
            transcriptionFound = true;
        }
        for (size_t j = 0; j < Min(def["tr"].ArraySize(), TR_MAX_NUM); ++j) {
            // get synonyms
            const auto tr = def["tr"][j];
            const TString word(tr["text"].GetString());
            TVector<TString> translations = { word };
            size_t wordsNum = StringSplitter(word).Split(' ').Count();
            if (result.empty()) {
                result = word;
            }
            if (tr.Has("syn")) {
                for (const auto& syn : tr["syn"].GetArray()) {
                    const TString text(syn["text"].GetString());
                    size_t curWordsNum = StringSplitter(text).Split(' ').Count();
                    if (wordsNum + curWordsNum > MEAN_MAX_NUM)
                        break;

                    wordsNum += curWordsNum;
                    translations.push_back(text);
                }
            }
            // get meanings
            if (tr.Has("mean")) {
                TVector<TString> meanings;
                size_t wordsNum = 0;
                for (const auto& mean : tr["mean"].GetArray()) {
                    const TString word(mean["text"].GetString());
                    size_t curWordsNum = StringSplitter(word).Split(' ').Count();
                    if (wordsNum + curWordsNum > MEAN_MAX_NUM)
                        break;

                    wordsNum += curWordsNum;
                    meanings.push_back(word);
                }
                definition["mean_" + ToString(j)] = Join(", ", meanings);
            }
            // get defintion (if exists)
            if (tr.Has("def")) {
                TString defStr(tr["def"].GetString());
                size_t delimPos = defStr.find('>');
                if (delimPos == TString::npos) {
                    factCard["def"] = defStr;
                } else {
                    TString term = defStr.substr(0, delimPos);
                    RemoveAnyOf(term, "<>");
                    factCard["def"] = term + defStr.substr(delimPos + 1);
                }
                factCard["ref_name"] = tr["ref"]["name"].GetString();
                TString url(tr["ref"]["url"].GetString());
                UrlEscape(url);
                factCard["ref_url"] = url;
            } else {
                definition["tr_" + ToString(j)] = Join(", ", translations);
            }
            // get image (if exists)
            if (tr.Has("img")) {
                factCard["img"] = tr["img"].GetString();
            }
        }
        card.Push(definition);
    }
}

bool TTranslateFormHandler::Translate(const TContext& ctx,
                                      TStringBuf text,
                                      const TLangDir& dir,
                                      TString& result) const {
    TCgiParameters cgi;
    cgi.InsertEscaped("lang", dir.ToString());
    cgi.InsertEscaped("srv", "vins");
    cgi.InsertEscaped("text", text);
    NHttpFetcher::TRequestPtr req = ctx.GetSources().Translate().Request();
    req->AddCgiParams(cgi);
    NHttpFetcher::TResponse::TRef resp = req->Fetch()->Wait();
    NSc::TValue respJson;
    if (resp->IsError() || !NSc::TValue::FromJson(respJson, resp->Data) ||
        !respJson.Has("text") || !respJson["text"].IsArray())
        return false;

    result = WideToUTF8(FixNumbers(UTF8ToWide(respJson["text"][0].GetString()), dir.To));
    return !result.empty();
}

bool TTranslateFormHandler::IsTranslit(const TContext& ctx,
                                       TStringBuf src,
                                       TStringBuf dst,
                                       ELanguage lang) const {
    TCgiParameters cgi;
    cgi.InsertEscaped("lang", IsoNameByLanguage(lang));
    cgi.InsertEscaped("src", src);
    cgi.InsertEscaped("dst", dst);
    NHttpFetcher::TRequestPtr req = ctx.GetSources().TranslateIsTranslit().Request();
    req->AddCgiParams(cgi);
    NHttpFetcher::TResponse::TRef resp = req->Fetch()->Wait();
    NSc::TValue respJson;
    if (resp->IsError() || !NSc::TValue::FromJson(respJson, resp->Data) || !respJson.Has("result"))
        return false;

    return respJson["result"].GetBool();
}

void TTranslateFormHandler::ReverseTranslateResult(TStringBuf reverseResult,
                                                   TString& result,
                                                   TSegmentation& segmentation,
                                                   TContext& ctx) const {
    std::swap(segmentation.From, segmentation.To);
    segmentation.TextToTranslate = UTF8ToWide(result);
    FillSlots("", segmentation, ctx);
    result = reverseResult;
}

void TTranslateFormHandler::AddSuggestLanguages(const TSegmentation& segmentation,
                                                TContext& ctx) const {
    ctx.AddSuggest("translate__repeat", "");
    THash<TString> hash;
    TMersenne<size_t> random(hash(ctx.Meta().RequestId()));
    TVector<int> indices(Languages.size());
    Iota(indices.begin(), indices.end(), 0);
    Shuffle(indices.begin(), indices.end(), random);
    TVector<TString> suggestLangs;
    for (size_t i = 0, numSuggests = 0; i < indices.size() && numSuggests < SUGGESTS_NUM; ++i) {
        size_t index = indices[i];
        if (Languages[index] != segmentation.From && Languages[index] != segmentation.To &&
            GetWrongSuggestLangs().find(Languages[index]) == GetWrongSuggestLangs().end()) {
            ctx.AddSuggest("translate__change_language", NSc::TValue(LanguageNames[Languages[index]]));
            suggestLangs.push_back(LanguageNames[Languages[index]]);
            ++numSuggests;
        }
    }
    // for purposes of voice repeat scenario.
    ctx.CreateSlot("suggest_langs", /* type */ "string", /* optional */ false, NSc::TValue(Join(" ", suggestLangs)));
}

bool TTranslateFormHandler::IsSwear(const TUtf16String& text) const {
    for (const auto& it : StringSplitter(text).Split(static_cast<wchar16>(' ')).SkipEmpty()) {
        if (SwearWords.find(it.Token()) != SwearWords.end()) {
            return true;
        }
    }
    return false;
}

void TTranslateFormHandler::FillPronunciation(const TSegmentation& segmentation,
                                              const TString& result,
                                              TStringBuf cardName,
                                              NSc::TValue& card,
                                              TContext& ctx) const {
    // get correct speed of voice in [LOWER_BOUND, UPPER_BOUND].
    double speed = DEFAULT_SPEED;
    TContext::TSlot* speedSlot = ctx.GetSlot("speed", "num");
    if (!IsSlotEmpty(speedSlot)) {
        speed = speedSlot->Value.GetNumber();
    }
    if (ctx.FormName() == TRANSLATE_SLOWER_SCENARIO) {
        speed = std::max(speed - SPEED_STEP, LOWER_SPEED_BOUND);
    } else if (ctx.FormName() == TRANSLATE_QUICKER_SCENARIO) {
        speed = std::min(speed + SPEED_STEP, UPPER_SPEED_BOUND);
    }
    ctx.CreateSlot("speed", /* type */ "num", /* optional */ false, NSc::TValue(speed));

    TString translit, voice;
    // create voice representation.
    GetVoiceFromText(ctx, result, segmentation.To, speed, voice, translit);
    if (translit) {
        if (cardName == "translate_card") {
            card["translit"] = translit;
        } else if (card.IsArray() && !card.ArrayEmpty()) {
            card[0]["ts"] = "[" + translit + "]";
        }
    }
    if (voice) {
        ctx.CreateSlot("voice", /* type */ "string", /* optional */ false, NSc::TValue(voice));
        if (segmentation.To != LANG_RUS) {
            // add suggests for speeding up / slowing down speech.
            TVector<TString> suggestSpeed;
            if (speed < UPPER_SPEED_BOUND) {
                ctx.AddSuggest("translate__quicker", "");
                suggestSpeed.push_back("quicker");
            }
            if (speed > LOWER_SPEED_BOUND) {
                ctx.AddSuggest("translate__slower", "");
                suggestSpeed.push_back("slower");
            }
            // for purposes of voice repeat scenario.
            if (!suggestSpeed.empty()) {
                ctx.CreateSlot("suggest_speed", /* type */ "string", /* optional */ false,
                               NSc::TValue(Join(" ", suggestSpeed)));
            }
        }
        // add pronounce icon.
        const auto* pronounceIcon = ctx.Avatar("serp_gallery", "pronounce");
        if (pronounceIcon) {
            if (cardName == "translate_card") {
                card["pronounce_icon"] = NSc::TValue(pronounceIcon->Https);
            } else if (card.IsArray() && !card.ArrayEmpty()) {
                card[0]["pronounce_icon"] = NSc::TValue(pronounceIcon->Https);
            }
        }
    } else {
        ctx.AddAttention(ATTENTION_NO_VOICE);
    }
}

void TTranslateFormHandler::GetVoiceFromText(const TContext& ctx,
                                             const TString& text,
                                             ELanguage lang,
                                             double speed,
                                             TString& voice,
                                             TString& translit) const {
    if (lang == LANG_RUS) {
        voice = text;
    } else if (GetSynthesLangs().find(lang) != GetSynthesLangs().end()) {
        voice = CreateVoiceSlotRepresentation(text, speed, IsoNameByLanguage(lang));
    } else if (GetPhonemeLangs().find(lang) != GetPhonemeLangs().end()) {
        voice = CreateVoiceSlotRepresentation(FormPronunciation(ctx, lang, text), speed);
    } else if (GetTranslitLangs().find(lang) == GetTranslitLangs().end()) {
        voice = CreateVoiceSlotRepresentation(
            WideToUTF8(DefaultBeamSearchTransliterator->Decode(
                NormalizeUnicode(UTF8ToWide(text)))), speed);
    } else if (TryToTranslitResult(ctx, text, lang, translit)) {
        voice = CreateVoiceSlotRepresentation(
            WideToUTF8(NonStandardBeamSearchTransliterator->Decode(
                NormalizeUnicode(UTF8ToWide(translit)))), speed);
    }
}

TString TTranslateFormHandler::FormPronunciation(const TContext& ctx,
                                                 ELanguage lang,
                                                 TStringBuf text) const {
    TString result;
    TCgiParameters cgi;
    cgi.InsertEscaped("lang", IsoNameByLanguage(lang));
    cgi.InsertEscaped("text", text);
    NHttpFetcher::TRequestPtr req =
        ctx.GetSources().TranslateMtAlice("/transcribe/transcribe").Request();
    req->AddCgiParams(cgi);
    NHttpFetcher::TResponse::TRef resp = req->Fetch()->Wait();
    NSc::TValue respJson;
    if (!resp->IsError() && NSc::TValue::FromJson(respJson, resp->Data) && respJson.Has("result")) {
        result = respJson["result"].GetString();
        if (result != text)
            return result;
    }
    return WideToUTF8(DefaultBeamSearchTransliterator->Decode(
        RemoveDiacritics(lang, UTF8ToWide(text))));
}

bool TTranslateFormHandler::TryToTranslitResult(const TContext& ctx,
                                                TStringBuf text,
                                                ELanguage dstLang,
                                                TString& translit) const {
    TCgiParameters cgi;
    cgi.InsertEscaped("lang", IsoNameByLanguage(dstLang));
    cgi.InsertEscaped("text", text);
    NHttpFetcher::TRequestPtr req = ctx.GetSources().TranslateTranslit().Request();
    req->AddCgiParams(cgi);
    NHttpFetcher::TResponse::TRef resp = req->Fetch()->Wait();
    NSc::TValue result;
    if (resp->IsError() || !NSc::TValue::FromJson(result, resp->Data) || !result.IsString())
        return false;

    translit = StripString(result.GetString());
    return !translit.empty();
}

TString TTranslateFormHandler::CreateVoiceSlotRepresentation(const TString& voice,
                                                             double speed,
                                                             TStringBuf langName) const {
    TStringStream voiceStr;
    if (langName) {
        voiceStr << "<speaker voice=\"oksana\" effect=\"translate_oksana_" << langName <<
            "\" lang=\"" << langName << "\" speed=\"" << speed << "\">" << voice;
    } else {
        voiceStr << "<speaker voice=\"alyss\" effect=\"translate_alyss_omni\" speed=\"" <<
            speed << "\">" << voice;
    }
    return voiceStr.Str();
}

}  // namespace NBASS
