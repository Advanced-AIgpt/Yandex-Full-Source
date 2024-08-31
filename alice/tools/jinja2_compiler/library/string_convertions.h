#pragma once

#include <util/string/ascii.h>
#include <util/generic/yexception.h>

namespace NAlice {

enum class EStringDetector {
    UNDEFINED,          // unknown or can not detect
    LOWER_CASE,         // lower_case
    UPPER_CAMEL_CASE,   // CamelCase
    LOWER_CAMEL_CASE,   // camelCase
    UPPER_CASE          // UPPER_CASE
};

//
// Check string styling
//
inline EStringDetector DetectString(const TString& str) {
    int lowercasecount = 0;
    int uppercasecount = 0;
    int underscorecount = 0;

    if (str.empty() || !IsAsciiAlnum(str[0])) {
        return EStringDetector::UNDEFINED;
    }

    for (const auto it : str) {
        if (it == '_') {
            underscorecount++;
        } else if (IsAsciiUpper(it)) {
            uppercasecount++;
        } else if (IsAsciiLower(it)) {
            lowercasecount++;
        } else if (!IsAsciiAlnum(it)) {
            return EStringDetector::UNDEFINED;
        }
    }

    if (uppercasecount == 0) {
        return EStringDetector::LOWER_CASE;
    }
    if (lowercasecount == 0) {
        return EStringDetector::UPPER_CASE;
    }
    if (underscorecount == 0) {
        return IsAsciiUpper(str[0]) ? EStringDetector::UPPER_CAMEL_CASE : EStringDetector::LOWER_CAMEL_CASE;
    }
    return EStringDetector::UNDEFINED;
}

//
// Convert String to different style format
//
inline TString ConvertString(const TString& str, EStringDetector targetFormat) {
    const EStringDetector sourceFormat = DetectString(str);
    if (sourceFormat == EStringDetector::UNDEFINED || 
        targetFormat == EStringDetector::UNDEFINED || 
        sourceFormat == targetFormat) {
        // Unknown format or target format is the same as source format
        return str;
    }
    // Converts between UPPER_CAMEL_CASE <-> LOWER_CAMEL_CASE
    if ((sourceFormat == EStringDetector::UPPER_CAMEL_CASE || sourceFormat == EStringDetector::LOWER_CAMEL_CASE) &&
        (targetFormat == EStringDetector::UPPER_CAMEL_CASE || targetFormat == EStringDetector::LOWER_CAMEL_CASE)) {
        TString resultString = str;
        auto fn1 = [targetFormat](size_t, char c) {
            return (targetFormat == EStringDetector::UPPER_CAMEL_CASE) ? AsciiToUpper(c) : AsciiToLower(c);
        };
        resultString.Transform(fn1, 0, 1);
        return resultString;
    }

    // Converts between LOWER_CASE <-> UPPER_CASE
    if ((sourceFormat == EStringDetector::UPPER_CASE || sourceFormat == EStringDetector::LOWER_CASE) &&
        (targetFormat == EStringDetector::UPPER_CASE || targetFormat == EStringDetector::LOWER_CASE)) {
        TString resultString = str;
        auto fn2 = [targetFormat](size_t, char c) {
            return (targetFormat == EStringDetector::UPPER_CASE) ? AsciiToUpper(c) : AsciiToLower(c);
        };
        resultString.Transform(fn2, 0); // Whole string
        return resultString;
    }

    // Complex conversions (from camelcase to name with underscores or back)
    if (targetFormat == EStringDetector::UPPER_CASE || targetFormat == EStringDetector::LOWER_CASE) {
        Y_ENSURE(sourceFormat == EStringDetector::UPPER_CAMEL_CASE || sourceFormat == EStringDetector::LOWER_CAMEL_CASE);
        TString resultString;
        for (size_t i=0; i < str.size(); i++) {
            char sourceChar = (targetFormat == EStringDetector::UPPER_CASE) ? AsciiToUpper(str[i]) : AsciiToLower(str[i]);
            if (i > 0 && IsLower(str[i - 1]) && IsUpper(str[i])) {
                resultString.append('_');
            }
            resultString.append(sourceChar);
        }
        return resultString;
    }
    if (targetFormat == EStringDetector::UPPER_CAMEL_CASE || targetFormat == EStringDetector::LOWER_CAMEL_CASE) {
        Y_ENSURE(sourceFormat == EStringDetector::UPPER_CASE || sourceFormat == EStringDetector::LOWER_CASE);
        TString resultString;
        bool nextCharUpper = (targetFormat == EStringDetector::UPPER_CAMEL_CASE);
        for (size_t i=0; i < str.size(); i++) {
            if (str[i] == '_') {
                nextCharUpper = true;
                continue;
            }
            resultString.append(nextCharUpper ? AsciiToUpper(str[i]) : AsciiToLower(str[i]));
            nextCharUpper = false;
        }
        return resultString;
    }
    Y_ENSURE(false); // Impossible here
    return str;
}



} // namespace NAlice
