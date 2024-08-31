#pragma once

#include <util/generic/list.h>

#include "entity.h"
#include "method.h"


namespace NGProxyTraits {


class TMethodAccessor : public TBaseAccessor<
    TMethodAccessor,
    ::google::protobuf::ServiceDescriptor,
    ::google::protobuf::MethodDescriptor,
    TMethod
> {
public:
    static inline int Count(const google::protobuf::ServiceDescriptor* p) {
        return p->method_count();
    }

    static inline const ::google::protobuf::MethodDescriptor* Get(const google::protobuf::ServiceDescriptor* p, int index) {
        return p->method(index);
    }
};


class TService : public TEntity<google::protobuf::ServiceDescriptor> {
public:
    using TEntity::TEntity;

    using ThisType = TEntity<google::protobuf::ServiceDescriptor>;

    using TIterator = ThisType::TBaseIterator<TMethodAccessor>;

    TIterator begin() const {
        return TIterator(this->Get());
    }

    TIterator end() const {
        return TIterator(this->Get(), true);
    }

    TService& AddMethod(TMethod method) {
        Methods_.emplace_back(std::move(method));
        return *this;
    }

    const TList<TMethod>& Methods() const {
        return Methods_;
    }

private:
    TList<TMethod> Methods_;
};

}   // namespace NGProxyTraits
