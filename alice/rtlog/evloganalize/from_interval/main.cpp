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
    void ProcessEvent(const TEvent* ev) override {
        if (ev->GetProto()->GetDescriptor()->name() == "LogEvent") {
            NRTLogEvents::LogEvent msg;
            msg.CopyFrom(*ev->GetProto());
            if (msg.GetFields().find("log_type") != msg.GetFields().end() && msg.GetFields().at("log_type") == "megamind_request") {
                if (!RequestsInfo.contains(LastReqId) || RequestsInfo[LastReqId].StartTimestamp == 0) {
                    RequestsInfo[LastReqId].StartTimestamp = ev->Timestamp;
                }
                RequestsInfo[LastReqId].FrameIds.push_back(ev->FrameId);
                RequestsInfo[LastReqId].RequestString += "\t" + ev->Get<NRTLogEvents::TLogEvent>()->GetMessage();
            }
        }
        if (ev->GetProto()->GetTypeName() == "NRTLogEvents.ActivationStarted") {
            LastReqId = ev->Get<NRTLogEvents::ActivationStarted>()->GetReqId();
        }
        if (ev->GetProto()->GetTypeName() == "NEventLogInternal.TEndOfFrameEvent") {
            RequestsInfo[LastReqId].EndTimestamp = ev->Timestamp;
        }
    }

public:
    THashMap<TString, TRequestLogInfo> RequestsInfo; //key is ReqId

private:
    TString LastReqId;
};

int main(int argc, char** argv) {
    ui64 durationFrom;
    ui64 durationTo;
    TString interestingReqId;
    NLastGetopt::TOpts opts;
    opts.AddLongOption("from").StoreResult(&durationFrom).DefaultValue("0");
    opts.AddLongOption("to").StoreResult(&durationTo).DefaultValue("200000");
    opts.SetFreeArgsNum(1);
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    auto args = res.GetFreeArgs();
    const char* crutch[] = {
        "",
        args[0].data()
    };
    auto processor = new MyProcessor();
    IterateEventLog(NEvClass::Factory(), processor, 2, crutch);
    for (auto elem : processor->RequestsInfo) {
        TRequestLogInfo requestInfo = elem.second;
        if (requestInfo.RequestString.Empty()) {
            continue;
        }
        if ((requestInfo.EndTimestamp - requestInfo.StartTimestamp) / 1000 < durationFrom || (requestInfo.EndTimestamp - requestInfo.StartTimestamp) / 1000 > durationTo) {
            continue;
        }
        Cout << requestInfo.StartTimestamp << "\t" << (requestInfo.EndTimestamp - requestInfo.StartTimestamp) << "\t" << elem.first << "\t" << requestInfo.RequestString << Endl;
    }
}
