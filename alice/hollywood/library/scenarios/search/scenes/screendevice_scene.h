#pragma once

#include <alice/hollywood/library/framework/framework.h>

#include <alice/hollywood/library/scenarios/search/proto/search.pb.h>

#include <alice/library/search_result_parser/search_result_parser.h>

#include <alice/protos/data/scenario/search/richcard.pb.h>

namespace NAlice::NHollywoodFw::NSearch {

class TSearchScreenDeviceScene : public TScene<NHollywood::TSearchEmptyProto> {
public:
    TSearchScreenDeviceScene(const TScenario* owner);
    TRetMain Main(const NHollywood::TSearchEmptyProto&, const TRunRequest& request, TStorage&, const TSource&) const override;
    TRetResponse Render(const NHollywood::TSearchRenderProto& renderArgs, TRender& render) const;

private:
    TRetMain ReturnOldOrUnsupported(const TRunRequest& request, const TStringBuf details) const;
};

} // NAlice::NHollywoodFw::NSearch

