#include "util.h"

#include <util/generic/size_literals.h>

using namespace NYT;

namespace NAliceYT {

namespace {

TUserJobSpec DefaultUserJobSpec(ui64 memoryLimitMB) {
    TUserJobSpec spec;
    spec.MemoryLimit(memoryLimitMB * 1024 * 1024);
    return spec;
}

void SetRowWeightLimit(TNode& spec, i64 limit) {
    spec["job_io"]["table_writer"]["max_row_weight"] = limit;
    spec["map_job_io"]["table_writer"]["max_row_weight"] = limit;
    spec["sort_job_io"]["table_writer"]["max_row_weight"] = limit;
    spec["reduce_job_io"]["table_writer"]["max_row_weight"] = limit;
    spec["merge_job_io"]["table_writer"]["max_row_weight"] = limit;
    spec["partition_job_io"]["table_writer"]["max_row_weight"] = limit;
}

} // namespace

TMapOperationSpec DefaultMapOperationSpec(ui64 memoryLimitMB) {
    TMapOperationSpec spec;
    spec.MapperSpec(DefaultUserJobSpec(memoryLimitMB));
    return spec;
}

TReduceOperationSpec DefaultReduceOperationSpec(ui64 memoryLimitMB) {
    TReduceOperationSpec spec;
    spec.ReducerSpec(DefaultUserJobSpec(memoryLimitMB));
    return spec;
}

TMapReduceOperationSpec DefaultMapReduceOperationSpec(ui64 memoryLimitMB) {
    TMapReduceOperationSpec spec;
    spec.MapperSpec(DefaultUserJobSpec(memoryLimitMB));
    spec.ReducerSpec(DefaultUserJobSpec(memoryLimitMB));
    spec.ReduceCombinerSpec(DefaultUserJobSpec(memoryLimitMB));
    return spec;
}

TOperationOptions DefaultOperationOptions() {
    TNode spec;
    SetRowWeightLimit(spec, 128_MB);
    TOperationOptions options;
    options.Spec(spec);
    options.MountSandboxInTmpfs(true);
    return options;
}

} // namespace NAliceYT

