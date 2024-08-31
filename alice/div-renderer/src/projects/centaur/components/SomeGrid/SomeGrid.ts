import { ContainerBlock, FixedSize, MatchParentSize, Template } from 'divcard2';
import { chain } from 'lodash';
import { insertBetween } from '../../helpers/helpers';
import { ContainerBlockProps, SeparatorBlockProps } from '../../helpers/types';
import EmptyDiv from '../EmptyDiv';

const orientationToKeyMap: Record<
    SomeGridSeparatorOrientation,
    keyof Pick<Exclude<SeparatorBlockProps, undefined>, 'height' | 'width'>
> = {
    vertical: 'height',
    horizontal: 'width',
};

type SomeGridSeparatorOrientation = 'vertical' | 'horizontal';
interface SomeGridSeparatorProps {
    orientation: SomeGridSeparatorOrientation;
    itemSpacing?: number;
}
const SomeGridSeparator = ({ orientation, itemSpacing }: SomeGridSeparatorProps) => {
    const key = orientationToKeyMap[orientation];

    return new EmptyDiv({
        [key]: itemSpacing ? new FixedSize({ value: itemSpacing }) : new MatchParentSize(),
    });
};

interface SomeGridProps extends ContainerBlockProps {
    columnCount: number;
    itemSpacing?: number;
    items: Exclude<ContainerBlockProps['items'], Template | undefined>;
}
export const SomeGrid = ({
    items,
    columnCount,
    itemSpacing,
    ...props
}: SomeGridProps) => {
    const rows = chain(items)
        .chunk(columnCount)
        .map(row => {
            const separator = SomeGridSeparator({ itemSpacing, orientation: 'horizontal' });
            const items = insertBetween(row, separator);

            return new ContainerBlock({ orientation: 'horizontal', items });
        })
        .value();
    const resultItems = insertBetween(rows, SomeGridSeparator({ itemSpacing, orientation: 'vertical' }));

    return new ContainerBlock({
        orientation: 'vertical',
        items: resultItems,
        ...props,
    });
};
