#pragma once

#include "fixlist.h"
#include <alice/bass/forms/vins.h>


namespace NBASS {

/**
 * Navigation form.
 */
class TNavigationFormHandler : public IHandler {
public:
    static void Init();

    TResultValue Do(TRequestHandler& r) override;
    static void Register(THandlersMap* handlers);

    // It create a new navigation form with a given text (or if text is empty get it from meta.utterance).
    static TContext::TPtr SetAsResponse(TContext& ctx, bool callbackSlot, TStringBuf text = TStringBuf());

private:
    TContext* Context;
    TContext::TSlot* Answer;
    TString Target;
    TString Query;

    bool IsYaAuto;
    bool IsAndroid;
    bool IsIos;
    bool IsTouch;
    bool IsWindows;
    bool IsPornoSerp;

    enum ETargetType {
        ETT_NONE,
        ETT_APPLICATION,
        ETT_SITE
    };
    ETargetType TargetType;

    struct TFixListResult;
    TFixListResult AddFixList(TContext& ctx);
    bool AddBno(NSc::TValue& doc);

    bool AddFirstUrl(NSc::TValue& searchResult);
    bool AddFirstUrlGeneric(NSc::TValue& doc, size_t index, NSc::TArray& docs);
    bool AddFirstUrlAppWiz(NSc::TValue& doc);
    bool AddFirstUrlVideoWiz(NSc::TValue& doc);
    bool AddFirstUrlImageWiz(NSc::TValue& doc, TStringBuf reqid);
    bool AddFirstUrlMapsWiz(NSc::TValue& doc);
    bool AddFirstUrlCompaniesWiz(NSc::TValue& doc);
    bool AddFirstUrlTvOnline(NSc::TValue& doc);
    bool AddFirstUrlTranslate(NSc::TValue& doc);
    bool AddFirstUrlObject(NSc::TValue& doc);
    bool AddFirstUrlAutoRu(NSc::TValue& doc);
    bool AddFirstUrlMarket(NSc::TValue& doc);
    bool AddFirstUrlPanoramas(NSc::TValue& doc);
    bool AddFirstUrlRasp(NSc::TValue& doc);
    bool AddFirstUrlRealty(NSc::TValue& doc);
    bool AddFirstUrlYaStation(NSc::TValue& doc);

    bool AddAppFirstUrl(NSc::TValue& searchResult);

    void AddResult(TStringBuf text, TStringBuf tts, TStringBuf app, TStringBuf url,
                   bool addSearchSuggest = true, bool addUriSuggest = true);
    void Postprocess();

    TResultValue OpenNativeApp(TString name);
    TMaybe<NAlice::TNavigationFixList::TWindowsApp> AddWindowsSoft();

    TString ExtractAppId(TStringBuf url) const;
};

}
