#pragma once

#include <util/generic/string.h>

void ConvertModelToMemmapped(
    const TString& pbModelFileName,
    size_t minConversionSizeBytes
);
