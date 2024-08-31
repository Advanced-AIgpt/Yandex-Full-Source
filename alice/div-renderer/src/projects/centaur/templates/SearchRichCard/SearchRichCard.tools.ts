import { FixedSize, IDivAction, SeparatorBlock } from 'divcard2';
import { textAction } from '../../../../common/actions';
import { NAlice } from '../../../../protos';

export const isGalleryBlock = ({ BlockType }: NAlice.NData.TSearchRichCardData.ITBlock) =>
    BlockType === NAlice.NData.TSearchRichCardData.TBlock.EBlockType.Gallery;

export const isMainBlock = ({ BlockType }: NAlice.NData.TSearchRichCardData.ITBlock) =>
    BlockType === NAlice.NData.TSearchRichCardData.TBlock.EBlockType.Main;

export const SectionSeparator = () =>
    new SeparatorBlock({ height: new FixedSize({ value: 48 }) });

export const getGoToOOTextAction = (text: string) => {
    return <IDivAction>{
        log_id: 'go_to_oo',
        url: textAction(`Кто такой ${text}, википедия`),
    };
};
