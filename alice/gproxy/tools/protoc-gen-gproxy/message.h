#pragma once

#include <util/generic/list.h>

#include "entity.h"
#include "field.h"


namespace NGProxyTraits {


class TFieldAccessor : public TBaseAccessor<
    TFieldAccessor,
    ::google::protobuf::Descriptor,
    ::google::protobuf::FieldDescriptor,
    TField
> {
public:
    static inline int Count(const google::protobuf::Descriptor* p) {
        return p->field_count();
    }

    static inline const ::google::protobuf::FieldDescriptor* Get(const google::protobuf::Descriptor* p, int index) {
        return p->field(index);
    }
};


class TMessage : public TEntity<google::protobuf::Descriptor> {
public:
    using TEntity::TEntity;

    using ThisType = TEntity<google::protobuf::Descriptor>;

    using TIterator = ThisType::TBaseIterator<TFieldAccessor>;

    TIterator begin() const {
        return TIterator(this->Get());
    }

    TIterator end() const {
        return TIterator(this->Get(), true);
    }

    const TList<TField>& Fields() const {
        return Fields_;
    }

    TMessage& AddField(TField field) {
        Fields_.emplace_back(std::move(field));
        return *this;
    }

private:
    TList<TField> Fields_;
};

}   // namespace NGProxyTraits
