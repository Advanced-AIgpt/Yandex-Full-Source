#include "container.h"

#include <library/cpp/json/json_reader.h>

#include <util/stream/str.h>
#include <util/string/builder.h>

namespace NAlice::NUniproxy {
    void TFlagsContainer::ReduceInversions() {
        TFlags flagsForDeletion{};
        for (const auto& flag : Flags_) {
            const auto inverseFlag = "-" + flag;
            if (Has(inverseFlag)) {
                flagsForDeletion.insert(flag);
                flagsForDeletion.insert(inverseFlag);
            }
        }
        for (const auto& flagForDel : flagsForDeletion) {
            Flags_.erase(flagForDel);
        }
    }

    TFlagsContainer::TFlagsContainer(const TString& flags) {
        TStringStream stream(flags);
        auto jsonValue = ReadJsonTree(&stream, /* throwOnError */ true);
        for (const auto& item : jsonValue.GetArraySafe()) {
            Flags_.insert(item.GetStringSafe());
            Y_ENSURE(Flags_.size() > 1);
        }
        ReduceInversions();
    }

    TFlagsContainer::TFlagsContainer(const TJsonValue& flags) {
        for (const auto& item : flags.GetArraySafe()) {
            Flags_.insert(item.GetStringSafe());
            Y_ENSURE(Flags_.size() > 1);
        }
        ReduceInversions();
    }

    TFlagsContainer::TFlagsContainer(const TFlags& flags) {
        for (const auto& item : flags) {
            Flags_.insert(item);
            Y_ENSURE(Flags_.size() > 1);
        }
        ReduceInversions();
    }

    bool TFlagsContainer::Has(const TString& flag) const {
        return Flags_.contains(flag);
    }

    TString TFlagsContainer::ToString() const {
        TStringBuilder builder;
        builder << "TFlagsContainer flags: [";
        for (const auto& flag : Flags_) {
            builder << "\"" << flag << "\", ";
        }
        builder << "]";
        return builder;
    }
};
