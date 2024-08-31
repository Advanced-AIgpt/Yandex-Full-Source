#pragma once

#include <alice/json_schema_builder/runtime/runtime.h>

namespace NAlice::NJsonSchemaBuilder::NFantasy {

enum class EDivImageScale {
    Fill  /* "fill" */,
    NoScale  /* "no_scale" */,
    Fit  /* "fit" */,
};

enum class EDivAlignmentVertical {
    Top  /* "top" */,
    Center  /* "center" */,
    Bottom  /* "bottom" */,
};

enum class EDivAlignmentHorizontal {
    Left  /* "left" */,
    Center  /* "center" */,
    Right  /* "right" */,
};

}  // namespace NAlice::NFantasy
