#include "differ.h"

#include "ttls.h"
#include "alice/wonderlogs/library/common/utils.h"
#include "alice/wonderlogs/library/yt/utils.h"

#include <alice/wonderlogs/protos/wonderlogs.pb.h>
#include <alice/wonderlogs/protos/wonderlogs_diff.pb.h>

#include <alice/library/field_differ/lib/field_differ.h>
#include <alice/library/json/json.h>

#include <library/cpp/json/json_prettifier.h>
#include <library/cpp/libgit2_wrapper/unidiff.h>
#include <library/cpp/yson/node/node_io.h>

#include <mapreduce/yt/library/operation_tracker/operation_tracker.h>

using namespace NAlice::NWonderlogs;

namespace {

using google::protobuf::Message;

const TVector<NYT::TNode::EType> COMPLEX_TYPES{NYT::TNode::List, NYT::TNode::Map};

TString ConcatPath(const TString& curPath, const TString& value) {
    return curPath + (curPath.empty() ? "" : ".") + value;
}

class TDiffReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& stableTable, const TString& testTable, const TString& diffTable)
            : StableTable(stableTable)
            , TestTable(testTable)
            , DiffTable(diffTable) {
        }
        NYT::TReduceOperationSpec AddToOperationSpec(NYT::TReduceOperationSpec&& operationSpec) {
            return operationSpec.AddInput<NYT::TNode>(StableTable)
                .AddInput<NYT::TNode>(TestTable)
                .AddOutput<NYT::TNode>(DiffTable);
        }

        enum EInIndices {
            Stable = 0,
            Test = 1,
        };

    private:
        const TString& StableTable;
        const TString& TestTable;
        const TString& DiffTable;
    };

    Y_SAVELOAD_JOB(DiffContext);
    TDiffReducer() = default;
    TDiffReducer(const ui32 diffContext)
        : DiffContext(diffContext) {
    }

    void Do(TReader* reader, TWriter* writer) override {
        NYT::TNode rowStable = NYT::TNode::CreateEntity();
        NYT::TNode rowTest = NYT::TNode::CreateEntity();
        for (auto& cursor : *reader) {
            switch (cursor.GetTableIndex()) {
                case TInputOutputTables::EInIndices::Stable: {
                    rowStable = cursor.GetRow();
                    break;
                }
                case TInputOutputTables::EInIndices::Test: {
                    rowTest = cursor.GetRow();
                    break;
                }
                default:
                    Y_FAIL("Unexpected table index in TDiffReducer");
            }
        }
        NYT::TNode stable = NYT::TNode::CreateEntity();
        if (!rowStable.IsNull()) {
            stable = NYT::NodeFromJsonString(rowStable["json"].AsString());
        }
        NYT::TNode test = NYT::TNode::CreateEntity();
        if (!rowTest.IsNull()) {
            test = NYT::NodeFromJsonString(rowTest["json"].AsString());
        }

        TString diff;
        TStringOutput out{diff};

        NLibgit2::UnifiedDiff(NYT::NodeToCanonicalYsonString(stable, NYson::EYsonFormat::Pretty) + '\n',
                          NYT::NodeToCanonicalYsonString(test, NYson::EYsonFormat::Pretty) + '\n', DiffContext, out,
                          /* colored= */ false);

        NYT::TNode row;
        if (!rowStable.IsNull() && rowStable.HasKey("uuid")) {
            row["uuid"] = rowStable["uuid"];
        } else {
            row["uuid"] = rowTest["uuid"];
        }
        if (!rowStable.IsNull() && rowStable.HasKey("message_id")) {
            row["message_id"] = rowStable["message_id"];
        } else {
            row["message_id"] = rowTest["message_id"];
        }
        row["diff"] = diff;
        row["stable"] = stable;
        if (row["stable"].IsUndefined()) {
            row["stable"] = NYT::TNode::CreateEntity();
        }
        row["test"] = test;
        if (row["test"].IsUndefined()) {
            row["test"] = NYT::TNode::CreateEntity();
        }

        writer->AddRow(row);
    }

private:
    ui32 DiffContext = 0;
};

class TDiffDeletedFieldsReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& stableTable, const TString& testTable, const TString& diffTable)
            : StableTable(stableTable)
            , TestTable(testTable)
            , DiffTable(diffTable) {
        }
        NYT::TReduceOperationSpec AddToOperationSpec(NYT::TReduceOperationSpec&& operationSpec) {
            return operationSpec.AddInput<NYT::TNode>(StableTable)
                .AddInput<NYT::TNode>(TestTable)
                .AddOutput<NYT::TNode>(DiffTable);
        }

        enum EInIndices {
            Stable = 0,
            Test = 1,
        };

    private:
        const TString& StableTable;
        const TString& TestTable;
        const TString& DiffTable;
    };

    void Do(TReader* reader, TWriter* writer) override {
        NYT::TNode row;
        row["stable"] = NYT::TNode::CreateEntity();
        row["test"] = NYT::TNode::CreateEntity();
        for (auto& cursor : *reader) {
            switch (cursor.GetTableIndex()) {
                case TInputOutputTables::EInIndices::Stable: {
                    row["megamind_request_id"] = cursor.GetRow()["megamind_request_id"];
                    row["stable"] = NYT::NodeFromJsonString(cursor.GetRow()["json"].AsString());
                    break;
                }
                case TInputOutputTables::EInIndices::Test: {
                    row["megamind_request_id"] = cursor.GetRow()["megamind_request_id"];
                    row["test"] = NYT::NodeFromJsonString(cursor.GetRow()["json"].AsString());
                    break;
                }
                default:
                    Y_FAIL("Unexpected table index in TDiffDeletedFieldsReducer");
            }
        }

        TVector<TString> changedFields;
        NImpl::GetChangedFields(row["stable"], row["test"], /* path= */ "", changedFields);
        row["changed_fields"] = NYT::TNode::CreateList();
        for (const auto& changedField : changedFields) {
            row["changed_fields"].Add(changedField);
        }

        writer->AddRow(row);
    }
};

class TDiffWonderlogs : public NYT::IReducer<NYT::TTableReader<Message>, NYT::TTableWriter<TWonderlogsDiff>> {
public:
    class TInputOutputTables {
    public:
        TInputOutputTables(const TString& stableTable, const TString& testTable, const TString& diffTable)
            : StableTable(stableTable)
            , TestTable(testTable)
            , DiffTable(diffTable) {
        }
        NYT::TReduceOperationSpec AddToOperationSpec(NYT::TReduceOperationSpec&& operationSpec) {
            return operationSpec.AddInput<TWonderlog>(StableTable)
                .AddInput<TWonderlog>(TestTable)
                .AddOutput<TWonderlogsDiff>(DiffTable);
        }

        enum EInIndices {
            Stable = 0,
            Test = 1,
        };

    private:
        const TString& StableTable;
        const TString& TestTable;
        const TString& DiffTable;
    };

    void Do(TReader* reader, TWriter* writer) override {
        TWonderlogsDiff diff;
        TWonderlog stable;
        TWonderlog test;
        for (auto& cursor : *reader) {
            switch (cursor.GetTableIndex()) {
                case TInputOutputTables::EInIndices::Stable: {
                    stable = cursor.GetRow<TWonderlog>();
                    diff.SetMegamindRequestId(stable.GetMegamindRequestId());
                    diff.SetStableWonderlog(NAlice::JsonStringFromProto(stable));
                    break;
                }
                case TInputOutputTables::EInIndices::Test: {
                    test = cursor.GetRow<TWonderlog>();
                    diff.SetMegamindRequestId(test.GetMegamindRequestId());
                    diff.SetTestWonderlog(NAlice::JsonStringFromProto(test));
                    break;
                }
                default:
                    Y_FAIL("Unexpected table index in TDiffWonderlogs");
            }
        }
        *diff.MutableDiff() = FieldDiffer.FindDiffs(stable, test);
        writer->AddRow(diff);
    }

private:
    NAlice::TFieldDiffer FieldDiffer;
};

REGISTER_REDUCER(TDiffReducer)
REGISTER_REDUCER(TDiffDeletedFieldsReducer)
REGISTER_REDUCER(TDiffWonderlogs)

} // namespace

void NAlice::NWonderlogs::MakeDiff(NYT::IClientPtr client, const TString& tmpDirectory, const TString& stableTable,
                                   const TString& testTable, const TString& outputTable, ui32 diffContext) {
    const auto stableTmpTableSorted = CreateRandomTable(client, tmpDirectory, "stable-sorted");
    const auto testTmpTableSorted = CreateRandomTable(client, tmpDirectory, "test-sorted");

    CreateTable(client, outputTable, TInstant::Now() + TEN_DAYS_TTL);

    {
        NYT::TOperationTracker tracker;
        for (const auto& [notSorted, sorted] :
             {std::tie(stableTable, stableTmpTableSorted), std::tie(testTable, testTmpTableSorted)}) {
            tracker.AddOperation(client->Sort(
                NYT::TSortOperationSpec{}.AddInput(notSorted).Output(sorted).SortBy({"uuid", "message_id"}),
                NYT::TOperationOptions{}.Wait(false)));
        }

        tracker.WaitAllCompleted();
    }

    client->Reduce(TDiffReducer::TInputOutputTables{stableTmpTableSorted, testTmpTableSorted, outputTable}
                       .AddToOperationSpec(NYT::TReduceOperationSpec{})
                       .ReduceBy({"uuid", "message_id"}),
                   new TDiffReducer(diffContext), NYT::TOperationOptions{}.InferOutputSchema(true));
}

void NAlice::NWonderlogs::MakeChangedFieldsDiff(NYT::IClientPtr client, const TString& tmpDirectory,
                                                const TString& stableTable, const TString& testTable,
                                                const TString& outputTable) {
    const auto stableTmpTableSorted = CreateRandomTable(client, tmpDirectory, "stable-sorted");
    const auto testTmpTableSorted = CreateRandomTable(client, tmpDirectory, "test-sorted");

    CreateTable(client, outputTable, /* expirationDate= */ {});

    {
        NYT::TOperationTracker tracker;
        for (const auto& [notSorted, sorted] :
             {std::tie(stableTable, stableTmpTableSorted), std::tie(testTable, testTmpTableSorted)}) {
            tracker.AddOperation(client->Sort(
                NYT::TSortOperationSpec{}.AddInput(notSorted).Output(sorted).SortBy({"megamind_request_id"}),
                NYT::TOperationOptions{}.Wait(false)));
        }

        tracker.WaitAllCompleted();
    }

    client->Reduce(TDiffDeletedFieldsReducer::TInputOutputTables{stableTmpTableSorted, testTmpTableSorted, outputTable}
                       .AddToOperationSpec(NYT::TReduceOperationSpec{})
                       .ReduceBy({"megamind_request_id"}),
                   new TDiffDeletedFieldsReducer, NYT::TOperationOptions{}.InferOutputSchema(true));
}

void NAlice::NWonderlogs::MakeWonderlogsDiff(NYT::IClientPtr client, const TString& tmpDirectory,
                                             const TString& stableTable, const TString& testTable,
                                             const TString& outputTable) {
    const auto stableTmpTableSorted = CreateRandomTable(client, tmpDirectory, "stable-wonderlogs-sorted");
    const auto testTmpTableSorted = CreateRandomTable(client, tmpDirectory, "test-wonderlogs-sorted");

    CreateTable(client, outputTable, TInstant::Now() + TEN_DAYS_TTL);

    {
        NYT::TOperationTracker tracker;
        for (const auto& [notSorted, sorted] :
             {std::tie(stableTable, stableTmpTableSorted), std::tie(testTable, testTmpTableSorted)}) {
            tracker.AddOperation(client->Sort(
                NYT::TSortOperationSpec{}.AddInput(notSorted).Output(sorted).SortBy("_megamind_request_id"),
                NYT::TOperationOptions{}.Wait(false)));
        }

        tracker.WaitAllCompleted();
    }

    client->Reduce(TDiffWonderlogs::TInputOutputTables{stableTmpTableSorted, testTmpTableSorted, outputTable}
                       .AddToOperationSpec(NYT::TReduceOperationSpec{})
                       .ReduceBy("_megamind_request_id"),
                   new TDiffWonderlogs, NYT::TOperationOptions{}.InferOutputSchema(true));
}

void NAlice::NWonderlogs::NImpl::GetChangedFields(const NYT::TNode& stable, const NYT::TNode& test,
                                                  const TString& path, TVector<TString>& changedFileds) {
    if (stable.GetType() != test.GetType()) {
        changedFileds.push_back(path);
        return;
    }

    switch (stable.GetType()) {
        case NYT::TNode::Undefined:
        case NYT::TNode::String:
        case NYT::TNode::Int64:
        case NYT::TNode::Uint64:
        case NYT::TNode::Double:
        case NYT::TNode::Bool:
        case NYT::TNode::Null:
            break;
        case NYT::TNode::List:
            if (ContainsComplexNode(stable) || ContainsComplexNode(test)) {
                if (stable.AsList().size() != test.AsList().size()) {
                    changedFileds.push_back(path);
                    return;
                }
                for (size_t i = 0; i < stable.AsList().size(); ++i) {
                    GetChangedFields(stable.AsList()[i], test.AsList()[i], ConcatPath(path, ToString(i)),
                                     changedFileds);
                }
            }
            break;
        case NYT::TNode::Map:
            for (const auto& [key, value] : stable.AsMap()) {
                const auto newPath = ConcatPath(path, key);
                if (!test.AsMap().contains(key)) {
                    changedFileds.push_back(newPath);
                    return;
                }
                const auto& stableValue = value;
                const auto& testValue = test.AsMap().at(key);
                GetChangedFields(stableValue, testValue, newPath, changedFileds);
            }
            break;
    }
}

bool NAlice::NWonderlogs::NImpl::ContainsComplexNode(const NYT::TNode& node) {
    switch (node.GetType()) {
        case NYT::TNode::Undefined:
        case NYT::TNode::String:
        case NYT::TNode::Int64:
        case NYT::TNode::Uint64:
        case NYT::TNode::Double:
        case NYT::TNode::Bool:
        case NYT::TNode::Null:
            return false;
        case NYT::TNode::List:
            for (const auto& value : node.AsList()) {
                if (Find(COMPLEX_TYPES, value.GetType()) != COMPLEX_TYPES.end()) {
                    return true;
                }
            }
            break;
        case NYT::TNode::Map:
            return true;
    }
    return false;
}
