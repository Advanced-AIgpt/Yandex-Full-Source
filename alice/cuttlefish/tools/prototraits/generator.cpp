#include "generator.h"
#include <alice/cuttlefish/tools/prototraits/utils.h>
#include <alice/cuttlefish/tools/prototraits/field_methods.h>

#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>
#include <util/generic/yexception.h>

#include <alice/cuttlefish/tools/prototraits/protos/prototraits.pb.h>


namespace NProtoTraits {

namespace {

void PrintFieldTraits(const FieldDescriptor& desc, io::Printer& printer)
{
    printer.Print(GetFieldVariables(desc), R"__(
struct $FIELD_NAME$ {
    using MessageType = $MESSAGE_TYPE$;
    using FieldType = $FIELD_TYPE$;

    static constexpr TStringBuf FieldName = "$FIELD_NAME$";
)__");

    printer.Indent();
    printer.Indent();
    PrintFieldMethods(desc, printer);
    printer.Outdent();
    printer.Outdent();

    printer.Print("};\n");
}

void PrintMessageTraits(const Descriptor& messageDesc, io::Printer& printer, bool nested = false)
{
    const std::map<TProtoStringType, TProtoStringType> vars({
        {"MESSAGE_TYPE_FULL", FullTypeName(messageDesc)},
        {"MESSAGE_TYPE", messageDesc.name()}
    });

    if (!nested) {
        printer.Print(vars, R"__(

template <>
struct TMessageTraits<$MESSAGE_TYPE_FULL$> {
)__");
    } else {
        printer.Print(vars, R"__(
struct $MESSAGE_TYPE$ {
)__");
    }

    printer.Indent();
    printer.Indent();
    for (int i = 0; i < messageDesc.nested_type_count(); ++i) {
        const Descriptor* nestedDesc = messageDesc.nested_type(i);
        Y_ENSURE(nestedDesc != nullptr);
        if (nestedDesc->options().map_entry())
            continue;
        PrintMessageTraits(*nestedDesc, printer, true);
    }

    for (int i = 0; i < messageDesc.field_count(); ++i) {
        const FieldDescriptor* fieldDesc = messageDesc.field(i);
        Y_ENSURE(fieldDesc != nullptr && !fieldDesc->is_extension());
        PrintFieldTraits(*fieldDesc, printer);
    }
    printer.Outdent();
    printer.Outdent();

    printer.Print(vars, "};\n");

}


void PrintMessageMeta(const Descriptor& messageDesc, io::Printer& printer) {
    const TString apphostName = messageDesc.options().GetExtension(apphost_name);

    if (apphostName.empty()) {
        return;
    }

    const std::map<TProtoStringType, TProtoStringType> vars({
        {"MESSAGE_TYPE_FULL", FullTypeName(messageDesc)},
        {"MESSAGE_TYPE", messageDesc.name()},
        {"APPHOST_NAME", apphostName}
    });

    printer.Print(vars, R"__(
template <>
struct TMessageMeta<$MESSAGE_TYPE_FULL$> {
)__");

    printer.Indent();
    printer.Indent();
    printer.Print(vars, "static constexpr const bool IsProtobufMessage = true;\n");
    printer.Print(vars, "static constexpr const char *ApphostName = \"$APPHOST_NAME$\";\n");
    printer.Print(vars, "static constexpr const char *MessageName = \"$MESSAGE_TYPE_FULL$\";\n");
    printer.Outdent();
    printer.Outdent();
    printer.Print(vars, "};\n");

}

}  // anonymous namespace


bool TProtobufTraitsGenerator::Generate(
    const FileDescriptor* file,
    const TProtoStringType& /*parameter*/,
    GeneratorContext* generator_context,
    TProtoStringType* /*error*/
) const {
    THolder<io::ZeroCopyOutputStream> stream(generator_context->Open(ProtobufTraitsHeader(file)));
    io::Printer printer(stream.Get(), '$');

    printer.Print(
        "#pragma once\n"
        "#include <$PB$>\n"
        "#include <util/generic/strbuf.h>\n"
        "#include <alice/cuttlefish/tools/prototraits/prototraits.h>\n"
        "\n",
        "PB", ProtobufHeader(file)
    );

    // for (int i = 0; i < file->dependency_count(); ++i) {
    //     const FileDescriptor* importedFile = file->dependency(i);
    //     TProtobufTraitsGenerator().Generate(importedFile, parameter, generator_context, error);
    //     printer.Print("#include <$PBT$>\n", "PBT", ProtobufTraitsHeader(importedFile));
    //     Cerr << "Generate traits of " << importedFile->name() << " into " << ProtobufTraitsHeader(importedFile) << Endl;
    // }

    printer.Print("\nnamespace NProtoTraits {\n");

    for (int i = 0; i < file->message_type_count(); ++i) {
        const Descriptor* messageDesc = file->message_type(i);
        Y_ENSURE(messageDesc != nullptr);
        PrintMessageTraits(*messageDesc, printer);

        const TString enclosingNamespaceName = FullEnclosingNamespaceName(*messageDesc);
        if (enclosingNamespaceName) {
            printer.Print(
                "\nnamespace $NAMESPACE$ {\n    using $NAME$ = TMessageTraits<$FULL_NAME$>;\n}",
                "NAMESPACE", enclosingNamespaceName,
                "NAME", messageDesc->name(),
                "FULL_NAME", FullTypeName(messageDesc)
            );
        } else {
            printer.Print(
                "\nusing $NAME$ = TMessageTraits<$FULL_NAME$>;\n",
                "NAME", messageDesc->name(),
                "FULL_NAME", FullTypeName(messageDesc)
            );
        }
    }

    printer.Print("\n}  // namespace NProtoTraits\n");

    printer.Print("\nnamespace NSM {\n");
    for (int i = 0; i < file->message_type_count(); ++i) {
        const Descriptor* messageDesc = file->message_type(i);
        Y_ENSURE(messageDesc != nullptr);
        PrintMessageMeta(*messageDesc, printer);
    }
    printer.Print("\n}  // namespace NSM\n");

    return true;
}

}  // namespace NProtoTraits

