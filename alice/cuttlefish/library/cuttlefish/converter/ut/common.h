#pragma once
#include <google/protobuf/util/json_util.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>


inline NJson::TJsonValue ReadJsonValue(TStringBuf raw) {
    NJson::TJsonValue value;
    NJson::ReadJsonTree(raw, &value, /*throwOnError = */ false);
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