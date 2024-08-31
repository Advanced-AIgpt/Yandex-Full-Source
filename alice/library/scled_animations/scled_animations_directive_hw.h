#pragma once

#include "scled_animations_directive.h"

#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NScledAnimation {

/// Add one of standard SCLED animation sequences from enum
inline void AddStandardScled(NHollywood::TResponseBodyBuilder& builder, EScledAnimations anim) {
    AddStandardScled(builder.GetResponseBody(), anim);
}

/// Add Typical animation with Right-To-Left and Left-To-Right scroll
inline void AddAnimatedScled(NHollywood::TResponseBodyBuilder& builder, const TString& pattern) {
    AddAnimatedScled(builder.GetResponseBody(), pattern);
}

/// Add SCLED animation from animation builder
inline void AddDrawScled(NHollywood::TResponseBodyBuilder& builder, TScledAnimationBuilder& scledBuilder, 
                         TScledAnimationOptions options = TScledAnimationOptions{}) {
    AddCustomScled(builder.GetResponseBody(), scledBuilder.PrepareBinary().c_str(), options);
}

/// Add your own SCLED animation "as is"
inline void AddCustomScled(NHollywood::TResponseBodyBuilder& builder, const TStringBuf& anim, 
                         TScledAnimationOptions options = TScledAnimationOptions{}) {
    AddCustomScled(builder.GetResponseBody(), anim, options);
}

} // namespace NAlice
