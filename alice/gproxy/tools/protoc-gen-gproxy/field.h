#pragma once

#include "entity.h"


namespace NGProxyTraits {


class TField : public TEntity<google::protobuf::FieldDescriptor> {
public:
    using TEntity::TEntity;

    using ThisType = TEntity<google::protobuf::FieldDescriptor>;

private:
};

}   // namespace NGProxyTraits
