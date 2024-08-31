#include <util/stream/file.h>
#include <util/datetime/base.h>
#include <util/generic/set.h>
#include <limits>
#include <library/cpp/getopt/last_getopt.h>


// These headers will be printed
TSet<TString> NEEDED_HEADERS({
    "X-UPRX-AUTH-TOKEN",
    "X-UPRX-UUID"
});


// Split TStringBuf onto multiple chunks
template <typename ...T>
bool TrySplit(TStringBuf row, char delim, TStringBuf& left, TStringBuf& right, T& ...others) {
    if (row.TrySplit(delim, left, right)) {
        if constexpr (sizeof...(others) > 0)
            return TrySplit(right, delim, right, others...);
        return true;
    }
    return false;
}


void PrintHeaders(IOutputStream& out, TStringBuf src)
{
    static const TStringBuf HEADER_BEGIN_MARK("<::");
    static const TStringBuf HEADER_END_MARK("::>");

    size_t pos = 0;
    while (pos < src.length()) {
        const size_t begin = src.find(HEADER_BEGIN_MARK, pos);
        if (begin == TStringBuf::npos)
            break;

        pos = begin + HEADER_BEGIN_MARK.length();

        const size_t end = src.find(HEADER_END_MARK, pos);
        if (end == TStringBuf::npos)
            break;

        TStringBuf name, value;
        if (TStringBuf(src.data() + pos, src.data() + end).TrySplit(':', name, value)) {
            if (NEEDED_HEADERS.contains(name))
                out << name << '=' << value << '\t';
        }

        pos = end + HEADER_END_MARK.length();
    }
}


void PrintHeadersFromProcessingTree(IOutputStream& out, TStringBuf record)
{
    static const TStringBuf HEADERS_BLOCK_BEGIN_MARK("log_headers");

    const size_t pos = record.find(HEADERS_BLOCK_BEGIN_MARK);
    if (pos == TStringBuf::npos)
        return;

    PrintHeaders(out, record.SubStr(pos + HEADERS_BLOCK_BEGIN_MARK.length()));
}


struct TConfig
{
    TString Path;
    TString Uri;
    TString Host;
    time_t BeginTs = std::numeric_limits<time_t>::min();
    time_t EndTs = std::numeric_limits<time_t>::max();
    unsigned MaxCount = 1000;

    static TConfig ParseFromArgs(int argc, const char** argv) {
        TConfig config;

        NLastGetopt::TOpts opts;
        opts.AddLongOption("path", "file for parsing")
            .Required().RequiredArgument().StoreResult(&config.Path);
        opts.AddLongOption("uri", "filter out requests which URIs don't start with this")
            .RequiredArgument().StoreResult(&config.Uri);
        opts.AddLongOption("host", "filter out requests which 'Host' headers aren't equal to this")
            .RequiredArgument().StoreResult(&config.Host);
        opts.AddLongOption("begin-ts", "filter out requests that made before this UTC timestamp")
            .RequiredArgument().StoreResult(&config.BeginTs);
        opts.AddLongOption("end-ts", "filter out requests that made after this UTC timestamp")
            .RequiredArgument().StoreResult(&config.EndTs);
        opts.AddLongOption("max", "limit amount of records").RequiredArgument().StoreResult(&config.MaxCount);

        NLastGetopt::TOptsParseResult(&opts, argc, argv);

        return config;
    }
};


inline bool GetAddrAndPort(TStringBuf endpoint, TStringBuf& addr, TStringBuf& port)
{
    if (!endpoint.TryRSplit(':', addr, port))
        return false;
    addr.SkipPrefix("[");
    addr.ChopSuffix("]");
    return true;
}


int main(int argc, const char** argv)
{
    const TConfig config = TConfig::ParseFromArgs(argc, argv);

    TFileInput fin(config.Path);

    TString line;
    TStringBuf srcEndpoint, start_time, request, duration, referer, host, proc;

    unsigned count = 0;
    while (count < config.MaxCount) {
        if (fin.ReadLine(line) == 0)
            break;
        if (!TrySplit(line, '\t', srcEndpoint, start_time, request, duration, referer, host, proc))
            continue;

        if (!config.Host.empty() && host != config.Host)
            continue; // host doesn't match

        TStringBuf method, uri, version;
        if (!TrySplit(request, ' ', method, uri, version))
            continue;
        if (!config.Uri.empty() && !uri.StartsWith(config.Uri))
            continue;  // URI doesn't match

        time_t ts;
        if (!ParseISO8601DateTime(start_time.data(), start_time.length(), ts))
            continue;
        if (ts < config.BeginTs || ts > config.EndTs)
            continue;  // time doesn't match

        TStringBuf addr, port;
        if (!GetAddrAndPort(srcEndpoint, addr, port))
            continue;

        ++count;
        Cout << "host=" << host << "\turi=" << uri << "\tts=" << ts << "\tip=" << addr << "\tport=" << port << '\t';
        PrintHeadersFromProcessingTree(Cout, proc);
        Cout << Endl;
    }

    return 0;
}
