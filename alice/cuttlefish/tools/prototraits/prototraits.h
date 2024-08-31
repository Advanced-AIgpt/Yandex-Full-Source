#pragma once

namespace NProtoTraits {

template <typename Type>
struct TMessageTraits {
    // empty
};


}  // namespace NProtoTraits


namespace NSM {

template <typename Type>
struct TMessageMeta {
    using TMessageType = Type;

    static constexpr const bool IsProtobufMessage = false;

    static constexpr const char *ApphostName = "undefined";

    static constexpr const char *MessageName = "undefined";
};

}   // namespace NSM
