#include "entity_to_proto.h"

namespace NBg::NAliceEntityCollector {

void ReadEntity(const NProto::TAliceEntity& src, NGranet::TEntity* dest) {
    Y_ENSURE(dest);
    dest->Interval.Begin = src.GetBegin();
    dest->Interval.End = src.GetEnd();
    dest->Type = src.GetType();
    dest->Value = src.GetValue();
    dest->Flags = src.GetFlags();
    dest->Source = src.GetSource();
    dest->LogProbability = src.GetLogProbability();
    dest->Quality = src.GetQuality();
}

void WriteEntity(const NGranet::TEntity& src, NProto::TAliceEntity* dest) {
    Y_ENSURE(dest);
    dest->SetBegin(src.Interval.Begin);
    dest->SetEnd(src.Interval.End);
    dest->SetType(src.Type);
    dest->SetValue(src.Value);
    dest->SetFlags(src.Flags);
    dest->SetSource(src.Source);
    dest->SetLogProbability(src.LogProbability);
    dest->SetQuality(src.Quality);
}

void ReadEntity(const NProto::TGranetEntity& src, NGranet::TEntity* dest) {
    Y_ENSURE(dest);
    dest->Interval.Begin = src.GetBegin();
    dest->Interval.End = src.GetEnd();
    dest->Type = src.GetType();
    dest->Value = src.GetValue();
    dest->Flags = src.GetFlags();
    dest->Source = src.GetSource();
    dest->LogProbability = src.GetLogProbability();
    dest->Quality = src.GetQuality();
}

void WriteEntity(const NGranet::TEntity& src, NProto::TGranetEntity* dest) {
    Y_ENSURE(dest);
    dest->SetBegin(src.Interval.Begin);
    dest->SetEnd(src.Interval.End);
    dest->SetType(src.Type);
    dest->SetValue(src.Value);
    dest->SetFlags(src.Flags);
    dest->SetSource(src.Source);
    dest->SetLogProbability(src.LogProbability);
    dest->SetQuality(src.Quality);
}

} // namespace NBg::NAliceEntityCollector
