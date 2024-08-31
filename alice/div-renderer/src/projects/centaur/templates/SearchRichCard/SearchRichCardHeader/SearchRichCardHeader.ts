import { ContainerBlock, FixedSize, ImageBlock, TextBlock, WrapContentSize } from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../../protos';
import { colorBlueText, offsetFromEdgeOfScreen } from '../../../style/constants';
import { title36m, title64m } from '../../../style/Text/Text';

import { SearchRichCardHeaderNavigationTabs } from './SearchRichCardHeaderNavigationTabs';
import { ContainerBlockProps } from '../../../helpers/types';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { IRequestState } from '../../../../../common/types/common';

interface HeaderProps extends ContainerBlockProps {
    title: string;
    subtitle?: string | null;
    withImage?: boolean;
    imageUrl?: string | null;
}
const Header = ({
    title,
    subtitle,
    withImage,
    imageUrl,
    ...props
}: HeaderProps) => {
    const image = (() => {
        if (!withImage || !imageUrl) {
            return null;
        }

        return new ContainerBlock({
            width: new FixedSize({ value: 180 }),
            height: new WrapContentSize(),
            paddings: {
                right: 45,
            },
            items: [
                new ImageBlock({
                    image_url: imageUrl,
                }),
            ],
        });
    })();

    return new ContainerBlock({
        ...props,
        orientation: 'horizontal',
        items: compact([
            image,
            new ContainerBlock({
                items: compact([
                    new TextBlock({
                        text: title,
                        ...title64m,
                        height: new FixedSize({ value: 80 }),
                        text_color: colorBlueText,
                    }),
                    subtitle && new TextBlock({
                        text: subtitle,
                        ...title36m,
                        height: new FixedSize({ value: 46 }),
                        text_color: colorBlueText,
                    }),
                ]),
            }),
        ]),
    });
};

const schema = {
    type: 'object',
    required: ['Header'],
    properties: {
        Header: {
            type: 'object',
            required: ['Text'],
            properties: {
                Text: { type: 'string' },
            },
        },
    },
};

const dataAdapter = createDataAdapter(schema, (rawData: NAlice.NData.ITSearchRichCardData) => {
    const data = rawData.Header ?? {};

    const header = {
        title: data.Text ?? ' ',
        subtitle: data.ExtraText,
        imageUrl: data.Image?.Url,
    };

    return { header };
});

interface SearchRichCardHeaderProps extends ContainerBlockProps {
    data: NAlice.NData.ITSearchRichCardData;
    requestState: IRequestState;
}
export const SearchRichCardHeader = ({ data, requestState, ...props }: SearchRichCardHeaderProps) => {
    const { header: { title, subtitle, imageUrl } } = dataAdapter(data, requestState);

    return new ContainerBlock({
        paddings: { top: 48 },
        items: compact([
            Header({
                title,
                subtitle,
                imageUrl,
                paddings: { left: offsetFromEdgeOfScreen, right: offsetFromEdgeOfScreen },
            }),
            SearchRichCardHeaderNavigationTabs({
                data,
                requestState,
                margins: { top: 48 },
            }),
        ]),
        ...props,
    });
};
