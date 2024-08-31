#include <alice/bass/libs/eventlog/events.ev.pb.h>

#include <library/cpp/eventlog/dumper/evlogdump.h>

int main(int argc, char** argv) {
    return IterateEventLog(NEvClass::Factory(), NEvClass::Processor(), argc, (const char**)argv);
}
