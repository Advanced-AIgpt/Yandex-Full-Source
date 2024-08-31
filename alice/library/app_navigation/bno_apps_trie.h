#pragma once

#include <library/cpp/containers/comptrie/comptrie.h>
#include <util/generic/string.h>
#include <util/string/builder.h>


namespace NAlice {

/**
 * Список мобильных приложений, относящихся к БНО (организации, сайту, ...)
 */
struct TBnoApp {
    TString AndroidAppId;
    TString IPhoneAppId;
    TString IPadAppId; // пока не используется, но есть в данных

    TString ToString() const {
        return TStringBuilder() << "(" << AndroidAppId << ", " << IPhoneAppId << ", " << IPadAppId << ")";
    }

    bool operator ==(const TBnoApp& other) const {
        return AndroidAppId == other.AndroidAppId && IPhoneAppId == other.IPhoneAppId && IPadAppId == other.IPadAppId;
    }

    bool operator !=(const TBnoApp& other) const {
        return !(*this == other);
    }

    bool MergeWith(const TBnoApp& app) {
        Merge(AndroidAppId, app.AndroidAppId);
        Merge(IPhoneAppId, app.IPhoneAppId);
        Merge(IPadAppId, app.IPadAppId);
        return true;
    }

private:
    template <typename T>
    static bool Merge(T& l, const T& r) {
        if (l == r)
            return true;
        if (l && r)
            return false;
        if (r)
            l = r;
        return true;
    }
};

/**
 * Packer (serializer/deserializer) для хранения TBnoApp в трае.
 */
struct TBnoAppPacker {
    inline void UnpackLeaf(const char* buffer, TBnoApp& v) const {
        NPackers::TPacker<TString>().UnpackLeaf(buffer, v.AndroidAppId);
        size_t size = NPackers::TPacker<TString>().SkipLeaf(buffer);
        NPackers::TPacker<TString>().UnpackLeaf(buffer + size, v.IPhoneAppId);
        size += NPackers::TPacker<TString>().SkipLeaf(buffer + size);
        NPackers::TPacker<TString>().UnpackLeaf(buffer + size, v.IPadAppId);
    }
    inline void PackLeaf(char* buffer, const TBnoApp& v, size_t size) const {
        size_t size1 = NPackers::TPacker<TString>().MeasureLeaf(v.AndroidAppId);
        NPackers::TPacker<TString>().PackLeaf(buffer, v.AndroidAppId, size1);
        size_t size2 = NPackers::TPacker<TString>().MeasureLeaf(v.IPhoneAppId);
        NPackers::TPacker<TString>().PackLeaf(buffer + size1, v.IPhoneAppId, size2);
        size_t size3 = NPackers::TPacker<TString>().MeasureLeaf(v.IPadAppId);
        NPackers::TPacker<TString>().PackLeaf(buffer + size1 + size2, v.IPadAppId, size3);
        Y_ASSERT(size == size1 + size2 + size3);
    }
    inline size_t MeasureLeaf(const TBnoApp& v) const {
        size_t size1 = NPackers::TPacker<TString>().MeasureLeaf(v.AndroidAppId);
        size_t size2 = NPackers::TPacker<TString>().MeasureLeaf(v.IPhoneAppId);
        size_t size3 = NPackers::TPacker<TString>().MeasureLeaf(v.IPadAppId);
        return size1 + size2 + size3;
    }
    inline size_t SkipLeaf(const char* buffer) const {
        size_t size1 = NPackers::TPacker<TString>().SkipLeaf(buffer);
        size_t size2 = NPackers::TPacker<TString>().SkipLeaf(buffer + size1);
        size_t size3 = NPackers::TPacker<TString>().SkipLeaf(buffer + size1 + size2);
        return size1 + size2 + size3;
    }
};

// docId -> apps
using TBnoAppsTrie = TCompactTrie<char, TBnoApp, TBnoAppPacker>;

}

inline IOutputStream& operator<<(IOutputStream& out, const NAlice::TBnoApp& app) {
    out << app.ToString();
    return out;
}
