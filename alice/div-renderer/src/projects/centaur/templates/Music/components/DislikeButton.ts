import { FixedSize, ImageBlock } from 'divcard2';
import { getStaticS3Asset } from '../../../helpers/assets';

interface ILikeButtonProps {
    isDisliked: boolean;
    size?: number;
    options?: Omit<ConstructorParameters<typeof ImageBlock>[0], 'image_url'>;
}

export default function DislikeButton({ isDisliked, size = 48, options }: ILikeButtonProps) {
    return new ImageBlock({
        ...options,
        width: new FixedSize({ value: size }),
        height: new FixedSize({ value: size }),
        alpha: isDisliked ? 1 : 0.5,
        image_url: getStaticS3Asset( 'music/like_dislike.png'),
    });
}
