import { FixedSize, SolidBackground, TextBlock } from 'divcard2';
import { createLocalTemplate } from '../../../../common/helpers/createTemplate';

type SomeProps = {
    title: string;
}
export const SearchRichCardSkeletonTemplate = createLocalTemplate<SomeProps>(({ title }) => {
    return new TextBlock({
        height: new FixedSize({ value: 100 }),
        width: new FixedSize({ value: 100 }),
        background: [new SolidBackground({ color: 'red' })],
        text: title,
    });
});
