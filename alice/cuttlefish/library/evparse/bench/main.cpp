#include <library/cpp/testing/benchmark/bench.h>

#include <library/cpp/resource/resource.h>
#include <util/stream/file.h>

#include <alice/cuttlefish/library/evparse/evparse.h>


#define X_CPU_BENCHMARK(Name, FnName, Resource) \
    Y_CPU_BENCHMARK(Name, iface) { \
        const TString Data = NResource::Find(Resource); \
        NAliceProtocol::TEventHeader Header; \
        for (size_t i = 0; i < iface.Iterations(); ++i) { \
            Y_DO_NOT_OPTIMIZE_AWAY(FnName(Data, Header)); \
        } \
    }


X_CPU_BENCHMARK(Json_Value_Large, ParseEvent, "/input.1.txt")
X_CPU_BENCHMARK(Json_Parse_Large, ParseEventFast, "/input.1.txt")

X_CPU_BENCHMARK(Json_Value_Small, ParseEvent, "/input.2.txt")
X_CPU_BENCHMARK(Json_Parse_Small, ParseEventFast, "/input.2.txt")
