#include "evparse.h"


#include <library/cpp/json/easy_parse/json_easy_parser.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_reader.h>


bool ParseEvent(TStringBuf data, NAliceProtocol::TEventHeader& header) {
    NJson::TJsonValue ev;
    NJson::ReadJsonFastTree(data, &ev);

    const NJson::TJsonValue& h = ev["Event"]["event"]["header"];

    header.SetNamespace(h["namespace"].GetString());
    header.SetName(h["name"].GetString());

    if (h.Has("messageId")) {
        header.SetMessageId(h["messageId"].GetString());
    }

    if (h.Has("streamId")) {
        header.SetStreamId(h["streamId"].GetInteger());
    }
    return true;
}


class TEventHeaderParser : public NJson::TJsonCallbacks {
public:
    enum EState {
        Init,
        EventOuter,
        EventInner,
        Header,
        Final
    };

    enum EKey {
        None,

        Namespace,
        Name,
        MessageId,
        RefMessageId,
        StreamId,
        EventId,

        EventOuterMap,
        EventInnerMap,
        HeaderMap
    };


    TEventHeaderParser(NAliceProtocol::TEventHeader& header)
        : Header_(header)
    { }


    bool OnInteger(long long value) override {
        switch (Key_) {
            case StreamId:
                Header_.SetStreamId(value);
                break;
            default:
                break;
        }
        return true;
    }

    bool OnUInteger(unsigned long long value) override {
        switch (Key_) {
            case StreamId:
                Header_.SetStreamId(value);
                break;
            default:
                break;
        }
        return true;
    }

    bool OnString(const TStringBuf& s) override {
        switch (Key_) {
            case MessageId:
                Header_.SetMessageId(TString{s});
                break;
            case Namespace:
                Header_.SetNamespace(TString{s});
                break;
            case Name:
                Header_.SetName(TString{s});
                break;
            case RefMessageId:
                Header_.SetRefMessageId(TString{s});
                break;
            default:
                break;
        }
        return true;
    }

    bool OnStringNoCopy(const TStringBuf& s) override {
        switch (Key_) {
            case MessageId:
                Header_.SetMessageId(TString{s});
                break;
            case Namespace:
                Header_.SetNamespace(TString{s});
                break;
            case Name:
                Header_.SetName(TString{s});
                break;
            case RefMessageId:
                Header_.SetRefMessageId(TString{s});
                break;
            default:
                break;
        }
        return true;
    }

    bool OnMapKeyNoCopy(const TStringBuf& name) override {
        switch (State_) {
            case Init:
                if (name == "Event") {
                    Key_ = EventOuterMap;
                }
                break;
            case EventOuter:
                if (name == "event") {
                    Key_ = EventInnerMap;
                }
                break;
            case EventInner:
                if (name == "header") {
                    Key_ = HeaderMap;
                }
                break;
            case Header:
                if (name == "messageId") {
                    Key_ = MessageId;
                } else if (name == "namespace") {
                    Key_ = Namespace;
                } else if (name == "name") {
                    Key_ = Name;
                } else if (name == "streamId") {
                    Key_ = StreamId;
                } else if (name == "refMessageId") {
                    Key_ = RefMessageId;
                }
                break;
            case Final:
                break;
            default:
                break;
        };
        return true;
    }

    bool OnMapKey(const TStringBuf& name) override{
        switch (State_) {
            case Init:
                if (name == "Event") {
                    Key_ = EventOuterMap;
                }
                break;
            case EventOuter:
                if (name == "event") {
                    Key_ = EventInnerMap;
                }
                break;
            case EventInner:
                if (name == "header") {
                    Key_ = HeaderMap;
                }
                break;
            case Header:
                if (name == "messageId") {
                    Key_ = MessageId;
                } else if (name == "namespace") {
                    Key_ = Namespace;
                } else if (name == "name") {
                    Key_ = Name;
                } else if (name == "streamId") {
                    Key_ = StreamId;
                } else if (name == "refMessageId") {
                    Key_ = RefMessageId;
                }
                break;
            case Final:
                break;
            default:
                break;
        };
        return true;
    }

    bool OnOpenMap() override {
        switch (State_) {
            case Init:
                if (Key_ == EventOuterMap) {
                    State_ = EventOuter;
                    Key_ = None;
                }
                break;

            case EventOuter:
                if (Key_ == EventInnerMap) {
                    State_ = EventInner;
                    Key_ = None;
                }
                break;

            case EventInner:
                if (Key_ == HeaderMap) {
                    State_ = Header;
                    Key_ = None;
                }
                break;
            case Header:
                break;
            case Final:
                break;
        };
        return true;
    }

    bool OnCloseMap() override {
        switch (State_) {
            case Init:
                break;
            case EventOuter:
                State_ = Init;
                break;
            case EventInner:
                State_ = EventOuter;
                break;
            case Header:
                State_ = EventInner;
                return false;
                break;
            case Final:
                break;
        };
        return true;
    }

private:
    NAliceProtocol::TEventHeader&   Header_;
    EState          State_  { EState::Init };
    EKey            Key_    { EKey::None };
};


bool ParseEventFast(TStringBuf data, NAliceProtocol::TEventHeader& header) {
    TEventHeaderParser parser(header);
    NJson::ReadJsonFast(data, &parser);
    return true;
}
