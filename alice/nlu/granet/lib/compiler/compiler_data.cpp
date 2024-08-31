#include "compiler_data.h"

namespace NGranet::NCompiler {

static const TString STR_PLUS = "+";
static const TString STR_QUESTION = "?";
static const TString STR_STAR = "*";

// ~~~~ TBagItemParams ~~~~

TString TBagItemParams::PrintSuffix() const {
    if (IsRequired) {
        return IsLimited ? TString() : STR_PLUS;
    } else {
        return IsLimited ? STR_QUESTION : STR_STAR;
    }
}

} // namespace NGranet::NCompiler
