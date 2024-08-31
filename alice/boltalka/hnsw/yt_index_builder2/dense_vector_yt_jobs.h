#pragma once

#include <alice/boltalka/hnsw/yt_index_builder2/dense_vector_index_builder.h>

#include <library/cpp/hnsw/index/dense_vector_distance.h>

#include <util/system/types.h>

#define REGISTER_HNSW_JOBS(Type, TDistance) \
    using T##Type##TDistance##Traits = NYtHnsw::TDenseDistanceTraits<Type, NHnsw::TDistance<Type>>; \
    REGISTER_HNSW_YT_DENSE_VECTOR_BUILD_JOBS(T##Type##TDistance##Traits, NYtHnsw::TYtDenseVectorStorage<Type>);

#define REGISTER_HNSW_JOBS_WITH_TYPE(Type) \
    REGISTER_HNSW_JOBS(Type, TL1Distance); \
    REGISTER_HNSW_JOBS(Type, TL2SqrDistance); \
    REGISTER_HNSW_JOBS(Type, TDotProduct);

#define REGISTER_COMMON_HNSW_JOBS() \
    REGISTER_HNSW_JOBS_WITH_TYPE(i8); \
    REGISTER_HNSW_JOBS_WITH_TYPE(i32); \
    REGISTER_HNSW_JOBS_WITH_TYPE(float);
