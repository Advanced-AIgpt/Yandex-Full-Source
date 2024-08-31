import {
    FixedSize,
    GalleryBlock,
    MatchParentSize,
    SolidBackground,
} from 'divcard2';
import { NAlice } from '../../../../protos';
import {
    offsetFromEdgeOfScreen,
} from '../../style/constants';

import { SearchRichCardMainBlock } from './SearchRichCardMainBlock';
import { SearchRichCardHeader } from './SearchRichCardHeader/SearchRichCardHeader';
import { SearchRichCardBlock } from './SearchRichCardBlock';
import { isGalleryBlock, isMainBlock } from './SearchRichCard.tools';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { IRequestState } from '../../../../common/types/common';
import { compact } from 'lodash';
import { createDataAdapter } from '../../helpers/createDataAdapter';
import { CloseButtonWrapper } from '../../components/CloseButtonWrapper/CloseButtonWrapper';
import { TopLevelCard } from '../../helpers/helpers';

const schema = {
    type: 'object',
    required: ['Blocks'],
    properties: {
        Blocks: {
            type: 'array',
            minItems: 1,
        },
    },
};

const otherBlockListDataAdapter = createDataAdapter(
    schema,
    (rawData: NAlice.NData.ITSearchRichCardData) => {
        const data = rawData.Blocks ?? [];

        const otherBlocks = data.filter(
            block =>
                isMainBlock(block) === false && isGalleryBlock(block) === false,
        );

        return { otherBlocks };
    },
);

const SearchRichCardOtherBlockList = (
    data: NAlice.NData.ITSearchRichCardData,
    requestState: IRequestState,
) => {
    const { otherBlocks } = otherBlockListDataAdapter(data, requestState);

    return otherBlocks.map(block => SearchRichCardBlock(block, requestState));
};

export default function SearchRichCard(
    data: NAlice.NData.ITSearchRichCardData,
    _mmRequest: MMRequest,
    requestState: IRequestState,
) {
    return TopLevelCard({
        /**
         * Math.random() нужен для того, чтобы каждый новый show_view с карточкой был новым инстансом.
         * Новый инстанс нужен для того, чтобы сбрасывать значение скролла.
         */
        log_id: 'search_rich_' + Math.random(),
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
                    div: new GalleryBlock({
                        id: 'main_gallery',
                        orientation: 'vertical',
                        background: [new SolidBackground({ color: '#1B2129' })],
                        width: new MatchParentSize(),
                        height: new FixedSize({ value: requestState.sizes.height }),
                        paddings: {
                            bottom: offsetFromEdgeOfScreen,
                        },
                        // отступ не 64, как в дизайне, а 16, потому что 48 пикселей
                        // паддинг у хедера каждого блока, это нужно для того чтобы при скролле
                        // до блока, оставалось пространство между хедером и краем экрана сверху
                        item_spacing: 16,
                        items: [
                            SearchRichCardHeader({ data, requestState }),
                            ...compact([
                                SearchRichCardMainBlock(data, requestState),
                                ...SearchRichCardOtherBlockList(data, requestState),
                            ]),
                        ],
                    }),
                }),
            },
        ],
    }, requestState);
}
