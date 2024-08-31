#include "proto_adapter.h"

namespace NAlice
{
    bool TProtoAdapter::IsExceptionSafeMode() const {
        return NoException_;
    }

    void TProtoAdapter::SetExceptionSafeMode(bool mode) {
        NoException_ = mode;
    }

    bool TProtoAdapter::Has(const TStringBuf& key) const {
        if (std::holds_alternative<GStruct>(State_)) {
            const auto st = std::get<GStruct>(State_);
            const auto iter = st.fields().find(key);
            return iter != st.fields().end();
        } else {
            return false;
        }
    };

    TProtoAdapter TProtoAdapter::operator[](const TStringBuf& key) const {
        if (std::holds_alternative<GStruct>(State_)) {
            const auto& st = std::get<GStruct>(State_);
            const auto iter = st.fields().find(key);
            if (iter != st.fields().end()) {
                if (iter->second.has_struct_value()) {
                    return TProtoAdapter(iter->second.struct_value(), IsExceptionSafeMode());
                } else {
                    return TProtoAdapter(iter->second, IsExceptionSafeMode());
                }
            }
        }
        if (NoException_) {
            GValue nothing{};
            return TProtoAdapter(nothing);
        }
        throw TProtoAdapterTypeException();
    }

    #define WRAP_GETTER(GETTER)                   \
        if (std::holds_alternative<GValue>(State_)) {   \
            const auto& val = std::get<GValue>(State_); \
            if (val.has_##GETTER()) {             \
                return val.GETTER();            \
            }                                           \
        }                                               \
        if (NoException_) {                             \
            return {};                                  \
        }                                               \
        throw TProtoAdapterTypeException();

    TString TProtoAdapter::GetString() const {
        WRAP_GETTER(string_value)
    }

    TString TProtoAdapter::GetStringRobust() const {
        if (std::holds_alternative<GValue>(State_)) {
            const auto& val = std::get<GValue>(State_);
            if (val.has_string_value()) {
                return (val.string_value());
            } else {
                return val.DebugString();
            }
        }
        return {}; 
    }

    double TProtoAdapter::GetDouble() const {
        WRAP_GETTER(number_value)
    }

    unsigned long long TProtoAdapter::GetUInteger() const {
        if (std::holds_alternative<GValue>(State_)) {
            const auto& val = std::get<GValue>(State_);
            if (val.has_number_value()) {
                return static_cast<unsigned long long>(val.number_value());
            }
        }
        if (NoException_) {
            return {}; 
        }
        throw TProtoAdapterTypeException();
    }

    long long TProtoAdapter::GetInteger() const {
        if (std::holds_alternative<GValue>(State_)) {
            const auto& val = std::get<GValue>(State_);
            if (val.has_number_value()) {
                return static_cast<long long>(val.number_value());
            }
        }
        if (NoException_) {
            return {}; 
        }
        throw TProtoAdapterTypeException();
    }

    bool TProtoAdapter::GetBoolean() const {
        WRAP_GETTER(bool_value)
    }

    #undef WRAP_GETTER

    TProtoAdapterArray TProtoAdapter::GetArray() const {
        if (std::holds_alternative<GValue>(State_)) {
            const auto& val = std::get<GValue>(State_);
            if (val.has_list_value()) {
                const auto& list_value = val.list_value();
                TProtoAdapterArray result;
                for (int i = 0; i < list_value.values_size(); ++i) {
                    const auto value_i = list_value.values(i);
                    if (value_i.has_struct_value()) {
                        result.push_back(TProtoAdapter(value_i.struct_value(), IsExceptionSafeMode()));
                    } else {
                        result.push_back(TProtoAdapter(value_i, IsExceptionSafeMode()));
                    }
                }
                return result;
            }
        }
        if (NoException_) {
            return {};
        }
        throw TProtoAdapterTypeException();
    }

} // namespace NAlice
