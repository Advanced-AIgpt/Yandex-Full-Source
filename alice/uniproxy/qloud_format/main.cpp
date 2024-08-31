#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/system/interrupt_signals.h>
#include <util/system/hostname.h>
#include <util/datetime/base.h>
#include <library/cpp/json/json_reader.h>


const TString hostname = FQDNHostName();


void SignalsHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM)
        exit(0);
    exit(-signum);
}


bool ValidateJsonInSessionlog(TStringBuf message) {
    static const TStringBuf sessionlogPrefix("SESSIONLOG: ");

    if (message.SkipPrefix(sessionlogPrefix)) {
        if (!NJson::ValidateJson(message)) {
            return false;
        }
    }

    return true;
}


template <typename ...T>
bool TrySplit(TStringBuf row, char delim, TStringBuf& left, TStringBuf& right, T& ...others) {
    if (row.TrySplit(delim, left, right)) {
        if constexpr (sizeof...(others) > 0)
            return TrySplit(right, delim, right, others...);
        return true;
    }
    return false;
}


class TEscaper {
public:
    TEscaper(TStringBuf string)
        : Src(string)
    { }

    void Write(IOutputStream& out) const {
        #define MATCH(sym, escaped)     \
            case sym:                   \
                out.Write(escaped);     \
                break

        for (char l : Src) {
            switch (l) {
                MATCH('"', "\\\"");
                MATCH('\\', "\\\\");
                MATCH('\b', "\\b");
                MATCH('\f', "\\f");
                MATCH('\n', "\\n");
                MATCH('\r', "\\r");
                MATCH('\t', "\\t");
                default:
                    out.Write(l);
                    break;
            }
        }

        #undef MATCH
    }

private:
    TStringBuf Src;
};

inline IOutputStream& operator <<(IOutputStream& out, TEscaper esc) {
    esc.Write(out);
    return out;
}


using TValidateFunc = bool(*)(TStringBuf);

template <TValidateFunc ValidateFunc = nullptr>
void ProcessLine(IInputStream& in, IOutputStream& out)
{
    static TString row;
    in.ReadLine(row);

    TStringBuf secret, rowId, offset, message;
    if (!TrySplit(row, ';', secret, rowId, offset, message))
        return;

    if constexpr (ValidateFunc != nullptr) {
        if (!ValidateFunc(message))
            return;
    }

    out << secret << ';' << rowId << ';' << offset << ";{"
        "\"pushclient_row_id\":\""  << rowId << "\","
        "\"level\":"                "20000,"
        "\"levelStr\":"             "\"INFO\","
        "\"loggerName\":"           "\"stdout\","
        "\"@version\":"             "1,"
        "\"threadName\":"           "\"qloud-init\","
        "\"@timestamp\":\""         << TInstant::Now().ToStringLocalUpToSeconds() << "\","
        "\"qloud_project\":"        "\"alice\","
        "\"qloud_application\":"    "\"uniproxy\","
        "\"qloud_environment\":"    "\"stable\","
        "\"qloud_component\":"      "\"uniproxy\","
        "\"qloud_instance\":"       "\"-\","
        "\"message\":\""            << TEscaper(message) << "\","
        "\"host\":\""               << hostname << "\""
        "}" << Endl;
}


int main(int argc, const char** argv)
{
    SetInterruptSignalsHandler(&SignalsHandler);

    auto func = ProcessLine<>;
    if (argc == 2 && TStringBuf("--validate-sessionlog") == argv[1]) {
        func = ProcessLine<&ValidateJsonInSessionlog>;
    }

    while (true) {
        try {
            func(Cin, Cout);
        } catch (...) { }
    }
    return 0;
}

