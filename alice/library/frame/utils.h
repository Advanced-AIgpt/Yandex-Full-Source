#pragma once

#include <alice/megamind/protos/common/frame.pb.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>


namespace NAlice {

TString GetFrameNameListString(const TVector<TSemanticFrame>& frames);

} // namespace NAlice
