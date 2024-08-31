#pragma once

#include <alice/megamind/library/models/cards/card_model.h>

#include <google/protobuf/struct.pb.h>
#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

class TDivCardModel final : public virtual TCardModel {
public:
    explicit TDivCardModel(google::protobuf::Struct body, TMaybe<TString> text = Nothing());

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const google::protobuf::Struct& GetBody() const;

    // FIXME(sparkle, zhigan, g-kostin): remove method below after HOLLYWOOD-586 supported in search app
    [[nodiscard]] const TMaybe<TString>& GetText() const;

private:
    const google::protobuf::Struct Body;
    TMaybe<TString> Text; // FIXME(sparkle, zhigan, g-kostin): remove it after HOLLYWOOD-586 supported in search app
};

} // namespace NAlice::NMegamind
