#include "field_methods.h"


namespace NProtoTraits {

namespace {

void PrintHas(const FieldDescriptor& desc, io::Printer& printer) {
    printer.Print(GetFieldVariables(desc), R"__(
static inline bool IsSet(const $MESSAGE_TYPE$& msg) {
    return Has(msg);
}
static inline bool Has(const $MESSAGE_TYPE$& msg) {
    return msg.Has$FIELD_NAME$();
}
)__");

}

}

// ------------------------------------------------------------------------------------------------
void PrintFieldMethods(const FieldDescriptor& desc, io::Printer& printer)
{
    switch (desc.cpp_type()) {
    case FieldDescriptor::CPPTYPE_STRING:
        PrintStringFieldMethods(desc, printer);
        break;
    case FieldDescriptor::CPPTYPE_MESSAGE:
        PrintMessageFieldMethods(desc, printer);
        break;
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_UINT64:
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_FLOAT:
        PrintNumericFieldMethods(desc, printer);
        break;
    case FieldDescriptor::CPPTYPE_ENUM:
        PrintEnumFieldMethods(desc, printer);
        break;
    case FieldDescriptor::CPPTYPE_BOOL:
        PrintBooleanFieldMethods(desc, printer);
        break;
    default:
        break;
    }

    if (desc.is_optional())
        PrintHas(desc, printer);

}

// ------------------------------------------------------------------------------------------------
void PrintStringFieldMethods(const FieldDescriptor& desc, io::Printer& printer)
{
    if (desc.is_repeated()) {
        printer.Print(GetFieldVariables(desc), R"__(
static inline void Add($MESSAGE_TYPE$& msg, const $FIELD_TYPE$& val) {
    msg.Add$FIELD_NAME$(val);
}
static inline void Add($MESSAGE_TYPE$& msg, $FIELD_TYPE$&& val) {
    msg.Add$FIELD_NAME$(std::move(val));
}
static inline size_t Size(const $MESSAGE_TYPE$& msg) {
    return msg.$FIELD_NAME$Size();
}
static inline const $FIELD_TYPE$& Get(const $MESSAGE_TYPE$& msg, size_t index) {
    return msg.Get$FIELD_NAME$(index);
}
)__");
    } else {
        printer.Print(GetFieldVariables(desc), R"__(
static inline void Set($MESSAGE_TYPE$& msg, const $FIELD_TYPE$& val) {
    msg.Set$FIELD_NAME$(val);
}
static inline void Set($MESSAGE_TYPE$& msg, $FIELD_TYPE$&& val) {
    msg.Set$FIELD_NAME$(std::move(val));
}
static inline const $FIELD_TYPE$& Get(const $MESSAGE_TYPE$& msg) {
    return msg.Get$FIELD_NAME$();
}
)__");
    }
}

// ------------------------------------------------------------------------------------------------
void PrintNumericFieldMethods(const FieldDescriptor& desc, io::Printer& printer)
{
    if (desc.is_repeated()) {
        printer.Print(GetFieldVariables(desc), R"__(
static inline void Add($MESSAGE_TYPE$& msg, $FIELD_TYPE$ val) {
    msg.Add$FIELD_NAME$(val);
}
static inline size_t Size(const $MESSAGE_TYPE$& msg) {
    return msg.$FIELD_NAME$Size();
}
static inline $FIELD_TYPE$ Get(const $MESSAGE_TYPE$& msg, size_t index) {
    return msg.Get$FIELD_NAME$(index);
}
)__");
    } else {
        printer.Print(GetFieldVariables(desc), R"__(
static inline void Set($MESSAGE_TYPE$& msg, $FIELD_TYPE$ val) {
    msg.Set$FIELD_NAME$(val);
}
static inline $FIELD_TYPE$ Get(const $MESSAGE_TYPE$& msg) {
    return msg.Get$FIELD_NAME$();
}
)__");
    }
}

// ------------------------------------------------------------------------------------------------
void PrintMessageFieldMethods(const FieldDescriptor& desc, io::Printer& printer)
{

    if (desc.is_map()) {
        printer.Print(GetFieldVariables(desc), R"__(
static inline size_t Size(const $MESSAGE_TYPE$& msg) {
    return msg.$FIELD_NAME$Size();
}
static inline const FieldType& Get(const $MESSAGE_TYPE$& msg) {
    return msg.Get$FIELD_NAME$();
}
static inline FieldType* Mutable($MESSAGE_TYPE$& msg) {
    return msg.Mutable$FIELD_NAME$();
}
)__");
    } else if (desc.is_repeated()) {
        printer.Print(GetFieldVariables(desc), R"__(
static inline $FIELD_TYPE$* Add($MESSAGE_TYPE$& msg) {
    return msg.Add$FIELD_NAME$();
}
static inline size_t Size(const $MESSAGE_TYPE$& msg) {
    return msg.$FIELD_NAME$Size();
}
static inline const $FIELD_TYPE$& Get(const $MESSAGE_TYPE$& msg, size_t index) {
    return msg.Get$FIELD_NAME$(index);
}
)__");
    } else {
        printer.Print(GetFieldVariables(desc), R"__(
static inline const $FIELD_TYPE$& Get(const $MESSAGE_TYPE$& msg) {
    return msg.Get$FIELD_NAME$();
}
static inline $FIELD_TYPE$* Mutable($MESSAGE_TYPE$& msg) {
    return msg.Mutable$FIELD_NAME$();
}
)__");
    }
}

// ------------------------------------------------------------------------------------------------
void PrintEnumFieldMethods(const FieldDescriptor& desc, io::Printer& printer)
{
    printer.Print(GetFieldVariables(desc), R"__(
static inline void Set($MESSAGE_TYPE$& msg, $FIELD_TYPE$ val) {
    msg.Set$FIELD_NAME$(val);
}
static inline $FIELD_TYPE$ Get(const $MESSAGE_TYPE$& msg) {
    return msg.Get$FIELD_NAME$();
}
static inline bool IsValid(int val) {
    return $FIELD_TYPE$_IsValid(val);
}
static inline const TString& GetName($FIELD_TYPE$ val) {
    return $FIELD_TYPE$_Name(val);
}
static inline bool Parse(const TString& name, $FIELD_TYPE$* val) {
    return $FIELD_TYPE$_Parse(name, val);
}
)__");
}

void PrintBooleanFieldMethods(const FieldDescriptor& desc, io::Printer& printer)
{
    printer.Print(GetFieldVariables(desc), R"__(
static inline void Set($MESSAGE_TYPE$& msg, $FIELD_TYPE$ val) {
    msg.Set$FIELD_NAME$(val);
}
static inline $FIELD_TYPE$ Get(const $MESSAGE_TYPE$& msg) {
    return msg.Get$FIELD_NAME$();
}
)__");
}

}  // namespace NProtoTraits
