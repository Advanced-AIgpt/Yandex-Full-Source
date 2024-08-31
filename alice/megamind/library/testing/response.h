#pragma once
#include <util/generic/string.h>

#if defined(UNIT_ASSERT_NOT_ERROR)
#error UNIT_ASSERT_NOT_ERROR is defined twice!
#endif

#define UNIT_ASSERT_HOLDS_TYPE_NOT_ERROR(response, type)            \
    do {                                                            \
        TString errorMsg;                                           \
        if (const auto* error = std::get_if<TError>(&response)) {         \
            errorMsg = ToString(*error);                            \
        }                                                           \
        UNIT_ASSERT_C(std::holds_alternative<type>(response), errorMsg);  \
    } while (false)
