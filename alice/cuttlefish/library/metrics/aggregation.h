#pragma once


#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/flags.h>

#include <util/generic/hash_set.h>
#include <util/generic/map.h>


namespace NVoice {
namespace NMetrics {


enum class EClientType {
    User,
    Robot,
    None,
};


template <class T>
struct TLabelsBase {
    T           DeviceName      { "" };     // e.g. yandex-station, samsung, xiaomi ...
    T           GroupName       { "" };     // e.g. quasar, pp, navigator ...
    T           SubgroupName    { "" };     // e.g. prod, beta, dev ...
    T           AppId           { "" };     // e.g. ru.yandex.searchapp, aliced ...
    EClientType ClientType      { EClientType::Robot };     // e.g. EClientType::User, EClientType::Robot

    TLabelsBase() { }

    TLabelsBase(const TLabelsBase<T>&) = default;

    TLabelsBase(TLabelsBase<T>&&) = default;

    TLabelsBase<T>& operator=(const TLabelsBase<T>&) = default;

    template <class U>
    inline TLabelsBase<U> Into() const {
        TLabelsBase<U> ret;
        ret.DeviceName = U(DeviceName);
        ret.GroupName = U(GroupName);
        ret.SubgroupName = U(SubgroupName);
        ret.AppId = U(AppId);
        ret.ClientType = ClientType;
        return ret;
    }

    template <class U>
    inline TLabelsBase<T>& From(const TLabelsBase<U>& other) {
        *this = other.template Into<T>();
        return *this;
    }

};

using TClientInfo = TLabelsBase<TString>;

using TLabels = TLabelsBase<TStringBuf>;


enum class ELabel : int {
    Device,         // TClientInfo::DeviceName
    Surface,        // TClientInfo::GroupName
    SubgroupName,   // TClientInfo::SubgroupName
    Application,    // TClientInfo::AppId
    Max,
};


class TAggregationRules {
public:
    TAggregationRules() { }

    inline TAggregationRules& AddMap(ELabel label, TStringBuf from, TStringBuf to) {
        const int index = IndexFromLabel(label);
        if (index >= 0) {
            Maps[index].emplace(from, to);
        }
        return *this;
    }

    inline TAggregationRules& AddMap(ELabel label, const THashMap<TString, TString>& map) {
        const int index = IndexFromLabel(label);
        if (index >= 0) {
            for (auto it : map) {
                Maps[index].emplace(it);
            }
        }
        return *this;
    }

    inline TAggregationRules& AddFilter(ELabel label, TStringBuf value) {
        const int index = IndexFromLabel(label);
        if (index >= 0) {
            Filters[index].emplace(value);
        }
        return *this;
    }

    inline TAggregationRules& AddFilter(ELabel label, THashSet<TString> values) {
        const int index = IndexFromLabel(label);
        if (index >= 0) {
            for (auto it : values) {
                Filters[index].emplace(it);
            }
        }
        return *this;
    }

    inline TAggregationRules& SetDefaultLabel(ELabel label, TStringBuf value) {
        const int index = IndexFromLabel(label);
        if (index >= 0) {
            Default[static_cast<size_t>(label)] = value;
        }
        return *this;
    }

    inline TLabels Process(const TClientInfo& info) const {
        TLabels labels;
        labels.ClientType = info.ClientType;
        labels.DeviceName = MapAndFilterUnsafe(static_cast<int>(ELabel::Device), info.DeviceName);
        labels.GroupName = MapAndFilterUnsafe(static_cast<int>(ELabel::Surface), info.GroupName);
        labels.SubgroupName = MapAndFilterUnsafe(static_cast<int>(ELabel::SubgroupName), info.SubgroupName);
        labels.AppId = MapAndFilterUnsafe(static_cast<int>(ELabel::Application), info.AppId);
        return labels;
    }


    inline TStringBuf MapLabel(ELabel label, TStringBuf value) const {
        const int index = IndexFromLabel(label);
        if (index >= 0) {
            return MapLabelUnsafe(index, value);
        }
        return value;
    }


    inline TStringBuf FilterLabel(ELabel label, TStringBuf value) const {
        const int index = IndexFromLabel(label);
        if (index >= 0) {
            return FilterLabelUnsafe(index, value);
        }
        return value;
    }

private:    /* methods */
    int IndexFromLabel(ELabel label) const {
        const int value = static_cast<int>(label);
        if (value < 0 || value >= static_cast<int>(ELabel::Max)) {
            return -1;
        }
        return value;
    }

    inline TStringBuf FilterLabelUnsafe(int label, TStringBuf value) const {
        const auto &filters = Filters[label];
        if (filters.empty() && Default[label].empty()) {
            return value;
        } else if (filters.empty()) {
            return Default[label];
        } else if (filters.contains(value)) {
            return value;
        }
        return Default[label];
    }

    inline TStringBuf MapLabelUnsafe(int label, TStringBuf value) const {
        const auto &maps = Maps[label];
        auto it = maps.find(value);
        if (it != maps.end()) {
            return it->second;
        }
        return value;
    }

    inline TStringBuf MapAndFilterUnsafe(int label, TStringBuf value) const {
        return FilterLabelUnsafe(
            label,
            MapLabelUnsafe(label, value)
        );
    }

private:    /* data */
    static constexpr const int FilterSize = static_cast<int>(ELabel::Max);

    THashMap<TString, TString>  Maps[FilterSize];

    THashSet<TString>           Filters[FilterSize];

    TString                     Default[FilterSize];
};

}   // namespace NMetrics
}   // namespace NVoice
