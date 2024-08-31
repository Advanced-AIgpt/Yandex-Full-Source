import { GalleryBlock, SolidBackground, TextBlock, WrapContentSize } from 'divcard2';
import { upperFirst } from 'lodash';
import { NAlice } from '../../../../protos';
import { text32r, title36m } from '../../style/Text/Text';
import { notTransparentSuggestColor, transparentSuggestColor } from '../../style/constants';
import { textAction } from '../../../../common/actions';

interface ActionButtonOptions {
    transparent?: boolean;
}

type ITActionButton = NAlice.NScenarios.TLayout.TSuggest.ITActionButton & ActionButtonOptions;

const SuggestButtonVertikalPadding = 13;
const SuggestButtonHorizontalPadding = 24;

export const suggestButtonHeight = text32r.line_height as number + SuggestButtonVertikalPadding * 2;

const Suggest = (
    ActionButton?: ITActionButton | null,
    options: Partial<ConstructorParameters<typeof TextBlock>[0]> = {},
) => {
    if (ActionButton && typeof ActionButton?.transparent === 'undefined') {
        ActionButton.transparent = true;
    }
    return new TextBlock({
        ...title36m,
        background: [
            ActionButton && ActionButton.transparent ?
                new SolidBackground({ color: transparentSuggestColor }) :
                new SolidBackground({ color: notTransparentSuggestColor })
            ,
        ],
        border: { corner_radius: suggestButtonHeight / 2 },
        text: (ActionButton?.Title && upperFirst(ActionButton.Title)) || '',
        paddings: {
            top: SuggestButtonVertikalPadding,
            right: SuggestButtonHorizontalPadding,
            bottom: SuggestButtonVertikalPadding,
            left: SuggestButtonHorizontalPadding,
        },
        actions: [
            {
                log_id: ActionButton?.ActionId ?? '',
                url: textAction(ActionButton?.Title ?? ''),
            },
        ],
        width: new WrapContentSize(),
        height: new WrapContentSize(),
        ...options,
    });
};

export const SuggestsBlock = (
    ResponseBody: NAlice.NScenarios.ITScenarioResponseBody,
    options: Partial<ConstructorParameters<typeof GalleryBlock>[0]> = {},
    suggestOptions: Parameters<typeof Suggest>[1] = {},
) => {
    const suggests = ResponseBody?.Layout?.SuggestButtons ?? [];
    return new GalleryBlock({
        item_spacing: 24,
        items:
            suggests
                .filter(({ ActionButton }) => Boolean(ActionButton))
                .map(({ ActionButton }) => {
                    return Suggest(ActionButton, suggestOptions);
                }),
        ...options,
    });
};

export const SuggestsBlockNormal = (
    SuggestButtons: ITActionButton[],
    options: Partial<ConstructorParameters<typeof GalleryBlock>[0]> = {},
) => {
    return new GalleryBlock({
        item_spacing: 24,
        items:
            SuggestButtons
                .filter(({ ActionId }) => Boolean(ActionId))
                .map(action => Suggest(action)),
        ...options,
    });
};
