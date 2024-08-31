#pragma once

#include <alice/bass/forms/vins.h>

#include <dict/dictutil/lang_dir.h>
#include <util/charset/wide.h>

class ITransliterator;
class TCharTrigramModel;

namespace NDict {
namespace NLangDetector {
    class ILangDetector;
}

namespace NFst {
    class TTransducer;
}
}

namespace NTestSuiteTTranslateFormHandlerTest {
    struct TTestCaseDetectLangDir;
}

namespace NBASS {

class TTranslateFormHandlerTest;

///////////////////////////////////////////////////////////////////////////////
//
// TTranslateFormHandler
//

class TTranslateFormHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& request) override;

    static void Init();
    static void Register(THandlersMap* handlers);
    static TResultValue SetAsResponse(TContext& ctx, TStringBuf query);

// structures
private:
    struct TSegmentation {
        TUtf16String TextToTranslate;
        ELanguage From{LANG_UNK};
        ELanguage To{LANG_UNK};

        void Update(const TSegmentation& other) {
            if (other.TextToTranslate)
                TextToTranslate = other.TextToTranslate;
            if (other.From != LANG_UNK)
                From = other.From;
            if (other.To != LANG_UNK)
                To = other.To;
        }
    };

    struct TTranslitResult {
        TUtf16String Translit;
        ELanguage Lang;

        TTranslitResult(const TUtf16String& translit, ELanguage lang) : Translit(translit), Lang(lang) {};
    };

// private members
private:
    static THolder<const NDict::NLangDetector::ILangDetector> LangDetector;
    static THashSet<TUtf16String> LanguageStems;
    static THashMap<TUtf16String, ELanguage> ShortLanguageStems;
    static THashMap<TUtf16String, TUtf16String> StopLanguageStems;
    static THashMap<ELanguage, THolder<NDict::NFst::TTransducer>> NumeralTransducers;
    static THolder<const TCharTrigramModel> RuTrigrams;
    static THolder<const ITransliterator> DefaultBeamSearchTransliterator;
    static THolder<const ITransliterator> NonStandardBeamSearchTransliterator;
    static THashMap<ELanguage, TString> LanguageNames;
    static TVector<ELanguage> Languages;
    static THashSet<TUtf16String> SwearWords;

// helper functions
private:
    static void LoadLanguageDetector();
    static void LoadLanguageStems();
    static void LoadLanguages();
    static void LoadNumeralTransducers();
    static void LoadTransliterators();
    static void LoadSwear();
    static void ReadToTempFile(TStringBuf resourceName,
                               const TString& filename);

// Segmentation functions
private:
    TResultValue ProcessVoiceRepeatScenario(TContext& ctx) const;
    bool SegmentQuery(TContext& ctx, TSegmentation& segmentation) const;
    bool SegmentQueryInMainScenario(TContext& ctx, TSegmentation& segmentation) const;
    bool SegmentQueryInEllipsisScenario(TContext& ctx, TSegmentation& segmentation) const;
    TString ParseQuerySegmentation(TUtf16String text, TUtf16String srcLang, TUtf16String dstLang,
                   TSegmentation& segmentation, TString& unsupportedLangSrc, TString& unsupportedLangDst) const;
    TUtf16String FixNumbers(const TUtf16String& text, ELanguage lang) const;
    bool DetectLangAndTranslit(const TUtf16String& text,
                               TSegmentation& segmentation,
                               TContext& ctx) const;
    void ParseLangName(const TWtringBuf segment, ELanguage& lang, TString& unsupportedLang) const;
    TString CorrectUnsupportedLangName(const TStringBuf langName) const;
    bool DetectTranslateDirection(TSegmentation& segmentation, bool redetect = false) const;
    bool HasNonRussianLetters(const TWtringBuf text) const;
    TTranslitResult TranslitQuery(const TContext& ctx,
                               const TUtf16String& query,
                               ELanguage lang,
                               bool forceTranslit) const;
    TString FindTranslateServiceInQuery(TStringBuf query) const;
    TUtf16String GetSlotValueIfExists(TContext& ctx, TString slotName) const;
    void FillSlots(TStringBuf service, const TSegmentation& segmentation, TContext& ctx) const;
    void FillSlots(TStringBuf unsupportedLangSrc,
                   TStringBuf unsupportedLangDst,
                   TStringBuf service,
                   TSegmentation& segmentation,
                   TContext& ctx) const;
    void FillLangSlot(TStringBuf name, ELanguage lang, TContext& ctx) const;
    // This function add command to open Yandex/Google Translate service with given
    // parameters (from segmentation) if the device supports opening uris. Otherwise
    // in case of error just adds error block.
    void AddOpenTranslateCommand(TStringBuf service,
                                 TStringBuf error,
                                 TSegmentation& segmentation,
                                 TContext& ctx) const;
    void AddOpenSearchCommand(TStringBuf query, TContext& ctx) const;
    TString ForceTranslateService(const TString& service) const;
    TUtf16String DropOddScript(const TUtf16String& text) const;
    TResultValue AddSearchOpenSuggests(TContext& ctx) const;

// Translate functions
private:
    bool TranslateQuery(TSegmentation& segmentation,
                        TString& result,
                        TString& cardName,
                        NSc::TValue& card,
                        NSc::TValue& factCard,
                        TContext& ctx) const;
    bool IsAcceptableForDictionaries(TStringBuf text) const;
    bool DictionariesLookup(TContext& ctx,
                            TStringBuf text,
                            const TLangDir& dir,
                            TString& result,
                            NSc::TValue& card,
                            NSc::TValue& factCard) const;
    void ParseDictResponse(const NSc::TValue& response,
                           TStringBuf translateUrl,
                           TString& result,
                           NSc::TValue& card,
                           NSc::TValue& factCard) const;
    bool Translate(const TContext& ctx,
                   TStringBuf text,
                   const TLangDir& dir,
                   TString& result) const;
    bool IsTranslit(const TContext& ctx, TStringBuf src, TStringBuf dst, ELanguage lang) const;
    void ReverseTranslateResult(TStringBuf reverseResult,
                                TString& result,
                                TSegmentation& segmentation,
                                TContext& ctx) const;
    void AddSuggestLanguages(const TSegmentation& segmentation, TContext& ctx) const;

// Helper functions
private:
    bool IsSwear(const TUtf16String& text) const;

// Pronunciation functions
private:
    void FillPronunciation(const TSegmentation& segmentation,
                           const TString& result,
                           TStringBuf cardName,
                           NSc::TValue& card,
                           TContext& ctx) const;
    void GetVoiceFromText(const TContext& ctx,
                          const TString& text,
                          ELanguage lang,
                          double speed,
                          TString& voice,
                          TString& translit) const;
    TString FormPronunciation(const TContext& ctx, ELanguage lang, TStringBuf text) const;
    bool TryToTranslitResult(const TContext& ctx,
                             TStringBuf text,
                             ELanguage dstLang,
                             TString& translit) const;
    TString CreateVoiceSlotRepresentation(const TString& voice,
                                          double speed,
                                          TStringBuf langName="") const;

// Unit test struct
private:
    friend struct NTestSuiteTTranslateFormHandlerTest::TTestCaseDetectLangDir;
};

}  // namespace NBASS
