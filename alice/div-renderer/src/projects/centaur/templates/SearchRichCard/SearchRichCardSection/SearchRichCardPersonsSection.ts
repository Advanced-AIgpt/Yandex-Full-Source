import { ContainerBlock, FixedSize, GalleryBlock, IDivAction, ImageBlock, MatchParentSize, SolidBackground, TextBlock } from 'divcard2';
import { NAlice } from '../../../../../protos';
import { colorBlueText, offsetFromEdgeOfScreen } from '../../../style/constants';
import { text28m } from '../../../style/Text/Text';
import { SectionTemplate } from './SearchRichCardSection';
import { GalleryBlockProps } from '../../../helpers/types';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { Avatar } from '../../../../../common/helpers/avatar';
import { createLocalTemplate } from '../../../../../common/helpers/createTemplate';
import { getGoToOOTextAction } from '../SearchRichCard.tools';
import { IRequestState } from '../../../../../common/types/common';
import { directivesAction } from '../../../../../common/actions';
import { createShowViewClientAction } from '../../../actions/client';
import { SearchRichCardSkeletonTemplate } from '../SearchRichCardSkeleton';
import { TopLevelCard } from '../../../helpers/helpers';
import { ExpFlags } from '../../../expFlags';
import { compact } from 'lodash';

const PersonsGalleryItemTemplate = createLocalTemplate<IPersonGalleryItem>(props => {
    const { actions, imageUrl, title } = props;

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
        actions,
        items: [
            new ImageBlock({
                image_url: imageUrl,
                height: new FixedSize({ value: 168 }),
                width: new FixedSize({ value: 128 }),
                border: { corner_radius: 16 },
            }),
            new TextBlock({
                text: title,
                margins: { left: 24 },
                height: new MatchParentSize(),
                text_alignment_horizontal: 'left',
                text_alignment_vertical: 'center',
                ...text28m,
                text_color: colorBlueText,
                max_lines: 3,
            }),
        ],
    });
});

interface IPersonGalleryItem {
    imageUrl: string;
    title: string;
    actions?: IDivAction[];
}
interface PersonGalleryProps extends GalleryBlockProps {
    persons: IPersonGalleryItem[];
}
const PersonGallery = ({ persons, ...props }: PersonGalleryProps, requestState: IRequestState) => {
    return new GalleryBlock({
        ...props,
        item_spacing: 16,
        items: persons.map(item => PersonsGalleryItemTemplate(item, requestState)),
    });
};

type PersonsSection = Pick<NAlice.NData.TSearchRichCardData.TBlock.ITSection, 'Persons'>;

const schema = {
    type: 'object',
    required: ['Persons'],
    properties: {
        Persons: {
            type: 'object',
            required: ['Persones'],
            properties: {
                Persones: {
                    type: 'array',
                    items: {
                        type: 'object',
                        required: ['Image', 'Name'],
                        properties: {
                            Image: {
                                type: 'object',
                                required: ['Url'],
                                properties: {
                                    Url: { type: 'string' },
                                },
                            },
                            Name: { type: 'string' },
                            TypedAction: {
                                type: 'object',
                                required: ['value'],
                                properties: {
                                    value: { type: 'object' },
                                },
                            },
                        },
                    },
                    minItems: 1,
                },
            },
        },
    },
};

const dataAdapter = createDataAdapter(schema, (section: PersonsSection, requestState) => {
    const data = section.Persons?.Persones ?? [];

    const persons = data.map<IPersonGalleryItem>(({ Image, Name }) => {
        const title = Name ?? ' ';

        const imageUrl = (() => {
            if (!Image?.Url) {
                return ' ';
            }

            return Avatar.setImageSize({
                data: Image.Url,
                size: 'small',
                type: 'Face',
                namespace: 'get-entity_search',
            });
        })();

        const shouldUseSkeleton = requestState.hasExperiment(ExpFlags.searchRichCardSkeleton);

        return {
            title,
            imageUrl,
            actions: compact([
                shouldUseSkeleton ? {
                    log_id: 'some',
                    url: directivesAction(createShowViewClientAction(
                        TopLevelCard({
                            log_id: 'some',
                            states: [
                                {
                                    state_id: 0,
                                    div: SearchRichCardSkeletonTemplate({ title: 'Hello World!' }, requestState),
                                },
                            ],
                        }, requestState),
                    )),
                } : undefined,
                getGoToOOTextAction(title),
            ]),
        };
    });

    return { persons };
});

export const SearchRichCardPersonsSection: SectionTemplate = (section, requestState) => {
    const { persons } = dataAdapter(section, requestState);

    if (persons.length === 0) {
        return undefined;
    }

    return PersonGallery(
        {
            persons,
            paddings: {
                left: offsetFromEdgeOfScreen,
                right: offsetFromEdgeOfScreen,
            },
            margins: { top: 32 },
        },
        requestState,
    );
};
