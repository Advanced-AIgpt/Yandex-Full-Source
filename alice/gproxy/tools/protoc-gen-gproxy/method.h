#pragma once

#include <util/string/cast.h>
#include <util/string/printf.h>

#include <alice/gproxy/library/protos/annotations/graph.pb.h>

#include "alice/protos/extensions/extensions.pb.h"
#include "entity.h"
#include "message.h"


namespace NGProxyTraits {


class TMethod : public TEntity<google::protobuf::MethodDescriptor> {
public:
    using TEntity::TEntity;

    using ThisType = TEntity<google::protobuf::MethodDescriptor>;

    static constexpr const int64_t DefaultGraphTimeout = 2000;
    static constexpr const int64_t DefaultGraphRetries = 1;
    static constexpr const char*   DefaultGraphName = "gproxy";
    static constexpr const char*   DefaultHttpPath = "/gproxy_http";


    inline TMessage InputType() const {
        return Get()->input_type();
    }

    inline TMessage OutputType() const {
        return Get()->output_type();
    }

    inline TProtoStringType ApphostRequestName() const {
        return Sprintf("request_%s", NameLower().c_str());
    }

    inline TProtoStringType ApphostResponseName() const {
        return Sprintf("response_%s", NameLower().c_str());
    }

    inline TProtoStringType VerticalName() const {
        return "VOICE";
    }

    inline TProtoStringType SemanticFrameName() const {
        const ::google::protobuf::MethodOptions opts = Get()->options();
        auto value = opts.GetExtension(::gproxy::graph::semantic_frame_message_name);
        if (!value) {
            const google::protobuf::MessageOptions& paramOpts = Get()->input_type()->options();
            return paramOpts.GetExtension(NAlice::SemanticFrameName);
        }
        return value;
    }

    inline TProtoStringType GraphName() const {
        const ::google::protobuf::MethodOptions& opts = Get()->options();
        auto value = opts.GetExtension(::gproxy::graph::name);
        if (!value) {
            return DefaultGraphName;
        }
        return value;
    }

    inline TProtoStringType HttpPathName() const {
        const ::google::protobuf::MethodOptions& opts = Get()->options();
        auto value = opts.GetExtension(::gproxy::graph::http_path);
        if (!value) {
            return DefaultHttpPath;
        }
        return value;
    }

    inline int64_t GraphTimeout() const {
        const ::google::protobuf::MethodOptions& opts = Get()->options();
        auto value = opts.GetExtension(::gproxy::graph::timeout);
        if (!value) {
            return DefaultGraphTimeout;
        }
        return value;
    }

    inline int64_t GraphRetries() const {
        const ::google::protobuf::MethodOptions& opts = Get()->options();
        auto value = opts.GetExtension(::gproxy::graph::retries);
        if (!value) {
            return DefaultGraphRetries;
        }
        return value;
    }

    inline bool UseRawRequestResponse() const {
        const ::google::protobuf::MethodOptions& opts = Get()->options();
        auto value = opts.GetExtension(::gproxy::graph::raw_request);
        if (!value) {
            return false;
        }
        return value;
    }

    inline const std::vector<TString> GetApphostFlags() const {
        const ::google::protobuf::MethodOptions& opts = Get()->options();
        std::vector<TString> flags;

        for (int i = 0; i < opts.ExtensionSize(::gproxy::graph::apphost_flags); ++i) {
            flags.push_back(opts.GetExtension(::gproxy::graph::apphost_flags, i));
        }
        return flags;
    }

private:
};

}   // namespace NGProxyTraits
