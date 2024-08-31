import { ContainerBlock, FixedSize, GalleryBlock, ImageBlock, SolidBackground, TextBlock } from 'divcard2';
import { compact } from 'lodash';
import { Avatar } from '../../../../../common/helpers/avatar';
import { NAlice } from '../../../../../protos';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { GalleryBlockProps } from '../../../helpers/types';
import { colorBlueText, colorBlueTextOpacity50, offsetFromEdgeOfScreen } from '../../../style/constants';
import { text28m } from '../../../style/Text/Text';
import { SectionTemplate } from './SearchRichCardSection';

interface Book {
    imageUrl: string;
    title: string;
    subtitle?: string;
}

interface BooksGalleryProps extends GalleryBlockProps {
    books: Book[];
}
const BooksGallery = ({ books, ...props }: BooksGalleryProps) => {
    return new GalleryBlock({
        ...props,
        item_spacing: 16,
        items: books.map(({ imageUrl, title, subtitle }) => {
            return new ContainerBlock({
                height: new FixedSize({ value: 216 }),
                width: new FixedSize({ value: 384 }),
                border: {
                    corner_radius: 24,
                },
                paddings: {
                    left: 24,
                    top: 24,
                    right: 24,
                    bottom: 24,
                },
                background: [new SolidBackground({ color: '#374352' })],
                orientation: 'horizontal',
                items: [
                    new ImageBlock({
                        image_url: imageUrl,
                        height: new FixedSize({ value: 168 }),
                        width: new FixedSize({ value: 128 }),
                        border: { corner_radius: 16 },
                    }),
                    new ContainerBlock({
                        margins: { left: 24 },
                        alignment_vertical: 'center',
                        items: compact([
                            new TextBlock({
                                ...text28m,
                                text: title,
                                text_alignment_horizontal: 'left',
                                text_color: colorBlueText,
                                max_lines: 3,
                            }),
                            subtitle ? new TextBlock({
                                ...text28m,
                                text: subtitle,
                                margins: { top: 4 },
                                text_alignment_horizontal: 'left',
                                text_color: colorBlueTextOpacity50,
                                max_lines: 1,
                            }) : null,
                        ]),
                    }),
                ],
            });
        }),
    });
};

type BooksSection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'Books'>;

const schema = {
    type: 'object',
    required: ['Books'],
    properties: {
        Books: {
            type: 'object',
            required: ['Books'],
            properties: {
                Books: {
                    type: 'array',
                    minItems: 1,
                    items: {
                        type: 'object',
                        required: ['Name', 'Image', 'ReleaseYear'],
                        properties: {
                            Name: { type: 'string' },
                            Image: {
                                type: 'object',
                                required: ['Url'],
                                properties: {
                                    Url: { type: 'string' },
                                },
                            },
                            ReleaseYear: { type: 'number' },
                        },
                    },
                },
            },
        },
    },
};

const dataAdapter = createDataAdapter(schema, (section: BooksSection) => {
    const data = section.Books?.Books ?? [];


    const books = data.map<Book>(({ Name, Image, ReleaseYear }) => {
        const imageUrl = (() => {
            if (!Image?.Url) {
                return ' ';
            }

            return Avatar.setImageSize({
                data: Image.Url,
                size: 'small',
                namespace: 'get-entity_search',
            });
        })();


        return ({
            title: Name ?? ' ',
            imageUrl,
            subtitle: (ReleaseYear ?? 0) > 0 ? `${ReleaseYear} г.` : undefined,
        });
    });

    return { books };
});

export const SearchRichCardBooksSection: SectionTemplate = (section, requestState) => {
    const { books } = dataAdapter(section, requestState);

    if (books.length === 0) {
        return undefined;
    }

    return BooksGallery({ books, paddings: { left: offsetFromEdgeOfScreen, right: offsetFromEdgeOfScreen } });
};
