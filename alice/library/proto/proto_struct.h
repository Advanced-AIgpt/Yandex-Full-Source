#pragma once

#include <alice/library/logger/logger.h>

#include <util/generic/maybe.h>
#include <util/generic/vector.h>
#include <util/string/split.h>

#include <google/protobuf/struct.pb.h>

//
// Proto Struct - additional helpers to work with google::protobuf::Struct object
// (read like JSON)
//

namespace NAlice {

//
// Helper class to parse google::protobuf::Struct like JSON object
//
class TProtoStructParser {
public:
    TProtoStructParser() = default;

    /*
        Setup functions
    */
    // Set deliiter for path (default is ".").
    // Delimiter is used to split long path (i.e. "Key1.Subkey2.Subkey3")
    void SetDelimiter(TStringBuf delimiter) {
        Delimiter_ = delimiter;
    }
    // Set Array flag (default is "[]")
    void SetArrayFlag(TStringBuf arrayFlag) {
        ArrayFlag_ = arrayFlag;
    }
    // Set logger and debug options
    void SetLogger(TRTLogger* logger, ELogPriority forTrace, ELogPriority forErrors, bool throwOnError) {
        Logger_ = logger;
        LevelForTrace_ = forTrace;
        LevelForError_ = forErrors;
        ThrowOnError_ = throwOnError;
    }

    /*
        Get values from Struct object by given path
        Return TMaybe<> objects
    */
    TMaybe<TString> GetValueString(const google::protobuf::Struct& proto, TString path) const;
    TMaybe<int> GetValueInt(const google::protobuf::Struct& proto, TString path) const;
    TMaybe<double> GetValueDouble(const google::protobuf::Struct& proto, TString path) const;
    TMaybe<bool> GetValueBool(const google::protobuf::Struct& proto, TString path) const;

    /*
        Get values from Struct object by given path
        Return default values
    */
    TString GetValueString(const google::protobuf::Struct& proto, TString path, TString defaultValue) const {
        auto res = GetValueString(proto, path);
        return res.Defined() ? *res : defaultValue;
    }
    int GetValueInt(const google::protobuf::Struct& proto, TString path, int defaultValue) const {
        auto res = GetValueInt(proto, path);
        return res.Defined() ? *res : defaultValue;
    }
    double GetValueDouble(const google::protobuf::Struct& proto, TString path, double defaultValue) const {
        auto res = GetValueDouble(proto, path);
        return res.Defined() ? *res : defaultValue;
    }
    bool GetValueBool(const google::protobuf::Struct& proto, TString path, bool defaultValue) const {
        auto res = GetValueBool(proto, path);
        return res.Defined() ? *res : defaultValue;
    }

    //
    // Inspecting a path
    //
    enum class EResult {
        Absent,  // The required path is absent
        Map,     // The required path exists and contains map of chilred object (i.e. {...})
        List,    // The required path exists and contains list of chilred object (i.e. [...])
        String,  // The required path exists as a string value
        Numeric, // The required path exists as a numeric value (i.e. double or integer)
        Boolean, // The required path exists as a boolean value
        Null     // The required path exists as a null value
    };
    // Inspect the path and return result
    EResult TestKey(const google::protobuf::Struct& proto, TString path) const;

    //
    // Get child key by path
    // Can be used for map objects ({})
    // Use google::protobuf::Struct::fields() to enumerate all objects
    //
    const google::protobuf::Struct& GetKey(const google::protobuf::Struct& proto, TString path) const;

    //
    // Get child key by path
    // Can be used for list objects ({})
    // Use google::protobuf::ListValue::Values() to enumerate all objects
    //
    const google::protobuf::ListValue& GetArray(const google::protobuf::Struct& proto, TString path) const;

    //
    // Enumerate all objects by he given path using callback function
    // You may use expressions like "subkey1.subkey2.[].subkey3" to enumerate all children objects after subkey3
    // Enumeration functions should return false to continue enumeration or true to stop enumeration
    // Functions return true if some callback return true, otherwise return false
    // `bool Callback(const google::protobuf::Struct& proto)`
    //
    template <typename T>
    bool EnumerateKeys(const google::protobuf::Struct& proto, TString path, T cb) const {
        TVector<TString> paths;
        Split(path, TString{Delimiter_}, paths);
        return EnumerateKeysImpl(&proto, paths, cb, 0);
    }

    // Convert internal string values into single text
    TMaybe<TString> EnumerateStringArray(const google::protobuf::ListValue& list, const TStringBuf delimiter) const {
        TStringBuilder sb;
        for (const auto& it: list.values()) {
            if (it.has_string_value()) {
                if (!sb.empty()) {
                    sb << delimiter;
                }
                sb << it.string_value();
            }
        }
        if (sb.Empty()) {
            return Nothing();
        }
        return TString{sb};
    }

private:
    //
    // Additional options
    // Can be used to override default processing
    //
    // Pointer to logger object (null - don't use logging, )
    TRTLogger* Logger_ = nullptr;
    // Log level to dump trace operations (N/A if Logger == nullptr)
    ELogPriority LevelForTrace_ = TLOG_DEBUG;
    // Log level to dump error operations (N/A if Logger == nullptr)
    ELogPriority LevelForError_ = TLOG_ERR;
    // Set to true to throw exception when error occured
    bool ThrowOnError_ = false;
    // Default sequence to split keys (i.e. "subkey1.subkey2")
    TString Delimiter_ = ".";
    // Default sequence to mark List object (i.e. "subkey1.[].subkey2"). Can be used in enumeration only!
    TString ArrayFlag_ = "[]";

private:
    void Trace(bool ErrorFlag, const TString& message) const;
    bool IsTraceNeed() const {
        return Logger_ != nullptr;
    }
    TString SplitPath(TString path, TVector<TString>& paths) const;
    const google::protobuf::Struct* GetSubKey(const google::protobuf::Struct* proto, const TVector<TString> paths) const;
    const google::protobuf::Value* GetFinalValue(const google::protobuf::Struct& proto, TString path) const;

    const google::protobuf::Value* GetChildValue(const google::protobuf::Struct* proto, TString singleKey) const;
    const google::protobuf::Value* GetChildValue(const google::protobuf::ListValue* list, TString singleKey) const;

    template <typename T>
    bool EnumerateKeysImpl(const google::protobuf::Struct* proto, const TVector<TString>& paths, T cb, size_t deep) const {
        // Either proto or list must be active
        if (proto == nullptr) {
            return false;
        }
        const google::protobuf::ListValue* list = nullptr;
        while (deep < paths.size()) {
            const TString& nodeName = paths[deep++];
            if (nodeName == ArrayFlag_) {
                // Use FOR EACH... at this level
                if (proto) {
                    for (const auto& it: proto->fields()) {
                        if (it.second.has_struct_value() && EnumerateKeysImpl(&it.second.struct_value(), paths, cb, deep)) {
                            return true;
                        }
                    }
                } else if (list) {
                    for (const auto& it: list->values()) {
                        if (it.has_struct_value() && EnumerateKeysImpl(&it.struct_value(), paths, cb, deep)) {
                            return true;
                        }
                    }
                } else {
                    Trace(false, "Both proto and list are nullptr, can't process [] enumeration");
                    return false;
                }
            } else {
                const google::protobuf::Value* value = nullptr;
                if (proto) {
                    value = GetChildValue(proto, nodeName);
                } else if (list) {
                    value = GetChildValue(list, nodeName);
                }
                if (value == nullptr) {
                    Trace(false, TStringBuilder{} << "Can't find '" << nodeName << "' at the current level. Source: " <<
                          (proto ? "proto" : "") << (list ? "list" : ""));
                    return false;
                }
                proto ? GetChildValue(proto, nodeName) : GetChildValue(list, nodeName);
                if (value->has_struct_value()) {
                    proto = &value->struct_value();
                    list = nullptr;
                } else if (value->has_list_value()) {
                    proto = nullptr;
                    list = &value->list_value();
                } else {
                    Trace(false, "Child proto object doen't have 'struct' or 'list' property");
                    return false;
                }
            }
        }
        if (proto) {
            return cb(*proto);
        }
        return false;
    }
};

} // namespace NAlice
