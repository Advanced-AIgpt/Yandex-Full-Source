import { FixedSize, GalleryBlock, TemplateCard, Templates, TextBlock } from 'divcard2';
import { compact } from 'lodash';
import { RenderFn } from '../../..';
import { PrettyJSON } from '../../../example/templates/ScenarioDataPrettyJSON/ScenarioDataPrettyJSON';
import { offsetFromEdgeOfScreen } from '../../style/constants';
import { CloseButtonWrapper } from '../CloseButtonWrapper/CloseButtonWrapper';

interface HumanErrorProps {
    error: Error;
    data: unknown;
}
export const HumanError: RenderFn<HumanErrorProps> = ({ error, data }, _, requestState) => {
    // TODO: CENTAUR-1243: залогировать вызов этого метода в продакшене, этого быть не должно

    return new TemplateCard(new Templates({}), {
        log_id: 'human_error',
        states: [
            {
                state_id: 0,
                div: CloseButtonWrapper({
                    div: new GalleryBlock({
                        orientation: 'vertical',
                        paddings: {
                            top: offsetFromEdgeOfScreen,
                            left: offsetFromEdgeOfScreen,
                            right: offsetFromEdgeOfScreen,
                            bottom: offsetFromEdgeOfScreen,
                        },
                        height: new FixedSize({ value: requestState.sizes.height }),
                        items: compact([
                            new TextBlock({
                                text: error.message,
                                text_color: 'white',
                                font_size: 30,
                            }),
                            error.stack ? new TextBlock({
                                text: error.stack,
                                text_color: 'white',
                                font_size: 30,
                            }) : undefined,
                            PrettyJSON({ data }),
                        ]),
                    }),
                }),
            },
        ],
    });
};
