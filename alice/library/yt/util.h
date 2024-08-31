#pragma once

#include <mapreduce/yt/interface/client.h>

namespace NAliceYT {

NYT::TMapOperationSpec DefaultMapOperationSpec(ui64 memoryLimitMB);
NYT::TReduceOperationSpec DefaultReduceOperationSpec(ui64 memoryLimitMB);
NYT::TMapReduceOperationSpec DefaultMapReduceOperationSpec(ui64 memoryLimitMB);
NYT::TOperationOptions DefaultOperationOptions();

template <typename TSchema>
void CreateTableWithSchema(NYT::IClientBasePtr client, const NYT::TYPath& to, bool force) {
    if (client->Exists(to) && !force) {
        return;
    }

    client->Remove(to, NYT::TRemoveOptions().Force(true));

    auto options = NYT::TCreateOptions()
        .Attributes(
            NYT::TNode()
                ("optimize_for", "scan")
                ("schema", NYT::CreateTableSchema<TSchema>().ToNode())
                ("compression_codec", "brotli_9")
                ("erasure_codec", "lrc_12_2_2")
            )
        .IgnoreExisting(true)
        .Recursive(true);
    client->Create(to, NYT::NT_TABLE, options);
}

/// Calls |fn| for each node that matches to |type| and lives in the map-node at |root|.
template <typename TFn>
void ForEachInMapNode(NYT::ICypressClient& client, const NYT::TYPath& root, NYT::ENodeType type, TFn&& fn) {
    static const TString YT_ATTRIBUTE_TYPE = "type";

    NYT::TAttributeFilter filter;
    filter.AddAttribute(YT_ATTRIBUTE_TYPE);

    NYT::TListOptions options;
    options.AttributeFilter(filter);

    auto nodes = client.List(root, options);

    for (const auto& node : nodes) {
        if (node.GetAttributes()[YT_ATTRIBUTE_TYPE] != ToString(type))
            continue;
        if (!node.IsString())
            continue;
        fn(node.AsString());
    }
}

/// Dummy reducer, just keeps a single item from a group of equivalent items.
template<typename T>
class TUniqueReducer : public NYT::IReducer<NYT::TTableReader<T>, NYT::TTableWriter<T>> {
public:
    void Do(NYT::TTableReader<T>* reader, NYT::TTableWriter<T>* writer) override {
        if (reader->IsValid()) {
            writer->AddRow(reader->GetRow());
        }
    }
};

inline bool IsRealReqId(TStringBuf reqId) {
    return reqId != TStringBuf("d4fa807b-b5cc-49d7-8a82-8b037dfedff8") &&
        !reqId.StartsWith(TStringBuf("ffffffff-ffff-ffff")) &&
        !reqId.StartsWith(TStringBuf("dddddddd-dddd-dddd"));
}

} // namespace NAliceYT
