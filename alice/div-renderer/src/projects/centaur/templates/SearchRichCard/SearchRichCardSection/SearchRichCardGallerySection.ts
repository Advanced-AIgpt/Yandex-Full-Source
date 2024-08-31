import { FixedSize, GalleryBlock, ImageBlock, MatchParentSize, WrapContentSize } from 'divcard2';
import { NAlice } from '../../../../../protos';
import { SectionTemplate } from './SearchRichCardSection';
import { GalleryBlockProps } from '../../../helpers/types';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { compact } from 'lodash';
import { Avatar } from '../../../../../common/helpers/avatar';

interface ImagesGalleryProps extends GalleryBlockProps {
    images: string[];
}
const ImagesGallery = ({ images, ...props }: ImagesGalleryProps) => {
    return new GalleryBlock({
        ...props,
        orientation: 'horizontal',
        item_spacing: 0,
        items: images.map(
            image_url =>
                new ImageBlock({
                    image_url,
                    height: new MatchParentSize(),
                    width: new WrapContentSize(),
                }),
        ),
    });
};

type GallerySection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'Gallery'>;

const schema = {
    type: 'object',
    properties: {
        Gallery: {
            type: 'object',
            properties: {
                Images: {
                    type: 'array',
                    items: {
                        type: 'object',
                        properties: {
                            UrlAvatar: {
                                type: 'object',
                                properties: {
                                    Url: { type: 'string' },
                                    Width: { type: 'integer' },
                                    Height: { type: 'integer' },
                                },
                                required: ['Url', 'Width', 'Height'],
                            },
                        },
                        required: ['UrlAvatar'],
                    },
                    minItems: 1,
                },
            },
            required: ['Images'],
        },
    },
    required: ['Gallery'],
};

const dataAdapter = createDataAdapter(
    schema,
    (section: GallerySection) => {
        const data = section.Gallery?.Images ?? [];

        const images = compact(data.map(({ UrlAvatar }) => {
            const imageUrl = (() => {
                if (!UrlAvatar?.Url) {
                    return ' ';
                }

                return Avatar.setImageSize({
                    // тут могут приходить картинки как из неймспейсов "get-altay" и "i"
                    // поэтому значение неймспейса не передаем, а вычислям внутри
                    data: UrlAvatar.Url,
                    size: 'largeRectangle',
                });
            })();

            return imageUrl;
        }));

        return { images };
    },
);

export const SearchRichCardGallerySection: SectionTemplate = (section, requestState) => {
    const { images } = dataAdapter(section, requestState);

    if (images.length === 0) {
        return undefined;
    }

    return ImagesGallery({
        images,
        height: new FixedSize({ value: 344 }),
    });
};
