#pragma once

#include "special_location.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/util/error.h>

namespace NBASS {

class TRequestedGeo {
public:
    /** These two vars are sugar for readablity in some constructors of this class */
    static constexpr bool NotFailIfNoSlot = false;
    static constexpr bool FailIfNoSlot = true;

public:
    TRequestedGeo(IGlobalContext& globalCtx);
    TRequestedGeo(const TRequestedGeo& geo);
    TRequestedGeo(TRequestedGeo&& geo);

    /* Updates object by new geoid but saves 'Same' field.
     * Used if incorrect geo object with same name was found first.
     */
    TRequestedGeo(IGlobalContext& globalCtx, NGeobase::TId geoId, bool same);

    explicit TRequestedGeo(IGlobalContext& globalCtx, NGeobase::TId geoId);

    TRequestedGeo& operator=(const TRequestedGeo& geo);

    /** Construscts geo from slot where.
     * Currently it supports only the following types:
     * "string" - a text location such as 'в москве' - requesting a geometasearch
     * "special_location" init geo from user location
     * @param[in] ctx is a current context
     * @param[in] whereSlot is a slot from which geo is going to be initialized
     * @param[in] failIfNoSlot specifies if you need to initalized geo from user location in case there is problem with get location from slot
     * @param[in] geoType list of geo types, separated by comma (default value: "geo,biz")
     */
    TRequestedGeo(TContext& ctx, const TContext::TSlot* whereSlot, bool failIfNoSlot = NotFailIfNoSlot, const TStringBuf geoType = TStringBuf("geo,biz"));
    TRequestedGeo(TContext& ctx, TStringBuf slotName, bool failIfNoSlot = NotFailIfNoSlot, const TStringBuf geoType = TStringBuf("geo,biz"));

    /** Re-initialize object with new GeoID only if geoid != internal geoid!
     * IsSame will be set to false
     * SpecialLocation will be the same (since we don't want to loose information about requested slot)
     * Error will reflect success of this process
     */
    void ConvertTo(NGeobase::TId geoId);

    /** Re-initialize obeject by going up in geo tree until
     * the given node type is found.
     * If current geo has the same type as requested, reamain object the same and return (BEWARE) true (even if there has been an Error)
     * If there are no parents with given type return false and remain node the same!
     * If parent is found then set 'Same' to false, update geo info and return true;
     * @param[in] type is a type of node we would like to became
     * @return true if the given type (node) found, otherwise false
     */
    bool ConvertTo(NGeobase::ERegionType type);

    /* Accessors */
    NGeobase::TRegion GetRegion() const;
    TResultValue GetError() const;
    NGeobase::TId GetId() const;
    bool IsSame() const;

    bool HasError() const;
    bool IsValidId() const;

    /** Return location type parsed from the slot (only if slot type is special not string).
     * If object has been created from geoid it returns TSpecialLocation::Enum::ERROR
     */
    TSpecialLocation GetSpecialLocation() const { return SpecialLocation; }

    /* Returns geo-type from  GeoBase */
    NGeobase::ERegionType GetGeoType() const;

    /** Returns parent region with given geobase type.
     * If no such parent, returns NGeobase::UNKNOWN_REGION
     */
    NGeobase::TId GetParentIdByType(NGeobase::ERegionType type) const;

    /** Returns parent with type TOWN or CITY.
     * !!! If no such parent, returns GeoId of the object !!!
     */
    NGeobase::TId GetParentCityId() const;

    /** Gets infinitive and prepositional from from GeoBase
     * @param[out] name — infinitive form
     * @param[out] namePrepcase — prepositional form with prepositon
     */
    void GetNames(TStringBuf lang, TString* name, TString* namePrepcase) const;

    /** Returns appropriate TLD for region
     * default is "ru"
     */
    TStringBuf ToTld() const;

    /** Returns a name of regions timezone
     * It can be empty for regions with several timezones
     */
    TString GetTimeZone() const;

    /** Create resolved slot and special attention block in case geo is not the same.
     * @param[out] ctx is used to get/create slot
     * @param[in] slotName is a name for newly created slot
     * @return result if success or not
     */
    TResultValue CreateResolvedMeta(TContext& ctx, TStringBuf slotName) const {
        return CreateResolvedMeta(ctx, slotName, true);
    }

    /** Create resolved slot and special attention block in case geo is not the same with additional check with compareGeo.
     * @param[out] ctx is used to get/create slot
     * @param[in] slotName is a name for newly created slot
     * @return result if success or not
     */
    TResultValue CreateResolvedMeta(TContext& ctx, TStringBuf slotName, NGeobase::TId compareGeo) const {
        return CreateResolvedMeta(ctx, slotName, GetId() == compareGeo);
    }

    void AddAllCaseForms(const TContext& ctx, NSc::TValue* geo, bool wantObsolete = false) const;
    void AddAllCaseFormsWithFallbackLanguage(const TContext& ctx, NSc::TValue* geo, bool wantObsolete = false) const;

private:
    TResultValue ParseAsQuery(TContext& ctx, const TContext::TSlot& slot, NGeobase::TId& geoId,
                              bool& same, const TStringBuf geoType = TStringBuf("geo,biz")) const;
    TResultValue ParseAsSpecialLocation(TContext& ctx, const TContext::TSlot& slot, NGeobase::TId& geoId,
                                        TSpecialLocation& specialLocation) const;
    TResultValue ParseAsId(const TContext::TSlot& slot, NGeobase::TId& geoId) const;
    TResultValue ParseAsSysGeo(const TContext::TSlot& slot, NGeobase::TId& geoId) const;
    TResultValue ParseAsGeoAddr(const TContext::TSlot& slot, NGeobase::TId& geoId) const;

    void InitGeoPtr(NGeobase::TId geoId);

    TResultValue CreateResolvedMeta(TContext& ctx, TStringBuf slotName, bool isSame) const;

private:
    TMaybe<NGeobase::TRegion> Region;
    TResultValue Error;
    bool Same;
    TSpecialLocation SpecialLocation;
    IGlobalContext& GlobalCtx;
};

} // namespace NBASS
