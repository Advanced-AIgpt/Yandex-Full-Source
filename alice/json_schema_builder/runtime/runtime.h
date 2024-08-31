#pragma once

// some of these includes are for the benefit of the generated code
#include <contrib/libs/re2/re2/re2.h>
#include <library/cpp/json/json_value.h>
#include <util/string/cast.h>
#include <util/generic/maybe.h>
#include <util/generic/yexception.h>
#include <functional>

namespace NAlice::NJsonSchemaBuilder::NRuntime {

class TBuilder {
public:
    const NJson::TJsonValue& ValueWithoutValidation() const & {
        return Value_;
    }

    NJson::TJsonValue&& ValueWithoutValidation() && {
        return std::move(Value_);
    }

protected:
    NJson::TJsonValue Value_;
};

class TArrayBuilder : public TBuilder {
public:
    TArrayBuilder() {
        Value_.SetType(NJson::JSON_ARRAY);
    }

    TArrayBuilder& Add(const TBuilder& value) & {
        Value_.AppendValue(value.ValueWithoutValidation());
        return *this;
    }

    TArrayBuilder&& Add(const TBuilder& value) && {
        Value_.AppendValue(value.ValueWithoutValidation());
        return std::move(*this);
    }

    TArrayBuilder& Add(TBuilder&& value) & {
        Value_.AppendValue(std::move(value).ValueWithoutValidation());
        return *this;
    }

    TArrayBuilder&& Add(TBuilder&& value) && {
        Value_.AppendValue(std::move(value).ValueWithoutValidation());
        return std::move(*this);
    }
};

struct TValidationError : public yexception {
    using yexception::yexception;
};

size_t GetNumUtf16CharsInUtf8String(const TStringBuf text);

void ValidateColor(const TStringBuf value);
void ValidateUri(const TStringBuf value);
void ValidateJsonPayload(const NJson::TJsonValue& value);
void ValidateIntegerConstraint(const TStringBuf constraint,
                               const std::function<bool(const i64)>& validator,
                               const i64 value);
void ValidateDoubleConstraint(const TStringBuf constraint,
                              const std::function<bool(const double)>& validator,
                              const double value);
void ValidateMinLength(size_t length, size_t minLength);
void ValidateMaxLength(size_t length, size_t maxLength);
void ValidatePattern(const re2::RE2& pattern, const TStringBuf value);

}  // namespace NAlice::NJsonSchemaBuilder::NRuntime
