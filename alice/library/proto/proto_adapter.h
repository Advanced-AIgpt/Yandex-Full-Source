#pragma once

#include <util/generic/yexception.h>
#include <util/generic/deque.h>

#include <google/protobuf/struct.pb.h>

using GStruct = google::protobuf::Struct;
using GValue = google::protobuf::Value;

namespace NAlice {
    class TProtoAdapterTypeException: public yexception {};
    class TProtoAdapterValueException: public yexception {};

    class TProtoAdapter;
    typedef TDeque<TProtoAdapter> TProtoAdapterArray;

    /// This class is aimed to envelop google::protobuf::Struct proto-Message and mimic TJsonValue type
    class TProtoAdapter {
    public:
        typedef TProtoAdapterArray TArray;

        explicit TProtoAdapter(GStruct s, bool safeMode = true)
            : NoException_(safeMode)
        {
            State_ = s;
        }

        explicit TProtoAdapter(GValue v, bool safeMode = true)
            : NoException_(safeMode)
        {
            State_ = v;
        }

        bool Has(const TStringBuf& key) const;

        TProtoAdapter operator[](const TStringBuf& key) const;

        TString GetString() const;
        TString GetStringRobust() const;

        double GetDouble() const;

        long long GetInteger() const;
        unsigned long long GetUInteger() const;

        bool GetBoolean() const;

        TProtoAdapterArray GetArray() const;

        // service section
        bool IsExceptionSafeMode() const;
        void SetExceptionSafeMode(bool mode);

    private:
        std::variant<GStruct, GValue> State_;
        bool NoException_;
    };

}