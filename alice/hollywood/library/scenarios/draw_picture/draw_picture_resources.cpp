#include "draw_picture_resources.h"
#include "draw_picture_impl.h"

#include <util/generic/mem_copy.h>
#include <util/stream/file.h>
#include <library/cpp/dot_product/dot_product.h>

#include <math.h>

namespace NAlice::NHollywood {

    void TDrawPictureResources::LoadFromPath(const TFsPath& dirPath) {
        const auto filename = dirPath / ResourceFileName;
        Y_ENSURE(filename.IsFile(), "Cannot find file " << filename);

        TFileInput fi(filename);
        TDrawPictureResourcesProto proto;
        Y_ENSURE(proto.ParseFromArcadiaStream(&fi), "Failed to parse protobuf from file " << filename);

        GenericCommentBucketId = proto.GetGenericBucketID();
        Suggests.assign(proto.GetSuggests().begin(), proto.GetSuggests().end());
        NluHints.assign(proto.GetNLUHints().begin(), proto.GetNLUHints().end());

        const auto numImages = proto.ImagesSize();
        Y_ENSURE(numImages >= TDrawPictureImpl::RankingSize);
        Y_ENSURE(Suggests.size() >= TDrawPictureImpl::SuggestsNum);
        Y_ENSURE(!NluHints.empty());

        Urls.reserve(numImages);
        Features.reserve(numImages);
        for (const auto& image : proto.GetImages()) {
            Urls.emplace_back(image.GetURL());
            auto& vec = Features.emplace_back();
            Y_ENSURE(image.FeaturesSize() == vec.size());
            MemCopy(vec.data(), image.GetFeatures().data(), vec.size());
            float l2norm = sqrt(L2NormSquared(vec.data(), vec.size()));
            for (size_t i = 0; i < vec.size(); ++i) {
                vec[i] /= l2norm;
            }
        }

        for (const auto& bucketProto : proto.GetCommentBuckets()) {
            auto& bucket = CommentBuckets.emplace_back();
            for (const auto& commentProto : bucketProto.GetComments()) {
                bucket.push_back({
                    commentProto.GetComment(),
                    commentProto.HasTTS() ? TMaybe<TString>(commentProto.GetTTS()) : Nothing(),
                });
            }
        }
        Y_ENSURE(GenericCommentBucketId < CommentBuckets.size());

        for (size_t i = 0; i < proto.SubstitutesSize(); ++i) {
            const auto& substProto = proto.GetSubstitutes(i);
            auto& subst = Substitutes.emplace_back();
            subst.CommentBucketId = substProto.GetCommentBucketID();
            Y_ENSURE(subst.CommentBucketId < CommentBuckets.size());
            for (const auto imId : substProto.GetImageIDs()) {
                Y_ENSURE(imId < numImages);
                subst.ImageIds.push_back(imId);
            }
            for (const auto& request : substProto.GetRequests()) {
                RequestToSubstituteId[request] = i;
            }
        }
    }

} // namespace NAlice::NHollywood
