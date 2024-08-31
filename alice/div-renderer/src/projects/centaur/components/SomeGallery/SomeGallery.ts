import { ContainerBlock, FixedSize, GalleryBlock, ImageBlock, TextBlock, WrapContentSize } from 'divcard2';
import { GalleryBlockProps } from '../../helpers/types';
import { colorBlueText, colorBlueTextOpacity50 } from '../../style/constants';
import { text28m } from '../../style/Text/Text';

export interface SomeGalleryItem {
    imageUrl: string;
    title: string;
    subtitle: string;
}

export interface SomeGalleryProps extends GalleryBlockProps {
    some: SomeGalleryItem[];
}
export const SomeGallery = ({ some, ...props }: SomeGalleryProps) => {
    return new GalleryBlock({
        ...props,
        item_spacing: 16,
        items: some.map(({ title, subtitle, imageUrl }) => {
            return new ContainerBlock({
                width: new FixedSize({ value: 284 }),
                height: new WrapContentSize(),
                items: [
                    new ImageBlock({
                        height: new FixedSize({ value: 284 }),
                        width: new FixedSize({ value: 284 }),
                        image_url: imageUrl,
                        border: {
                            corner_radius: 24,
                        },
                    }),
                    new ContainerBlock({
                        margins: { top: 20 },
                        items: [
                            new TextBlock({
                                ...text28m,
                                text: title,
                                text_color: colorBlueText,
                                max_lines: 2,
                            }),
                            new TextBlock({
                                ...text28m,
                                margins: {
                                    top: 4,
                                },
                                text: subtitle,
                                text_color: colorBlueTextOpacity50,
                                max_lines: 3,
                            }),
                        ],
                    }),
                ],
            });
        }),
    });
};
