#pragma once
#include "occurrence_searcher.h"
#include <library/cpp/getopt/last_getopt.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/generic/ptr.h>


namespace NAlice {
namespace NNlu {

class TInput {
public:
    explicit TInput(const TString& filename)
        : Input(filename == "-" ? nullptr : MakeHolder<TFileInput>(filename))
    { }

    IInputStream& Get() {
        if (Input) {
            return *Input;
        } else {
            return Cin;
        }
    }

private:
    THolder<IInputStream> Input;
};


template<typename TValue>
class TAutomatonBuilderApp {
public:
    TAutomatonBuilderApp(int argc, const char* argv[])
        : Input(argc > 1 ? argv[1] : "-")
    { }

    int Run() {
        TOccurrenceSearcherDataBuilder<TValue> builder;
        TString name;
        TValue value;
        for (TString line; Input.Get().ReadLine(line); ) {
            ParseLine(line, &name, &value);
            builder.Add(name, value);
        }
        const auto searcherData = builder.Build();
        Cout.Write(searcherData.Data(), searcherData.Length());
        return 0;
    }

private:
    void ParseLine(const TStringBuf& line, TString* name, TValue* value) const {
        Y_ASSERT(name);
        Y_ASSERT(value);
        TStringBuf left;
        TStringBuf right;
        line.Split('\t', left, right);
        (*name) = left;
        (*value) = DeserializeValue(right);
    }

    virtual TValue DeserializeValue(const TStringBuf& string) const = 0;

private:
    TInput Input;
};

} // namespace NNlu
} // namespace NAlice
