#pragma once

#include <util/generic/list.h>
#include <util/generic/set.h>

#include "entity.h"
#include "service.h"


namespace NGProxyTraits {


class TServiceAccessor : public TBaseAccessor<
    TServiceAccessor,
    ::google::protobuf::FileDescriptor,
    ::google::protobuf::ServiceDescriptor,
    TService
> {
public:
    static inline int Count(const google::protobuf::FileDescriptor* p) {
        return p->service_count();
    }

    static inline const ::google::protobuf::ServiceDescriptor* Get(const google::protobuf::FileDescriptor* p, int index) {
        return p->service(index);
    }
};


class TFile : public TEntity<google::protobuf::FileDescriptor> {
public:
    using TEntity::TEntity;

    using ThisType = TEntity<google::protobuf::FileDescriptor>;

    using TIterator = ThisType::TBaseIterator<TServiceAccessor>;

    TIterator begin() const {
        return TIterator(Get());
    }

    TIterator end() const {
        return TIterator(Get(), true);
    }

    TFile& AddService(TService service) {
        ServicePbs_.insert(service.HeaderPath("grpc"));
        Services_.emplace_back(std::move(service));
        return *this;
    }

    TFile& AddMessage(TMessage msg) {
        MessagePbs_.insert(msg.HeaderPath());
        return *this;
    }

    const TList<TService>& Services() const {
        return Services_;
    }

    const TSet<TString>& MessagePbs() const {
        return MessagePbs_;
    }

    const TSet<TString>& ServicePbs() const {
        return ServicePbs_;
    }

private:
    TSet<TString>   MessagePbs_;
    TSet<TString>   ServicePbs_;
    TList<TService> Services_;
};

}   // namespace NGProxyTraits
