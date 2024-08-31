// TODO: deprecated in CENTAUR-526 (use CentaurWebviewData)

import { ContainerBlock, DivCustomBlock, MatchParentSize, TemplateCard, Templates } from 'divcard2';
import { NAlice } from '../../../../protos';
import { DEFAULT_USER_AGENT } from '../../components/cards';

export const renderMainScreenWebviewTab = (
    { WebviewUrl }: NAlice.NData.ITCentaurMainScreenWebviewTabData,
) => {
    return new TemplateCard(new Templates({}), {
        states: [
            {
                state_id: 0,
                div: new ContainerBlock({
                    items: [
                        new DivCustomBlock({
                            custom_type: 'centaur_webview',
                            custom_props: {
                                url: WebviewUrl,
                                user_agent: DEFAULT_USER_AGENT,
                            },
                            width: new MatchParentSize(),
                            height: new MatchParentSize(),
                            alignment_vertical: 'bottom',
                        }),
                    ],
                    orientation: 'vertical',
                    width: new MatchParentSize(),
                    height: new MatchParentSize(),
                }),
            },
        ],
        log_id: 'webview',
    });
};
