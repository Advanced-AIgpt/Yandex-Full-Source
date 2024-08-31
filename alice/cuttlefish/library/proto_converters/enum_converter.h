#pragma once
#include <util/string/ascii.h>
#include <util/generic/map.h>

namespace NAlice::NCuttlefish {

struct TCaseInsensitiveLess {
    bool operator()(const TStringBuf& lhs, const TStringBuf& rhs) const {
        return AsciiCompareIgnoreCase(lhs, rhs) < 0;
    }
};


template <typename EnumType>
class TEnumConverter {
public:
    TEnumConverter(const std::initializer_list<std::pair<TStringBuf, EnumType>>& list)
    {
        for (const auto& v : list) {
            Str2Enum[v.first] = v.second;
            Enum2Str[v.second] = v.first;
        }
    }

    inline EnumType FromString(TStringBuf str, EnumType defaultValue) const noexcept {
        const EnumType* value = Str2Enum.FindPtr(str);
        return value ? *value : defaultValue;
    }
    inline TStringBuf ToString(EnumType value, TStringBuf defaultStr = "") const noexcept {
        const TStringBuf* str = Enum2Str.FindPtr(value);
        return str ? *str : defaultStr;
    }

private:
    TMap<TStringBuf, EnumType, TCaseInsensitiveLess> Str2Enum;
    TMap<EnumType, TStringBuf> Enum2Str;
};


template <typename FieldTraits, auto DefaultValue>
struct TEnumFieldConverter : private TEnumConverter<typename FieldTraits::FieldType>
{
    using TEnumConverter<typename FieldTraits::FieldType>::TEnumConverter;

    inline void Parse(const NJson::TJsonValue& src, typename FieldTraits::MessageType& msg) const {
        FieldTraits::Set(msg, this->FromString(src.GetStringSafe(), DefaultValue));
    }

    template <typename WriterT>
    inline void Serialize(WriterT&& writer, const typename FieldTraits::MessageType& msg) const {
        writer.Value(this->ToString(FieldTraits::Get(msg)));
    }
};

}  // namespace NAlice::NCuttlefish
