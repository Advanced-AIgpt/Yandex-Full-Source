#pragma once
#include <library/cpp/testing/unittest/registar.h>
#include <google/protobuf/util/json_util.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>


#define UNIT_ASSERT_CONTAINS(CONTAINER, VALUE) {                            \
    bool found = false;                                                     \
    for (const auto& it : CONTAINER) {                                      \
        if (it == VALUE) {                                                  \
            found = true;                                                   \
            break;                                                          \
        }                                                                   \
    }                                                                       \
    UNIT_ASSERT_C(found, "There is no '" << VALUE << "' in " #CONTAINER);   \
}


inline NJson::TJsonValue ReadJsonValue(TStringBuf raw) {
    NJson::TJsonValue value;
    NJson::ReadJsonTree(raw, &value, /*throwOnError =*/ true);
    return value;
}

inline TString AsSortedJsonString(const NJson::TJsonValue& x) {
    return NJson::WriteJson(&x, /* formatOutput = */ false, /* sortKeys = */ true);
}

inline TString AsSortedJsonString(TStringBuf x) {
    return AsSortedJsonString(ReadJsonValue(x));
}

inline TString AsSortedJsonString(const TString& x) {
    return AsSortedJsonString(ReadJsonValue(x));
}

inline TString AsSortedJsonString(const char* x) {
    return AsSortedJsonString(ReadJsonValue(x));
}

inline TString AsSortedJsonString(const ::google::protobuf::Message& x) {
    TString asJson;
    ::google::protobuf::util::MessageToJsonString(x, &asJson);
    return AsSortedJsonString(TStringBuf(asJson));
}

inline NJson::TJsonValue AsJsonValue(const ::google::protobuf::Message& x) {
    TString asJson;
    ::google::protobuf::util::MessageToJsonString(x, &asJson);
    return ReadJsonValue(asJson);
}

template <typename MessageType>
inline MessageType FromJsonString(const TString& input) {
    MessageType msg;
    Y_ENSURE(::google::protobuf::util::JsonStringToMessage(input, &msg).ok());
    return msg;
}