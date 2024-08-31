#pragma once

#include "defs.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/system/types.h>

namespace NBASS {
namespace NVideo {

class TJsonSlot {
public:
    explicit TJsonSlot(const TSlot* slot)
        : Slot(slot)
    {
    }

    virtual ~TJsonSlot() = default;

    const NSc::TValue& GetRawValue() const {
        return Slot ? Slot->Value : Default<NSc::TValue>();
    }

    TStringBuf GetSourceText() const {
        return Slot ? Slot->SourceText.GetString() : TStringBuf();
    }

    TStringBuf GetType() const {
        return Slot ? Slot->Type : TStringBuf();
    }

    bool Empty() const {
        return IsSlotEmpty(Slot);
    }

protected:
    const TSlot* Slot;
};

template <typename EEnum>
class TEnumSlot : public TJsonSlot {
public:
    explicit TEnumSlot(const TSlot* slot, TMaybe<EEnum> defaultValue = Nothing())
        : TJsonSlot(slot)
    {
        if (!Parse(slot)) {
            Value = defaultValue;
        }
    }

    bool Parse(const TSlot* slot) {
        if (IsSlotEmpty(slot)) {
            return false;
        }
        EEnum value;
        TStringBuf str = slot->Value.GetString();
        if (TryFromString(str, value)) {
            Value = value;
            return true;
        }
        LOG(WARNING) << "Ignored unexpected slot " << slot->Name << " value " << str << Endl;
        return false;

    }

    TMaybe<EEnum> GetMaybe() const {
        return Value;
    }
    EEnum GetEnumValue() const {
        return Value.GetRef();
    }

    bool Defined() const {
        return Value.Defined();
    }

    bool operator==(EEnum value) const {
        return Value.Defined() && *Value == value;
    }
    bool operator!=(EEnum value) const {
        return !operator==(value);
    }

private:
    TMaybe<EEnum> Value;
};

class TBoolSlot : public TJsonSlot {
public:
    explicit TBoolSlot(const TSlot* slot)
        : TJsonSlot(slot)
    {
        if (!IsSlotEmpty(slot)) {
            Value = slot->Value.GetBool();
        }
    }

    bool GetBoolValue() const {
        return Value.GetOrElse(false);
    }

    bool Defined() const {
        return Value.Defined();
    }

private:
    TMaybe<bool> Value;
};

class TIntSlot : public TJsonSlot {
public:
    explicit TIntSlot(const TSlot* slot)
        : TJsonSlot(slot)
    {
        if (!IsSlotEmpty(slot) && slot->Value.IsIntNumber()) {
            Value = slot->Value.GetIntNumber();
        }
    }

    TMaybe<i64> Get() const {
        return Value;
    }
    i64 GetIntNumber() const {
        return Value.GetRef();
    }

    bool Defined() const {
        return Value.Defined();
    }

private:
    TMaybe<i64> Value;
};

class TStringSlot : public TJsonSlot {
public:
    TStringSlot(const TSlot* slot)
        : TJsonSlot(slot)
    {
        if (!IsSlotEmpty(slot)) {
            Value = slot->Value.GetString();
        }
    }

    TStringBuf GetString() const {
        return Value;
    }

    bool Defined() const {
        return !IsSlotEmpty(Slot) && Slot->Value.IsString();
    }

protected:
    TStringBuf Value;
};

class TProviderSlot : protected TStringSlot {
public:
    TProviderSlot(const TSlot* slot, bool amediatekaToKinopoisk, bool iviToKinopoisk)
        : TStringSlot(slot) {
        if (GetType() != NAlice::NVideoCommon::SLOT_PROVIDER_TYPE)
            return;

        if ((amediatekaToKinopoisk && Value == NVideoCommon::PROVIDER_AMEDIATEKA) ||
            (iviToKinopoisk && Value == NVideoCommon::PROVIDER_IVI))
        {
            Value = NVideoCommon::PROVIDER_KINOPOISK;
        }
    }

    using TJsonSlot::GetType;
    using TStringSlot::Defined;
    using TStringSlot::GetString;
};

class TSerialSlot : public TJsonSlot {
public:
    bool Defined() const {
        return Value.Defined();
    }

    TMaybe<TSerialIndex> GetMaybe() const {
        return Value;
    }

    TSerialIndex GetOrElse(TSerialIndex defValue) const {
        return Defined() ? *Value : defValue;
    }

    TSerialIndex GetSerialIndex() const {
        return *Value;
    }

protected:
    TSerialSlot(const TSlot* slot, TStringBuf specialType)
        : TJsonSlot(slot)
        , Value(ParseFromSlot(slot, specialType))
    {
    }

private:
    static TMaybe<TSerialIndex> ParseFromSlot(const TSlot* slot, TStringBuf specialType) {
        if (IsSlotEmpty(slot)) {
            return Nothing();
        }
        if (slot->Type == TStringBuf("num")) {
            const i64 value = slot->Value.GetIntNumber();
            if (value < 1) {
                LOG(ERR) << "Ignored unexpected slot value of type 'num': " << value << Endl;
                return Nothing();
            }
            return NVideo::SerialIndexFromNumber(value);
        }
        if (slot->Type == specialType) {
            NVideo::ESpecialSerialNumber value;
            if (TryFromString(slot->Value.GetString(), value)) {
                return NVideo::TSerialIndex(value);
            }
            LOG(WARNING) << "Ignored unexpected slot value of type " << specialType << ": " << slot->Value.GetString()
                         << Endl;
            return Nothing();
        }
        LOG(WARNING) << "Ignored unexpected slot type: " << slot->Type << Endl;
        return Nothing();
    }

private:
    TMaybe<TSerialIndex> Value;
};

struct TSeasonSlot : public TSerialSlot {
    explicit TSeasonSlot(const TSlot* slot)
        : TSerialSlot(slot, TStringBuf("video_season") /* type */) {
    }
};

struct TEpisodeSlot : public TSerialSlot {
    explicit TEpisodeSlot(const TSlot* slot)
        : TSerialSlot(slot, TStringBuf("video_episode") /* type */) {
    }
};

struct TReleaseYearSlot : public TJsonSlot {
    TReleaseYearSlot(const TSlot* slot, const TContext& ctx);

    bool Defined() const {
        return ExactYear || DecadeStartYear || RelativeYear || YearsRange;
    }

    TMaybe<ui32> ExactYear;
    TMaybe<ui32> DecadeStartYear;
    TMaybe<i32> RelativeYear;
    TMaybe<std::pair<ui32, ui32>> YearsRange;
};

struct TVideoLanguageSlot : public TStringSlot {
    explicit TVideoLanguageSlot(const TSlot* slot)
        : TStringSlot(slot) {
    }
};

class TVideoSlots {
private:
    explicit TVideoSlots(const TContext& ctx);

public:
    static TMaybe<TVideoSlots> TryGetFromContext(TContext& ctx);

    TString BuildSearchQueryForWeb() const;
    TString BuildSearchQueryForInternetVideos() const;

    /**
     * TODO: Сейчас при добавлении нового слота нужно не забыть поправить BuildSearchQuery*(), если требуется.
     *       Хорошо бы сделать механизм перечисления слотов, который не дал бы забыть об этом.
     */
    TStringSlot SearchText;
    TStringSlot SearchTextRaw;
    TStringSlot SearchTextText;

    // Original content provider slot, as-is.
    TProviderSlot OriginalProvider;
    TStringSlot ProviderText;

    // Possible cases:
    // * when original provider slot is a valid provider, this field is
    //   the same as the original provider value.
    // * when original provider is empty, this field is empty too.
    // * otherwise, this field is set to the youtube provider and ProviderWasChanged is set.
    TString FixedProvider;

    bool ProviderWasChanged = false;

    TEnumSlot<EContentType> ContentType;
    TStringSlot ContentTypeRaw;
    TStringSlot ContentTypeText;
    TEnumSlot<EVideoGenre> VideoGenre;
    TStringSlot VideoGenreText;

    TStringSlot Country;
    TStringSlot CountryText;
    TReleaseYearSlot ReleaseDate;
    TStringSlot ReleaseDateText;

    TEnumSlot<EVideoAction> Action;
    TEnumSlot<ENewVideo> NewVideo;
    TEnumSlot<ETopVideo> TopVideo;
    TEnumSlot<EFreeVideo> FreeVideo;
    TEnumSlot<NAlice::NVideoCommon::EProviderOverrideType> ProviderOverride;

    TSeasonSlot SeasonIndex;
    TEpisodeSlot EpisodeIndex;

    TIntSlot GalleryNumber;

    TEnumSlot<ESelectionAction> SelectionAction;

    TEnumSlot<EScreenName> ScreenName;

    TBoolSlot ForbidAutoSelect;

    TVideoLanguageSlot AudioLanguage;
    TVideoLanguageSlot SubtitleLanguage;

    bool ShouldUseContentTypeForProviderSearch = true;
    bool ShouldUseTextOnlySlotsInVideoSearch = false;
    bool ShouldUseRawSlotsInVideoSearch = false;

    TBoolSlot SilentResponse;
};


} // namespace NVideo
} // namespace NBASS
