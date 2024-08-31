#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <library/cpp/eventlog/dumper/evlogdump.h>

int main(int argc, const char** argv) {
    return IterateEventLog(NEvClass::Factory(), NEvClass::Processor(), argc, argv);
}
