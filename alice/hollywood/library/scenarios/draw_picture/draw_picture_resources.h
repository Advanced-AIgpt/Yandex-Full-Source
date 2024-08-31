#pragma once

#include <alice/hollywood/library/resources/resources.h>
#include <util/random/random.h>
#include <util/generic/maybe.h>
#include <util/generic/hash.h>

#include <alice/hollywood/library/scenarios/draw_picture/resources/proto/resources.pb.h>
#include <milab/lib/i2tclient/cpp/i2tclient.h>

namespace NAlice::NHollywood {

    class TDrawPictureResources final : public IResourceContainer {
    public:
        static constexpr TStringBuf ResourceFileName = "ganart_image_features.pb";

        struct TComment {
            TString Text;
            TMaybe<TString> Tts;

            inline const TString& GetTts() const {
                return Tts.Defined() ? *Tts : Text;
            }
        };

        typedef TVector<TComment> TCommentBucket;

        struct TSubstitute {
            size_t CommentBucketId;
            TVector<size_t> ImageIds;
        };

        void LoadFromPath(const TFsPath& dirPath) override;

        inline const TVector<TString>& GetSuggests() const {
            return Suggests;
        }

        inline const TVector<TString>& GetNluHints() const {
            return NluHints;
        }

        inline const THashMap<TString, size_t>& GetRequestToSubstituteId() const {
            return RequestToSubstituteId;
        }

        inline const TVector<TString>& GetUrls() const {
            return Urls;
        }

        inline const TVector<TCommentBucket>& GetCommentBuckets() const {
            return CommentBuckets;
        }

        inline size_t GetGenericCommentBucketId() const {
            return GenericCommentBucketId;
        }

        inline const TVector<TSubstitute>& GetSubstitutes() const {
            return Substitutes;
        }

        inline const TVector<NMilab::NI2t::TI2tVector>& GetFeatures() const {
            return Features;
        }

    private:
        TVector<NMilab::NI2t::TI2tVector> Features;
        TVector<TString> Urls;

        TVector<TCommentBucket> CommentBuckets;
        TVector<TSubstitute> Substitutes;
        THashMap<TString, size_t> RequestToSubstituteId;
        size_t GenericCommentBucketId;

        TVector<TString> Suggests;
        TVector<TString> NluHints;
    };

} // namespace NAlice::NHollywood
