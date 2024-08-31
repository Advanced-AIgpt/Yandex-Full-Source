#pragma once

#include <alice/hollywood/library/scenarios/image_what_is_this/answers/answer.h>
#include <library/cpp/scheme/scheme.h>

namespace NAlice::NHollywood::NImage::NAnswers {

class TMarket : public IAnswer {
public:
    TMarket();

    static TMarket* GetPtr();

    NSc::TValue MakeMarketGallery(TImageWhatIsThisApplyContext& ctx, const NSc::TValue& visionData,
                                  bool ignoreFirst, bool appendTailLink, int cropId = -1) const;

    void MakeRequests(TImageWhatIsThisApplyContext& ctx) const override;

protected:
    bool IsSuitableAnswer(TImageWhatIsThisApplyContext& ctx, bool force) const override;
    void ComposeAnswer(TImageWhatIsThisApplyContext& ctx) const override;
    bool RenderErrorAnswer(TImageWhatIsThisApplyContext& ctx) const override;

private:
    void AddMarketCard(TImageWhatIsThisApplyContext& ctx) const;
    void AddMarketGallery(TImageWhatIsThisApplyContext& ctx, bool ignoreFirst) const;

    bool CheckMarketItem(TImageWhatIsThisApplyContext& ctx, const NSc::TValue& marketItem) const;
    void FillMarketItem(TImageWhatIsThisApplyContext& ctx, const NSc::TValue& item, NSc::TValue& marketItem,
                        bool isOrigImage, bool imageThumb, const TString& cardType) const;
    void FillMarketStars(NSc::TValue& divCard) const;
};

}
