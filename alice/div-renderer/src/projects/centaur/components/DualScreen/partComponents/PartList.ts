import {
    ContainerBlock,
    Div,
    FixedSize,
    GalleryBlock, SolidBackground,
} from 'divcard2';
import { offsetFromEdgeOfScreen } from '../../../style/constants';
import { IColorSet } from '../../../style/colorSet';
import { IRequestState } from '../../../../../common/types/common';

interface IPartListItem {
    items: Div[];
    options?: Omit<Partial<ConstructorParameters<typeof ContainerBlock>[0]>, 'items'>;
}

interface IPartListProps {
    wrapperOptions?: Omit<Partial<ConstructorParameters<typeof GalleryBlock>[0]>, 'items'>;
    items: IPartListItem[];
    colorSet: IColorSet;
    id: string;
    requestState: IRequestState;
}

const defaultItemsProps: Partial<ConstructorParameters<typeof ContainerBlock>[0]> = {
    paddings: {
        top: 28,
        left: offsetFromEdgeOfScreen,
        bottom: 28,
        right: offsetFromEdgeOfScreen,
    },
};

export default function PartList({
    wrapperOptions = {},
    colorSet,
    items,
    id,
    requestState,
}: IPartListProps): Div[] {
    return [new GalleryBlock({
        height: new FixedSize({ value: requestState.sizes.height }),
        orientation: 'vertical',
        paddings: {
            top: 96,
        },
        ...wrapperOptions,
        id,
        items: items.map((item, index) => {
            const containerProps: ConstructorParameters<typeof ContainerBlock>[0] = {
                ...defaultItemsProps,
                background: index % 2 === 0 ? undefined : [new SolidBackground({ color: colorSet.mainColor2 })],
                ...item.options,
                items: item.items,
            };
            return new ContainerBlock(containerProps);
        }),
    })];
}
