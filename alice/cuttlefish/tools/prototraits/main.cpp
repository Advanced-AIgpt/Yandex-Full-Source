#include "generator.h"
#include <google/protobuf/compiler/plugin.h>


int main(int argc, char *argv[]) {
    NProtoTraits::TProtobufTraitsGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
