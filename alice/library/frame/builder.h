#pragma once

#include <alice/megamind/protos/common/frame.pb.h>
#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>

namespace NAlice {

class TSemanticFrameBuilder {
public:
    explicit TSemanticFrameBuilder()
    {
    }

    explicit TSemanticFrameBuilder(const TString& name) {
        Frame.SetName(name);
    }

    explicit TSemanticFrameBuilder(const TSemanticFrame& frame)
        : Frame(frame)
    {
    }

    TSemanticFrameBuilder& SetName(const TString& name);

    TSemanticFrameBuilder& AddSlot(
        const TString& name,
        TArrayRef<const TString> acceptedTypes,
        const TMaybe<TString>& type,
        const TMaybe<TString>& value,
        bool isRequested = false,
        bool isFilled = false
    );

    TSemanticFrameBuilder& AddSlot(
        const TString& name,
        const TString& type,
        const TMaybe<TString>& value
    ) {
        return AddSlot(name, {type}, type, value, /* isRequested= */ false, /* isFilled= */ false);
    }

    TSemanticFrameBuilder& SetSlotValue(const TString& name, const TString& type, const TString& value);

    TSemanticFrame Build() const;

private:
    TSemanticFrame Frame;
};


TSemanticFrame::TSlot MakeSlot(const TString& name, TArrayRef<const TString> acceptedTypes,
                               const TMaybe<TString>& type, const TMaybe<TString>& value,
                               const bool isRequested = false, const bool isFilled = false);

TClientEntity MakeEntity(const TString& name, const THashMap<TString, TArrayRef<const TString>>& items);

TSemanticFrame MakeFrame(
    const TString& name,
    TArrayRef<const TSemanticFrame::TSlot> slots = {}
);

} // namespace NAlice
