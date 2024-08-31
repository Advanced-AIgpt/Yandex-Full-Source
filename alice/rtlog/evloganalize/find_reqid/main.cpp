#include <alice/rtlog/protos/rtlog.ev.pb.h>

#include <library/cpp/eventlog/dumper/evlogdump.h>
#include <library/cpp/getopt/last_getopt.h>

struct TRequestLogInfo{
    ui64 StartTimestamp;
    ui64 EndTimestamp;
    TVector<ui32> FrameIds;
    TString RequestString;
};

class MyProcessor: public IEventProcessor {
public:
    MyProcessor(const TString& interestingReqId):
        InterestingReqId(interestingReqId) {
    }

    void ProcessEvent(const TEvent* ev) override {
        if (!InterestingReqId.empty() && LastReqId == InterestingReqId) {
            Cout << ev->ToString() << Endl;
        }
        if (ev->GetProto()->GetTypeName() == "NRTLogEvents.ActivationStarted") {
            LastReqId = ev->Get<NRTLogEvents::ActivationStarted>()->GetReqId();
        }
    }

private:
    TString LastReqId;
    TString InterestingReqId;
};

int main(int argc, char** argv) {
    TString interestingReqId;
    NLastGetopt::TOpts opts;
    opts.AddLongOption("reqid").StoreResult(&interestingReqId).Required();
    opts.SetFreeArgsMax(100);
    opts.SetFreeArgsMin(1);
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    auto args = res.GetFreeArgs();

    MyProcessor* processor;
    processor = new MyProcessor(interestingReqId);
    for (auto elem : res.GetFreeArgs()) {
        const char* crutch[] = {
           "",
           elem.data()
        };
        IterateEventLog(NEvClass::Factory(), processor, 2, crutch);
    }
    return 0;
}
