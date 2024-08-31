import { ContainerBlock, GalleryBlock, ImageBlock, MatchParentSize, TextBlock, WrapContentSize } from 'divcard2';
import { FixedSize } from 'divcard2/lib/generated/FixedSize';
import { compact } from 'lodash';
import { GalleryBlockProps } from '../../helpers/types';
import { colorBlueTextOpacity50 } from '../../style/constants';
import { text28m } from '../../style/Text/Text';
import { YaMapsRating } from '../YaMapsRating/YaMapsRating';

export interface ICompanyGalleryItem {
    imageUrl: string;
    title: string;
    rating?: number;
    subtitle: string;
}
interface ICompanyGalleryProps extends GalleryBlockProps {
    companies: ICompanyGalleryItem[];
}
export const CompanyGallery = ({ companies, ...props }: ICompanyGalleryProps) => {
    return new GalleryBlock({
        ...props,
        item_spacing: 16,
        items: companies.map(({ imageUrl, title, rating, subtitle }) => {
            return new ContainerBlock({
                width: new FixedSize({ value: 384 }),
                items: [
                    new ImageBlock({
                        image_url: imageUrl,
                        width: new MatchParentSize(),
                        height: new FixedSize({ value: 216 }),
                        border: {
                            corner_radius: 24,
                        },
                    }),
                    new ContainerBlock({
                        orientation: 'horizontal',
                        margins: {
                            top: 18,
                        },
                        width: new WrapContentSize(),
                        items: compact([
                            new TextBlock({
                                ...text28m,
                                text: title,
                                width: new MatchParentSize(),
                                max_lines: 1,
                            }),
                            rating ? YaMapsRating({ rating, margins: { left: 10 } }) : null,
                        ]),
                    }),
                    new TextBlock({
                        ...text28m,
                        text: subtitle,
                        margins: {
                            top: 6,
                        },
                        max_lines: 2,
                        text_color: colorBlueTextOpacity50,
                    }),
                ],
            });
        }),
    });
};
