#pragma once

#include <util/generic/string.h>


namespace NCachalot::NApplication {

    int Run(int argc, const char **argv, const TString& defaultConfigResource = "");

}   // namespace NCachalot::NApplication
