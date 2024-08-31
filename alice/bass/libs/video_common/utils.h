#pragma once

#include "defs.h"

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/source_request/handle.h>
#include <alice/bass/libs/video_common/keys.h>
#include <alice/bass/libs/video_content/protos/rows.pb.h>
#include <alice/bass/util/error.h>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/scheme/scheme.h>

#include <ydb/public/sdk/cpp/client/ydb_table/table.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/system/compiler.h>

#include <chrono>
#include <iterator>

namespace NVideoCommon {
template <typename TValue>
struct TIndexed {
    TIndexed() = default;

    template <typename... TArgs>
    TIndexed(size_t index, TArgs&&... args)
        : Index(index)
        , Value(std::forward<TArgs>(args)...) {
    }

    size_t Index = 0;
    TValue Value = {};
};

struct TError {
    explicit TError(TStringBuf msg)
        : Msg(msg) {
    }

    TError(TStringBuf msg, int code)
        : Msg(msg)
        , Code(code) {
    }

    TString Msg;
    int Code = HttpCodes::HTTP_OK;
};

template <typename... TArgs>
TError MakeError(TArgs&&... messagePieces) {
    auto errText = (TStringBuilder{} << ... << messagePieces);
    return TError{errText};
}

using TResult = TMaybe<TError>;

//TODO: Drop it
template <typename TRequest>
struct IContentRequestHandle {
    virtual ~IContentRequestHandle() = default;
    virtual TResult WaitAndParseResponse(TRequest& request) = 0;
};

//TODO: Drop it
template <class TResponse>
struct TDummyContentRequestHandle : public IContentRequestHandle<TResponse> {
    // IContentRequestHandle<TResponse> overrides:
    TResult WaitAndParseResponse(TResponse& /* response */) override {
        return {};
    }
};

template <typename TResponse>
class TMockContentRequestHandle : public IContentRequestHandle<TResponse> {
public:
    explicit TMockContentRequestHandle(typename TTypeTraits<TResponse>::TFuncParam response)
        : Response(response) {
    }

    // IContentRequestHandle<TResponse> overrides:
    TResult WaitAndParseResponse(TResponse& response) override {
        response = Response;
        return {};
    }

private:
    const TResponse Response;
};

template <typename TResponse>
class TErrorContentRequestHandle : public IContentRequestHandle<TResponse> {
public:
    explicit TErrorContentRequestHandle(const TResult& result)
        : Result(result) {
    }

    // IContentRequestHandle<TResponse> overrides:
    TResult WaitAndParseResponse(TResponse& /* response */) override {
        return Result;
    }

private:
    const TResult Result;
};

struct TVideoItemList {
    TVector<TVideoItem> Items;
    TMaybe<TInstant> Updated;
};

using IAllSeasonsDescriptorHandle = IContentRequestHandle<TSerialDescriptor>;
using ISeasonDescriptorHandle = IContentRequestHandle<TSeasonDescriptor>;
using ISerialDescriptorHandle = IContentRequestHandle<TSerialDescriptor>;
using IVideoItemHandle = IContentRequestHandle<TVideoItem>;
using IVideoItemListHandle = IContentRequestHandle<TVideoItemList>;

float NormalizeRelevance(double relevance);

TMaybe<TDuration> ParseDurationString(TStringBuf s);
TString SingularizeGenre(const TString& originalGenre);

bool CheckSeasonIndex(ui32 seasonIndex, ui32 seasonsCount, TStringBuf providerName);
bool CheckEpisodeIndex(ui32 episodeIndex, ui32 episodesCount, TStringBuf providerName);
void AddProviderInfoIfNeeded(TVideoItemScheme item, TLightVideoItemConstScheme info);

NYdb::TValue KeysToList(const THashSet<TVideoKey>& keys);
NYdb::TValue SerialKeysToList(const THashSet<TSerialKey>& keys);

// Merges |base| with |update|, assuming that they have consistent
// types, provider names and ids.
TVideoItem MergeItems(TVideoItemConstScheme base, TVideoItemConstScheme update);

TString JoinStringArray(const NSc::TArray& value);

TString GetEnvOrThrow(const TString& name);

TMaybe<EPlayError> ParseRejectionReason(TStringBuf reason);

using TClock = std::chrono::steady_clock;
using TTimePoint = TClock::time_point;

void WaitForNextDownload(TMaybe<TTimePoint>& lastDownload);

struct TSepByLineEmitter final {
    TSepByLineEmitter(IOutputStream& out, TStringBuf sep)
        : Out(out)
        , Sep(sep)
        , First(true) {
    }

    ~TSepByLineEmitter() {
        Out << Endl;
    }

    template <typename TValue>
    TSepByLineEmitter& operator<<(const TValue& value) {
        if (!First)
            Out << Sep;
        First = false;
        Out << value;
        return *this;
    }

    IOutputStream& Out;
    const TString Sep;
    bool First;
};

class ISourceRequestFactory {
public:
    virtual ~ISourceRequestFactory() = default;

    virtual NHttpFetcher::TRequestPtr Request(TStringBuf path) = 0;
    virtual NHttpFetcher::TRequestPtr AttachRequest(TStringBuf path,
                                                    NHttpFetcher::IMultiRequest::TRef multiRequest) = 0;
};

struct IContentInfoDelegate {
    virtual ~IContentInfoDelegate() = default;

    virtual bool PassesAgeRestriction(ui32 minAge, bool isPornoGenre) const = 0;
    virtual bool PassesAgeRestriction(const TVideoItemConstScheme& videoItem) const = 0;
};

class TDummyDelegate : public IContentInfoDelegate {
public:
    // IContentInfoDelegate overrides:
    bool PassesAgeRestriction(ui32 /* minAge */, bool /* isPornoGenre */) const override {
        return true;
    }

    bool PassesAgeRestriction(const TVideoItemConstScheme& /* videoItem */) const override {
        return true;
    }
};

class IContentInfoProvider {
public:
    enum class EPreferredSeasonDownloadMode { All, Individual };

    virtual ~IContentInfoProvider() = default;

    virtual bool IsContentListAvailable() const;

    virtual std::unique_ptr<IVideoItemListHandle> MakeContentListRequest(NHttpFetcher::IMultiRequest::TRef multiRequest);

    std::unique_ptr<IVideoItemHandle> MakeContentInfoRequest(TLightVideoItemConstScheme item,
                                                             NHttpFetcher::IMultiRequest::TRef multiRequest);

    virtual std::unique_ptr<ISerialDescriptorHandle>
    MakeSerialDescriptorRequest(TVideoItemConstScheme tvShowItem, NHttpFetcher::IMultiRequest::TRef multiRequest) = 0;

    virtual EPreferredSeasonDownloadMode GetPreferredSeasonDownloadMode() const = 0;

    virtual std::unique_ptr<IAllSeasonsDescriptorHandle>
    MakeAllSeasonsDescriptorRequest(const TSerialDescriptor& serialDescr,
                                    NHttpFetcher::IMultiRequest::TRef multiRequest);

    virtual std::unique_ptr<ISeasonDescriptorHandle>
    MakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr, const TSeasonDescriptor& seasonDescr,
                                NHttpFetcher::IMultiRequest::TRef multiRequest) = 0;

    virtual bool IsAuxInfoRequestFeasible(TLightVideoItemConstScheme item);
    virtual std::unique_ptr<IVideoItemHandle> MakeAuxInfoRequest(TLightVideoItemConstScheme item,
                                                                 NHttpFetcher::IMultiRequest::TRef multiRequest);

protected:
    virtual std::unique_ptr<IVideoItemHandle>
    MakeContentInfoRequestImpl(TLightVideoItemConstScheme item, NHttpFetcher::IMultiRequest::TRef multiRequest) = 0;
};

struct IContentInfoProvidersCache {
    virtual ~IContentInfoProvidersCache() = default;

    virtual IContentInfoProvider* GetProvider(TStringBuf name) = 0;

    class iterator {
    public:
        static iterator begin(IContentInfoProvidersCache* cache) {
            for (const TStringBuf* iter = std::begin(ALL_VIDEO_PROVIDERS); iter != std::end(ALL_VIDEO_PROVIDERS);
                 ++iter) {
                if (auto* value = cache->GetProvider(*iter))
                    return {cache, iter, value};
            }
            return end(cache);
        }

        static iterator end(IContentInfoProvidersCache* cache) {
            return {cache, std::end(ALL_VIDEO_PROVIDERS), nullptr};
        }

        bool operator==(const iterator& rhs) const {
            return Cache == rhs.Cache && KeyIterator == rhs.KeyIterator;
        }

        bool operator!=(const iterator& rhs) const {
            return !(*this == rhs);
        }

        IContentInfoProvider* operator*() {
            Y_ASSERT(!IsEnd());
            return Value;
        }

        iterator& operator++() {
            Y_ASSERT(!IsEnd());
            ++KeyIterator;

            while (!IsEnd()) {
                Value = Cache->GetProvider(*KeyIterator);
                if (Value)
                    return *this;
                ++KeyIterator;
            }

            Value = nullptr;
            return *this;
        }

        iterator operator++(int) {
            iterator result = *this;
            ++(*this);
            return result;
        }

        TString GetProviderName() const {
            Y_ASSERT(!IsEnd());
            return TString{*KeyIterator};
        }

    private:
        iterator(IContentInfoProvidersCache* cache, const TStringBuf* keyIter, IContentInfoProvider* value)
            : Cache(cache)
            , KeyIterator(keyIter)
            , Value(value) {
        }

        bool IsEnd() const {
            return KeyIterator == std::end(ALL_VIDEO_PROVIDERS);
        }

        IContentInfoProvidersCache* Cache = nullptr;
        const TStringBuf* KeyIterator = std::end(ALL_VIDEO_PROVIDERS);
        IContentInfoProvider* Value = nullptr;
    };

    iterator begin() {
        return iterator::begin(this);
    }

    iterator end() {
        return iterator::end(this);
    }
};

void Ser(const TVideoItem& item, bool isVoid, NVideoContent::NProtos::TVideoItemRowV5YT& row);
void Ser(const TVideoItem& item, ui64 id, bool isVoid, NVideoContent::NProtos::TVideoItemRowV5YDb& row);

[[nodiscard]] bool Ser(const TVideoItem& item, ui64 id, NVideoContent::NProtos::TProviderItemIdIndexRow& row);
[[nodiscard]] bool Ser(const TVideoItem& item, ui64 id, NVideoContent::NProtos::THumanReadableIdIndexRow& row);
[[nodiscard]] bool Ser(const TVideoItem& item, ui64 id, NVideoContent::NProtos::TKinopoiskIdIndexRow& row);
[[nodiscard]] bool Ser(const TVideoItem& item, NVideoContent::NProtos::TProviderUniqueVideoItemRow& row);

[[nodiscard]] bool Des(const TString& content, TVideoItem& item);

void VideoItemRowV5YTToV5YDb(ui64 id, const NVideoContent::NProtos::TVideoItemRowV5YT& input,
                             NVideoContent::NProtos::TVideoItemRowV5YDb& output);

} // namespace NVideoCommon
