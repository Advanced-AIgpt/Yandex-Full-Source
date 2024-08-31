#pragma once

#include <alice/nlu/granet/lib/sample/entity.h>
#include <search/begemot/rules/alice/request/proto/common.pb.h>
#include <search/begemot/rules/granet/proto/granet.pb.h>

namespace NBg::NAliceEntityCollector {

void ReadEntity(const NProto::TAliceEntity& src, NGranet::TEntity* dest);
void WriteEntity(const NGranet::TEntity& src, NProto::TAliceEntity* dest);

void ReadEntity(const NProto::TGranetEntity& src, NGranet::TEntity* dest);
void WriteEntity(const NGranet::TEntity& src, NProto::TGranetEntity* dest);

} // namespace NBg::NAliceEntityCollector
