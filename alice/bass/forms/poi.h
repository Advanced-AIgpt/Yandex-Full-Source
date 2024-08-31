#pragma once

#include <alice/bass/forms/geo_resolver.h>
#include <alice/bass/forms/vins.h>
#include <alice/bass/forms/parallel_handler.h>

#include <library/cpp/xml/document/xml-document.h>
#include <library/cpp/neh/neh.h>

#include <search/idl/meta.pb.h>

#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/variant.h>
#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NBASS {

class TPoiFormHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    /** Try to handle a given context as FindPoi
     * If query is a user bookmark or a valid address
     */
    static IParallelHandler::TTryResult TryToHandle(TContext& ctx, TStringBuf query);

    /** Just a wrapper to simplify the answer.
     */
    static bool TryToHandleSimple(TContext& ctx, TStringBuf query) {
        const auto res = TryToHandle(ctx, query);
        const auto* tryResult = std::get_if<IParallelHandler::ETryResult>(&res);
        return !tryResult /* means error */ || *tryResult != IParallelHandler::ETryResult::NonSuitable;
    }

    static TResultValue SetAsResponse(TContext& ctx, TStringBuf what);
    static void Register(THandlersMap* handlers);

private:
    TResultValue DoFindPoi(TContext& ctx, TString searchText, TMaybe<TGeoPosition> searchPos, TStringBuf sortBy, TVector<TStringBuf>& businessFilters);
    TResultValue DoFindPoi(TContext& ctx, TStringBuf objectId);

    void AddPoiSuggests(TContext& ctx, const NSc::TValue& foundPoi);
    void PushSuggestBlock(TContext& ctx, const TStringBuf suggestType) const;
    void PushEventsBlock(TContext& ctx, const NSc::TValue& foundPoi) const;
    TResultValue PushDivCardBlock(TContext& ctx, const NSc::TValue& poiData) const;
    TResultValue PushGalleryDivCardBlock(TContext& ctx, const TStringBuf searchText, const NSc::TValue& poiData) const;
    TResultValue PushNewDivCardBlock(TContext& ctx, const NSc::TValue& poiData) const;
};
}
