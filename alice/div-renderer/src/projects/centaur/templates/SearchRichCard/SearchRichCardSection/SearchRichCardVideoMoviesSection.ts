import { ContainerBlock, FixedSize, GalleryBlock, ImageBlock, MatchParentSize, SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { NAlice } from '../../../../../protos';
import { colorBlueText, colorBlueTextOpacity50, colorWhite, offsetFromEdgeOfScreen } from '../../../style/constants';
import { text28m, title22m } from '../../../style/Text/Text';
import { SectionTemplate } from './SearchRichCardSection';
import { GalleryBlockProps } from '../../../helpers/types';
import { createDataAdapter } from '../../../helpers/createDataAdapter';

interface Movie {
    imageUrl: string;
    title: string;
    subtitle: string;
    rating: string;
    ratingColor: string;
}
interface MovieGalleryProps extends GalleryBlockProps {
    movies: Movie[];
}
const MovieGallery = ({ movies, ...props }: MovieGalleryProps) => {
    return new GalleryBlock({
        ...props,
        item_spacing: 16,
        items: movies.map(movie => {
            return new ContainerBlock({
                orientation: 'overlap',
                height: new FixedSize({ value: 310 }),
                width: new FixedSize({ value: 384 }),
                items: [
                    new ImageBlock({
                        image_url: movie.imageUrl,
                        width: new MatchParentSize(),
                        height: new FixedSize({ value: 216 }),
                        border: {
                            corner_radius: 24,
                        },
                        alignment_vertical: 'top',
                    }),
                    new ContainerBlock({
                        alignment_vertical: 'bottom',
                        items: [
                            new TextBlock({
                                ...text28m,
                                text_color: colorBlueText,
                                margins: { top: 18 },
                                text: movie.title,
                            }),
                            new TextBlock({
                                ...text28m,
                                text_color: colorBlueTextOpacity50,
                                margins: { top: 4 },
                                text: movie.subtitle,
                                max_lines: 1,
                            }),
                        ],
                    }),
                    new TextBlock({
                        ...title22m,
                        paddings: {
                            top: 8,
                            left: 12,
                            right: 12,
                            bottom: 8,
                        },
                        margins: {
                            top: 24,
                            left: 24,
                        },
                        border: {
                            corner_radius: 12,
                        },
                        width: new WrapContentSize(),
                        text_color: colorWhite,
                        background: [
                            new SolidBackground({ color: movie.ratingColor }),
                        ],
                        text: movie.rating,
                        alignment_horizontal: 'left',
                        alignment_vertical: 'top',
                    }),
                ],
            });
        }),
    });
};

type VideoMoviesSection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'VideoMovies'>;

const schema = {
    type: 'object',
    required: ['VideoMovies'],
    properties: {
        VideoMovies: {
            type: 'object',
            required: ['Videos'],
            properties: {
                Videos: {
                    type: 'array',
                    minItems: 1,
                    items: {
                        type: 'object',
                        required: ['Logo', 'Title'],
                        properties: {
                            Logo: {
                                type: 'object',
                                required: ['Url'],
                                properties: {
                                    Url: { type: 'string' },
                                },
                            },
                            Title: { type: 'string' },
                        },
                    },
                },
            },
        },
    },
};

const dataAdapter = createDataAdapter(schema, (section: VideoMoviesSection) => {
    const data = section.VideoMovies?.Videos ?? [];

    const movies = data.map<Movie>(({ Logo, Title, Rating, HintInfo, Subtitle }) => {
        // TODO: CENTAUR-1176: нет данных о рейтинге и фильме
        // assert(Rating, 'SearchRichCard => VideoMovies: в карточке нет поля Rating');
        // assert(HintInfo, 'SearchRichCard => VideoMovies: в карточке нет поля HintInfo');

        return {
            title: Title ?? ' ',
            imageUrl: Logo?.Url ?? ' ',
            // TODO: CENTAUR-1176: нет данных по фильму
            subtitle: HintInfo || Subtitle || '(нет данных)',
            // TODO: CENTAUR-1176: нет данных рейтинга
            rating: Rating || '9.2',
            // TODO: CENTAUR-1176: подставить мапу цветов рейтинга
            ratingColor: '#60AF4D',
        };
    });

    return { movies };
});

export const SearchRicCardVideoMoviesSection: SectionTemplate = (section, requestState) => {
    const { movies } = dataAdapter(section, requestState);

    if (movies.length === 0) {
        return undefined;
    }

    return MovieGallery({
        movies,
        margins: { top: 32 },
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
    });
};
