#pragma once

#include <mapreduce/yt/interface/client.h>

namespace NAlice::NWonderlogs {

namespace NImpl {

bool ContainsComplexNode(const NYT::TNode& node);
void GetChangedFields(const NYT::TNode& stable, const NYT::TNode& test, const TString& path,
                      TVector<TString>& changedFileds);

} // namespace NImpl

void MakeDiff(NYT::IClientPtr client, const TString& tmpDirectory, const TString& stableTable,
              const TString& testTable, const TString& outputTable, ui32 diffContext);

void MakeChangedFieldsDiff(NYT::IClientPtr client, const TString& tmpDirectory, const TString& stableTable,
                           const TString& testTable, const TString& outputTable);

void MakeWonderlogsDiff(NYT::IClientPtr client, const TString& tmpDirectory, const TString& stableTable,
                        const TString& testTable, const TString& outputTable);

} // namespace NAlice::NWonderlogs
