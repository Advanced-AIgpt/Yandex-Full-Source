#pragma once
#include <alice/cuttlefish/tools/prototraits/utils.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>

namespace NProtoTraits {

using namespace ::google::protobuf;
using namespace ::google::protobuf::compiler;

// ------------------------------------------------------------------------------------------------
void PrintFieldMethods(const FieldDescriptor& desc, io::Printer& printer);

void PrintStringFieldMethods(const FieldDescriptor& desc, io::Printer& printer);

void PrintNumericFieldMethods(const FieldDescriptor& desc, io::Printer& printer);

void PrintMessageFieldMethods(const FieldDescriptor& desc, io::Printer& printer);

void PrintEnumFieldMethods(const FieldDescriptor& desc, io::Printer& printer);

void PrintBooleanFieldMethods(const FieldDescriptor& desc, io::Printer& printer);

}  // namespace NProtoTraits
