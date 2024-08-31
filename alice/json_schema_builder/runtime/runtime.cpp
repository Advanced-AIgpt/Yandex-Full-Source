#include "runtime.h"

#include <util/charset/utf8.h>

namespace NAlice::NJsonSchemaBuilder::NRuntime {

namespace {

re2::RE2 COLOR_PATTERN("#[[:xdigit:]]{6}|#[[:xdigit:]]{8}");

bool CheckPattern(const re2::RE2& pattern, const TStringBuf value) {
    if (!pattern.ok()) {
        ythrow TValidationError() << "Internal error: invalid pattern " << pattern.pattern();
    }
    return re2::RE2::FullMatch(re2::StringPiece(value.data(), value.size()), pattern);
}

}  // namespace

size_t GetNumUtf16CharsInUtf8String(const TStringBuf text) {
    auto it = reinterpret_cast<const unsigned char*>(text.begin());
    const auto end = it + text.size();

    size_t result = 0;
    while (it != end) {
        wchar32 rune = BROKEN_RUNE;
        const auto retcode = ReadUTF8CharAndAdvance(rune, it, end);
        if (retcode != RECODE_OK) {
            ythrow TValidationError() << "Invalid UTF-8 string";
        }

        // NOTE(a-square): the util has no dedicated function for
        // figuring out the number of UTF-16 words it takes to encode
        // a Unicode code point, so we roll one ourselves
        size_t runeLen = (rune <= static_cast<size_t>(0xFFFF)) ? 1 : 2;
        result += runeLen;
    }

    return result;
}

void ValidateColor(const TStringBuf value) {
    if (!CheckPattern(COLOR_PATTERN, value)) {
        ythrow TValidationError() << "Invalid color";
    }
}

void ValidateUri(const TStringBuf value) {
    // TODO(a-square): learn to validate URIs
    Y_UNUSED(value);
}

void ValidateJsonPayload(const NJson::TJsonValue& value) {
    if (!value.IsMap()) {
        ythrow TValidationError() << "JSON payload must a JSON object";
    }
}

void ValidateIntegerConstraint(const TStringBuf constraint,
                               const std::function<bool(const i64)>& validator,
                               const i64 value) {
    if (!validator(value)) {
        ythrow TValidationError() << "Integer constraint broken: " << constraint;
    }
}

void ValidateDoubleConstraint(const TStringBuf constraint,
                              const std::function<bool(const double)>& validator,
                              const double value) {
    if (!validator(value)) {
        ythrow TValidationError() << "Floating point constraint broken: " << constraint;
    }
}

void ValidateMinLength(size_t length, size_t minLength) {
    if (length < minLength) {
        ythrow TValidationError()
            << "String length in UTF-16 words is less than min_length = " << minLength;
    }
}

void ValidateMaxLength(size_t length, size_t maxLength) {
    if (length > maxLength) {
        ythrow TValidationError()
            << "String length in UTF-16 words is greater than max_length = " << maxLength;
    }
}

void ValidatePattern(const re2::RE2& pattern, const TStringBuf value) {
    if (!CheckPattern(pattern, value)) {
        ythrow TValidationError() << "Value doesn't match the pattern " << pattern.pattern();
    }
}

}  // namespace NAlice::NJsonSchemaBuilder::NRuntime
