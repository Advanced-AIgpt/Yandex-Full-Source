#include <alice/cachalot/events/cachalot.ev.pb.h>
#include <alice/gproxy/library/events/gproxy.ev.pb.h>
#include <alice/rtlog/protos/rtlog.ev.pb.h>
#include <voicetech/library/idl/log/events.ev.pb.h>

#include <voicetech/library/evlogdump/evlogdump.h>

int main(int argc, const char** argv) {
    return EventLogDumpMain(argc, argv);
}
