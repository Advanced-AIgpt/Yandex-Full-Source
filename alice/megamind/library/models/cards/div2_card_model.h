#pragma once

#include <alice/megamind/library/models/cards/card_model.h>

#include <google/protobuf/struct.pb.h>

namespace NAlice::NMegamind {

class TDiv2CardModel final : public virtual TCardModel {
public:
    explicit TDiv2CardModel(google::protobuf::Struct body, bool hasBorders = true, TString text = {})
        : TCardModel(ECardType::Div2Card)
        , Body(std::move(body))
        , HasBorders(hasBorders)
        , Text(std::move(text))
    {
    }

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] const google::protobuf::Struct& GetBody() const {
        return Body;
    }

    [[nodiscard]] bool GetHasBorders() const {
        return HasBorders;
    }

    [[nodiscard]] const TString& GetText() const {
        return Text;
    }

private:
    google::protobuf::Struct Body;
    bool HasBorders;
    TString Text;
};

} // namespace NAlice::NMegamind
