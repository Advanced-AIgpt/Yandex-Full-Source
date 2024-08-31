import { compact } from 'lodash';
import { NAlice } from '../../../../../protos';
import {
    mainTextColor,
    offsetFromEdgeOfScreen,
} from '../../../style/constants';
import { GalleryBlock, IDivAction, SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { title36m } from '../../../style/Text/Text';

import { isGalleryBlock } from '../SearchRichCard.tools';
import { GalleryBlockProps } from '../../../helpers/types';
import { createDataAdapter } from '../../../helpers/createDataAdapter';
import { IRequestState } from '../../../../../common/types/common';
import { setCurrentItem } from '../../../../../common/actions/div';

interface Tab {
    text: string;
    action: IDivAction;
}

interface TabsProps extends GalleryBlockProps {
    tabs: Tab[];
}
const Tabs = ({ tabs, ...props }: TabsProps) => {
    return new GalleryBlock({
        ...props,
        item_spacing: 16,
        items: tabs.map(({ text, action }) => {
            return new TextBlock({
                ...title36m,
                text,
                width: new WrapContentSize(),
                border: { corner_radius: 24 },
                background: [new SolidBackground({ color: '#374352' })],
                text_color: mainTextColor,
                paddings: { left: 24, top: 13, right: 24, bottom: 13 },
                action,
            });
        }),
    });
};

const schema = {
    type: 'object',
    required: ['Blocks'],
    properties: {
        Blocks: {
            type: 'array',
            minItems: 1,
            items: {
                type: 'object',
                required: ['BlockType', 'TitleNavigation', 'Title'],
                properties: {
                    BlockType: { type: 'number' },
                    TitleNavigation: { type: 'string' },
                    Title: { type: 'string' },
                },
            },
        },
    },
};

const dataAdapter = createDataAdapter(schema, (data: NAlice.NData.ITSearchRichCardData) => {
    const blocks = data.Blocks ?? [];

    const tabs = compact(
        blocks
            // выфильтровываем Gallery потому что секции этого блока будут в составе Main
            .filter(block => isGalleryBlock(block) === false)
            .map<Tab | undefined>(({ TitleNavigation, Title }, index) => {
                const text = TitleNavigation || Title;
                // +1 потому что в блоке галереи первый блок хедер, а не блок контента,
                // но в то же время первый якорь скролла не контентый блок, а блок хедера,
                // потому у что главного контентного блока нет хедера, его хедер - общестраничный хедер
                const indexToNavigate = index === 0 ? 0 : index + 1;

                if (!text) {
                    return undefined;
                }

                return ({
                    text,
                    action: {
                        log_id: 'scroll_to_section',
                        url: setCurrentItem('main_gallery', indexToNavigate),
                    },
                });
            }),
    );

    return { tabs };
});

interface SearchRichCardHeaderNavigationTabsProps extends Partial<TabsProps> {
    data: NAlice.NData.ITSearchRichCardData;
    requestState: IRequestState;
}
export const SearchRichCardHeaderNavigationTabs = ({ data, requestState, ...props }: SearchRichCardHeaderNavigationTabsProps) => {
    const { tabs } = dataAdapter(data, requestState);

    if (tabs.length === 0) {
        return undefined;
    }

    return Tabs({
        tabs,
        paddings: {
            left: offsetFromEdgeOfScreen,
            right: offsetFromEdgeOfScreen,
        },
        ...props,
    });
};
