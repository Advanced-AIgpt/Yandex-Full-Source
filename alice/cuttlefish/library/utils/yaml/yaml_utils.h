#pragma once
#include <contrib/libs/yaml-cpp/include/yaml-cpp/yaml.h>
#include <util/generic/strbuf.h>
#include <util/system/type_name.h>
#include <util/generic/vector.h>
#include <util/generic/set.h>
#include <util/string/cast.h>


namespace NYamlUtils {

template <typename T>
T NodeAs(const YAML::Node& node, const TStringBuf nodeName = {});


namespace NPrivate {

template <typename>
constexpr bool IsSequentialContainer = false;

template <typename ...TArgs>
constexpr bool IsSequentialContainer<TVector<TArgs...>> = true;

template <typename ...TArgs>
constexpr bool IsSequentialContainer<TSet<TArgs...>> = true;


template <typename>
struct TAsSequentialContainer { };

template <typename T, typename ...Ts>
struct TAsSequentialContainer<TVector<T, Ts...>> {
    using TContainer = TVector<T, Ts...>;

    static void Convert(const YAML::Node& node, TContainer& dst) {
        TContainer tmp(Reserve(node.size()));
        for (const YAML::Node& it : node)
            tmp.push_back(NodeAs<T>(it));
        std::swap(dst, tmp);
    };
};

template <typename T, typename ...Ts>
struct TAsSequentialContainer<TSet<T, Ts...>> {
    using TContainer = TSet<T, Ts...>;

    static void Convert(const YAML::Node& node, TContainer& dst) {
        TContainer tmp;
        for (const YAML::Node& it : node)
            tmp.insert(NodeAs<T>(it));
        std::swap(dst, tmp);
    };
};

}  // namespace NPrivate


template <typename T>
void NodeAs(const YAML::Node& node, T& dst, const TStringBuf nodeName = {}) {
    if constexpr (NPrivate::IsSequentialContainer<T>) {
        Y_ENSURE(node.IsSequence(), "invalid YAML: '" << nodeName << "' is not sequence");
        NPrivate::TAsSequentialContainer<T>::Convert(node, dst);
        return;
    }

    Y_ENSURE(node.IsScalar(), "invalid YAML: '" << nodeName << "' is not scalar");
    const TStringBuf val(node.Scalar());
    Y_ENSURE(TryFromString(val, dst), "invalid YAML: couldn't convert node '" << nodeName << "' (=\"" << val << "\") into " << TypeName<T>());
}


template <typename T>
T NodeAs(const YAML::Node& node, const TStringBuf nodeName) {
    T ret;
    NodeAs<T>(node, ret, nodeName);
    return ret;
}


template <typename T>
bool FromSubnodeIfExist(const YAML::Node& doc, T& dst, TStringBuf name) {
    const YAML::Node node = doc[name.data()];
    if (!node.IsDefined())
        return false;
    NodeAs(node, dst, name);
    return true;
}


template <typename T>
void FromSubnode(const YAML::Node& doc, T& dst, TStringBuf name) {
    Y_ENSURE(FromSubnodeIfExist(doc, dst, name), "invalid YAML: required node '" << name << "' is absent");
}


template <typename T>
T SubnodeAs(const YAML::Node& doc, TStringBuf name) {
    T ret;
    FromSubnode(doc, ret, name);
    return ret;
}


template <typename T>
T SubnodeAs(const YAML::Node& doc, TStringBuf name, const T& defaultValue) {
    T ret;
    return FromSubnodeIfExist(doc, ret, name) ? ret : defaultValue;
}

}  // namespace NYamlUtils
