#include "utils.h"

#include "keys.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <kernel/inflectorlib/phrase/simple/simple.h>

#include <library/cpp/langs/langs.h>

#include <util/charset/wide.h>
#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/system/env.h>
#include <util/system/types.h>

#include <thread>
#include <utility>

namespace NVideoCommon {

namespace {
const THashSet<TString> KEEP_ORIGINAL_GENRES = {"Дети"};
const THashSet<TString> SKIP_GENRES = {"Никакой"};

const TUtf16String NOM_PL = u"{g=nom,pl}";
const TUtf16String NOM_SG = u"{g=nom,sg}";

TMaybe<TDuration> DoParseDurationString(TStringBuf s) {
    TVector<ui32> nums;
    while (s) {
        ui32 num;
        if (TryFromString(s.NextTok(':'), num)) {
            nums.push_back(num);
        } else {
            LOG(ERR) << "Bad duration string format " << s << " (should be HH:MM:SS)" << Endl;
            return Nothing();
        }
    }

    if (nums.size() == 3) {
        return TDuration::Hours(nums[0]) + TDuration::Minutes(nums[1]) + TDuration::Seconds(nums[2]);
    }
    return Nothing();
}

NYdb::TValue ToSerialKeyStruct(TSerialKey key) {
    NYdb::TValueBuilder builder;

    builder.BeginStruct();
    builder.AddMember("ProviderName").String(key.ProviderName);
    builder.AddMember("SerialId").String(key.SerialId);
    builder.EndStruct();

    return builder.Build();
}

NYdb::TValue ToKeyStruct(TVideoKey key) {
    NYdb::TValueBuilder builder;

    builder.BeginStruct();
    builder.AddMember("ProviderName").String(key.ProviderName);
    builder.AddMember("Type").OptionalString(key.Type);

    switch (key.IdType) {
        case TVideoKey::EIdType::KINOPOISK_ID:
            builder.AddMember("KinopoiskId").String(key.Id);
            break;
        case TVideoKey::EIdType::HRID:
            builder.AddMember("HumanReadableId").String(key.Id);
            break;
        case TVideoKey::EIdType::ID:
            builder.AddMember("ProviderItemId").String(key.Id);
            break;
    }
    builder.EndStruct();

    return builder.Build();
}

class TAllSeasonsDescriptorHandle : public IAllSeasonsDescriptorHandle {
public:
    explicit TAllSeasonsDescriptorHandle(TVector<std::unique_ptr<ISeasonDescriptorHandle>>&& handles)
        : Handles(std::move(handles)) {
    }

    // IAllSeasonsDescriptorHandle overrides:
    TResult WaitAndParseResponse(TSerialDescriptor& serial) override {
        Y_ASSERT(serial.Seasons.size() == Handles.size());

        for (size_t i = 0; i < Handles.size(); ++i) {
            if (const auto error = Handles[i]->WaitAndParseResponse(serial.Seasons[i]))
                return error;
        }
        return {};
    }

private:
    TVector<std::unique_ptr<ISeasonDescriptorHandle>> Handles;
};

class TProviderInfoMaker : public IVideoItemHandle {
public:
    explicit TProviderInfoMaker(std::unique_ptr<IVideoItemHandle> handle)
        : Handle(std::move(handle)) {
        Y_ASSERT(Handle);
    }

    // IVideoItemHandle overrides:
    TResult WaitAndParseResponse(TVideoItem& item) override {
        Y_ASSERT(Handle);
        const auto error = Handle->WaitAndParseResponse(item);
        if (!error) {
            TLightVideoItem info;
            info->Assign(item.Scheme());
            AddProviderInfoIfNeeded(item.Scheme(), info.Scheme());
        }
        return error;
    }

private:
    std::unique_ptr<IVideoItemHandle> Handle;
};
} // namespace

float NormalizeRelevance(double relevance) {
    constexpr double OFFSET = 1e8;

    if (relevance >= OFFSET)
        relevance -= OFFSET;

    return static_cast<float>(relevance / OFFSET);
}

TMaybe<TDuration> ParseDurationString(TStringBuf s) {
    if (s.empty()) {
        return Nothing();
    }
    TMaybe<TDuration> d = DoParseDurationString(s);
    if (!d.Defined()) {
        LOG(ERR) << "Bad duration string format " << s << " (should be HH:MM:SS)" << Endl;
    }
    return d;
}

TString SingularizeGenre(const TString& originalGenre) {
    TStringBuilder genreResult;

    if (originalGenre.Contains(" ") || KEEP_ORIGINAL_GENRES.contains(originalGenre)) {
        genreResult << originalGenre;
    } else if (!SKIP_GENRES.contains(originalGenre)) {
        const TUtf16String unknownForm = TUtf16String::FromUtf8(originalGenre);
        if (unknownForm) {
            // Unknown form is plural usually, but sometimes it is singular, and inflector makes awkward
            // singularizations (plural -> singular) like fantastica -> fantastico
            // To avoid that, let's check a singular option too
            NInfl::TSimpleInflector inflector(NameByLanguage(LANG_RUS));
            const TUtf16String expectPluralToSingle = inflector.Inflect(unknownForm + NOM_PL, "nom,sg");
            const TUtf16String expectSingularToPlural = inflector.Inflect(unknownForm + NOM_SG, "nom,pl");

            if (expectSingularToPlural) { // Sometimes inflector can't make a plural form from a word if this word is a
                                          // plural form already
                const TUtf16String expectSingularToPluralToSingular =
                    inflector.Inflect(expectSingularToPlural + NOM_PL, "nom,sg");
                if (expectSingularToPluralToSingular == expectPluralToSingle) {
                    genreResult << expectPluralToSingle;
                } else {
                    genreResult << unknownForm;
                }
            } else {
                genreResult << expectPluralToSingle;
            }
        }
    }

    return genreResult;
}

bool CheckSeasonIndex(ui32 seasonIndex, ui32 seasonsCount, TStringBuf providerName) {
    if (seasonIndex < seasonsCount)
        return true;
    TStringBuilder msg;
    msg << providerName << " error: ";
    msg << "unexpected season index: " << seasonIndex << ", ";
    msg << "total number of seasons: " << seasonsCount;
    LOG(ERR) << msg << Endl;
    return false;
}

bool CheckEpisodeIndex(ui32 episodeIndex, ui32 episodesCount, TStringBuf providerName) {
    if (episodeIndex < episodesCount)
        return true;
    TStringBuilder msg;
    msg << providerName << " error: ";
    msg << "unexpected episode index: " << episodeIndex << ", ";
    msg << "total number of episodes: " << episodesCount;
    LOG(ERR) << msg << Endl;
    return false;
}

TString JoinStringArray(const NSc::TArray& array) {
    TStringBuilder b;
    for (const auto& field : array) {
        if (b) {
            b << TStringBuf(", ");
        }
        b << field.GetString();
    }
    return b;
}

TString GetEnvOrThrow(const TString& name) {
    auto value = GetEnv(name);
    if (value.empty())
        ythrow yexception() << "Env variable " << name << " must be set";
    return value;
}

TMaybe<EPlayError> ParseRejectionReason(TStringBuf reason) {
    static const THashMap<TString, EPlayError> MASTER_PLAYLIST_ERRORS = {
        {"PURCHASE_NOT_FOUND", EPlayError::PURCHASE_NOT_FOUND},
        {"PURCHASE_EXPIRED", EPlayError::PURCHASE_EXPIRED},
        {"SUBSCRIPTION_NOT_FOUND", EPlayError::SUBSCRIPTION_NOT_FOUND},
        {"GEO_CONSTRAINT_VIOLATION", EPlayError::GEO_CONSTRAINT_VIOLATION},
        {"LICENSES_NOT_FOUND", EPlayError::LICENSES_NOT_FOUND},
        {"SERVICE_CONSTRAINT_VIOLATION", EPlayError::SERVICE_CONSTRAINT_VIOLATION},
        {"SUPPORTED_STREAMS_NOT_FOUND", EPlayError::SUPPORTED_STREAMS_NOT_FOUND},
        {"UNEXPLAINABLE", EPlayError::UNEXPLAINABLE},
        {"PRODUCT_CONSTRAINT_VIOLATION", EPlayError::PRODUCT_CONSTRAINT_VIOLATION},
        {"STREAMS_NOT_FOUND", EPlayError::STREAMS_NOT_FOUND},
        {"MONETIZATION_MODEL_CONSTRAINT_VIOLATION", EPlayError::MONETIZATION_MODEL_CONSTRAINT_VIOLATION},
        {"AUTH_TOKEN_SIGNATURE_FAILED", EPlayError::AUTH_TOKEN_SIGNATURE_FAILED},
        {"INTERSECTION_BETWEEN_LICENSE_AND_STREAMS_NOT_FOUND",
         EPlayError::INTERSECTION_BETWEEN_LICENSE_AND_STREAMS_NOT_FOUND},
        {"UNAUTHORIZED", EPlayError::UNAUTHORIZED},
        {"VIDEOERROR", EPlayError::VIDEOERROR}};

    if (const auto* error = MASTER_PLAYLIST_ERRORS.FindPtr(reason))
        return *error;
    return Nothing();
}

void WaitForNextDownload(TMaybe<TTimePoint>& lastDownload) {
    if (lastDownload) {
        const auto goodTime = *lastDownload + std::chrono::seconds(1);
        do {
            const auto now = TClock::now();
            if (goodTime <= now)
                break;
            std::this_thread::sleep_for(goodTime - now);
        } while (true);
    }

    lastDownload = TClock::now();
}

void AddProviderInfoIfNeeded(TVideoItemScheme item, TLightVideoItemConstScheme info) {
    if (!item.HasProviderInfo()) {
        item.ProviderInfo().Add() = info;
        return;
    }

    for (const auto& existingInfo : item.ProviderInfo()) {
        if (existingInfo.ProviderName().Get() == info.ProviderName().Get() &&
            existingInfo.ProviderItemId().Get() == info.ProviderItemId().Get() &&
            existingInfo.TvShowSeasonId().Get() == info.TvShowSeasonId().Get() &&
            existingInfo.TvShowItemId().Get() == info.TvShowItemId().Get()) {
            return;
        }
    }

    item.ProviderInfo().Add() = info;
}

// Non-empty container is required.
NYdb::TValue KeysToList(const THashSet<TVideoKey>& keys) {
    Y_ENSURE(!keys.empty());

    NYdb::TValueBuilder builder;

    builder.BeginList();
    for (const auto& key : keys)
        builder.AddListItem(ToKeyStruct(key));
    builder.EndList();

    return builder.Build();
}

// Non-empty container is required.
NYdb::TValue SerialKeysToList(const THashSet<TSerialKey>& keys) {
    Y_ENSURE(!keys.empty());

    NYdb::TValueBuilder builder;

    builder.BeginList();
    for (const auto& key : keys)
        builder.AddListItem(ToSerialKeyStruct(key));
    builder.EndList();

    return builder.Build();
}

TVideoItem MergeItems(TVideoItemConstScheme base, TVideoItemConstScheme update) {
    TVideoItem result(*base->GetRawValue());
    result.Value().MergeUpdate(*update->GetRawValue());

    // This is the tricky case - |update| may rewrite |base|
    // provider info, we need to properly merge these lists.
    for (const auto& info : base->ProviderInfo())
        AddProviderInfoIfNeeded(result.Scheme(), info);

    return result;
}

// IContentInfoProvider --------------------------------------------------------
bool IContentInfoProvider::IsContentListAvailable() const {
    return false;
}

std::unique_ptr<IVideoItemListHandle>
    IContentInfoProvider::MakeContentListRequest(NHttpFetcher::IMultiRequest::TRef /* multiRequest */) {
    LOG(ERR) << "Please, call this only when IsContentListAvailable() is true" << Endl;
    return {};
}

std::unique_ptr<IVideoItemHandle>
IContentInfoProvider::MakeContentInfoRequest(TLightVideoItemConstScheme item,
                                             NHttpFetcher::IMultiRequest::TRef multiRequest) {
    auto handle = MakeContentInfoRequestImpl(item, multiRequest);
    if (!handle)
        return {};
    return std::make_unique<TProviderInfoMaker>(std::move(handle));
}

std::unique_ptr<IAllSeasonsDescriptorHandle>
IContentInfoProvider::MakeAllSeasonsDescriptorRequest(const TSerialDescriptor& serialDescr,
                                                      NHttpFetcher::IMultiRequest::TRef multiRequest) {
    TVector<std::unique_ptr<ISeasonDescriptorHandle>> handles;
    for (const auto& seasonDescr : serialDescr.Seasons)
        handles.emplace_back(MakeSeasonDescriptorRequest(serialDescr, seasonDescr, multiRequest));
    return std::make_unique<TAllSeasonsDescriptorHandle>(std::move(handles));
}

bool IContentInfoProvider::IsAuxInfoRequestFeasible(TLightVideoItemConstScheme /* item */) {
    return false;
}

std::unique_ptr<IVideoItemHandle>
    IContentInfoProvider::MakeAuxInfoRequest(TLightVideoItemConstScheme /* item */,
                                             NHttpFetcher::IMultiRequest::TRef /* multiRequest */) {
    LOG(ERR) << "Please, call this only when is IsAuxInfoRequestFeasible() true" << Endl;
    return {};
}

// -----------------------------------------------------------------------------

void Ser(const TVideoItem& item, bool isVoid, NVideoContent::NProtos::TVideoItemRowV5YT& row) {
    row.SetProviderName(TString{item->ProviderName().Get()});
    if (item->HasProviderItemId())
        row.SetProviderItemId(TString{item->ProviderItemId().Get()});
    if (item->HasHumanReadableId())
        row.SetHumanReadableId(TString{item->HumanReadableId().Get()});
    row.SetType(TString{item->Type().Get()});
    row.SetContent(item.Value().ToJson());
    row.SetIsVoid(isVoid);
    if (item->HasMiscIds() && item->MiscIds().HasKinopoisk())
        row.SetKinopoiskId(TString{*item->MiscIds().Kinopoisk()});
    if (item->HasUpdateAtUs())
        row.SetUpdateAtUS(item->UpdateAtUs());
}

bool Ser(const TVideoItem& item, NVideoContent::NProtos::TProviderUniqueVideoItemRow& row) {
    if (!item->HasProviderItemId())
        return false;
    row.SetProviderName(TString{item->ProviderName().Get()});
    row.SetProviderItemId(TString{item->ProviderItemId().Get()});
    if (item->HasHumanReadableId())
        row.SetHumanReadableId(TString{item->HumanReadableId().Get()});
    row.SetType(TString{item->Type().Get()});
    row.SetContent(item.Value().ToJson());
    if (item->HasMiscIds() && item->MiscIds().HasKinopoisk())
        row.SetKinopoiskId(TString{*item->MiscIds().Kinopoisk()});
    if (item->Type() == ToString(EItemType::TvShowEpisode))
        row.SetParentTvShowId(TString{*item->TvShowItemId()});
    return true;
}

void Ser(const TVideoItem& item, ui64 id, bool isVoid, NVideoContent::NProtos::TVideoItemRowV5YDb& row) {
    row.SetId(id);
    row.SetProviderName(TString{item->ProviderName().Get()});
    if (item->HasProviderItemId())
        row.SetProviderItemId(TString{item->ProviderItemId().Get()});
    if (item->HasHumanReadableId())
        row.SetHumanReadableId(TString{item->HumanReadableId().Get()});
    row.SetType(TString{item->Type().Get()});
    row.SetContent(item.Value().ToJson());
    row.SetIsVoid(isVoid);
    if (item->HasMiscIds() && item->MiscIds().HasKinopoisk())
        row.SetKinopoiskId(TString{*item->MiscIds().Kinopoisk()});
    if (item->HasUpdateAtUs())
        row.SetUpdateAtUS(item->UpdateAtUs());
}

bool Ser(const TVideoItem& item, ui64 id, NVideoContent::NProtos::TProviderItemIdIndexRow& row) {
    if (!item->HasProviderName() || !item->HasProviderItemId() || !item->HasType())
        return false;
    const auto piid = TString{*item->ProviderItemId()};
    if (piid.empty())
        return false;
    row.SetProviderName(TString{*item->ProviderName()});
    row.SetProviderItemId(piid);
    row.SetType(TString{*item->Type()});
    row.SetId(id);
    return true;
}

bool Ser(const TVideoItem& item, ui64 id, NVideoContent::NProtos::THumanReadableIdIndexRow& row) {
    if (!item->HasProviderName() || !item->HasHumanReadableId() || !item->HasType())
        return false;
    const auto hrid = TString{*item->HumanReadableId()};
    if (hrid.empty())
        return false;
    row.SetProviderName(TString{*item->ProviderName()});
    row.SetHumanReadableId(hrid);
    row.SetType(TString{*item->Type()});
    row.SetId(id);
    return true;
}

bool Ser(const TVideoItem& item, ui64 id, NVideoContent::NProtos::TKinopoiskIdIndexRow& row) {
    if (!item->HasProviderName() || !item->MiscIds()->HasKinopoisk() || !item->HasType())
        return false;
    const auto kpid = TString{*item->MiscIds()->Kinopoisk()};
    if (kpid.empty())
        return false;
    row.SetProviderName(TString{*item->ProviderName()});
    row.SetKinopoiskId(kpid);
    row.SetType(TString{*item->Type()});
    row.SetId(id);
    return true;
}

bool Des(const TString& content, TVideoItem& item) {
    TVideoItem videoItem;
    if (!NSc::TValue::FromJson(videoItem.Value(), content))
        return false;

    auto onError = [](TStringBuf path, TStringBuf error) {
        LOG(ERR) << "Failed to validate TVideoItem: " << path << ": " << error << Endl;
    };

    if (!videoItem.Scheme().Validate({} /* path */, false /* strict */, onError))
        return false;

    item = videoItem;
    return true;
}

void VideoItemRowV5YTToV5YDb(ui64 id, const NVideoContent::NProtos::TVideoItemRowV5YT& input,
                             NVideoContent::NProtos::TVideoItemRowV5YDb& output) {
    output.SetId(id);
    if (input.HasIsVoid())
        output.SetIsVoid(input.GetIsVoid());
    if (input.HasProviderName())
        output.SetProviderName(input.GetProviderName());
    if (input.HasProviderItemId())
        output.SetProviderItemId(input.GetProviderItemId());
    if (input.HasHumanReadableId())
        output.SetHumanReadableId(input.GetHumanReadableId());
    if (input.HasKinopoiskId())
        output.SetKinopoiskId(input.GetKinopoiskId());
    if (input.HasType())
        output.SetType(input.GetType());
    if (input.HasContent())
        output.SetContent(input.GetContent());
    if (input.HasUpdateAtUS())
        output.SetUpdateAtUS(input.GetUpdateAtUS());
}

} // namespace NVideoCommon

template <>
void Out<NVideoCommon::TError>(IOutputStream& os, const NVideoCommon::TError& error) {
    os << "TError [";
    os << "msg: " << error.Msg << ", ";
    os << "http code: " << error.Code << "]";
}
