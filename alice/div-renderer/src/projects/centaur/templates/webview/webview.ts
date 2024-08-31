import { ContainerBlock, DivCustomBlock, FixedSize, ImageBlock, MatchParentSize, TemplateCard, Templates, WrapContentSize } from 'divcard2';
import { compact } from 'lodash';
import { NAlice } from '../../../../protos';
import { DEFAULT_USER_AGENT } from '../../components/cards';
import { getStaticS3Asset } from '../../helpers/assets';

export const renderWebview = (
    { WebviewUrl, ShowNavigationBar, MediaSessionId }: NAlice.NData.ITCentaurWebviewData,
) => {
    return new TemplateCard(new Templates({}), {
        states: [
            {
                state_id: 0,
                div: new ContainerBlock({
                    items: compact([
                        new DivCustomBlock({
                            id: WebviewUrl ?? 'webview',
                            custom_type: 'centaur_webview',
                            custom_props: {
                                url: WebviewUrl,
                                media_session_id: MediaSessionId,
                                user_agent: DEFAULT_USER_AGENT,
                            },
                            width: new MatchParentSize(),
                            height: new MatchParentSize(),
                            alignment_vertical: 'bottom',
                        }),
                        ShowNavigationBar && new ContainerBlock({
                            orientation: 'overlap',
                            width: new MatchParentSize(),
                            height: new WrapContentSize(),
                            alignment_vertical: 'top',
                            items: [
                                new ContainerBlock({
                                    orientation: 'horizontal',
                                    width: new WrapContentSize(),
                                    height: new WrapContentSize(),
                                    alignment_horizontal: 'right',
                                    alignment_vertical: 'top',
                                    margins: {
                                        top: 16,
                                        right: 24,
                                        bottom: 16,
                                    },
                                    items: [
                                        new ImageBlock({
                                            image_url: getStaticS3Asset('icons/close_wt_border.png'),
                                            width: new FixedSize({ value: 56 }),
                                            height: new FixedSize({ value: 56 }),
                                            action: {
                                                log_id: 'close_webview',
                                                url: 'centaur://local_command?local_commands=[{\'command\':\'close_layer\',\'layer\':\'CONTENT\'}]',
                                            },
                                            preload_required: 1,
                                        }),
                                    ],
                                }),
                                new ContainerBlock({
                                    orientation: 'horizontal',
                                    width: new WrapContentSize(),
                                    height: new WrapContentSize(),
                                    alignment_horizontal: 'left',
                                    alignment_vertical: 'top',
                                    margins: {
                                        top: 16,
                                        left: 24,
                                        bottom: 16,
                                    },
                                    items: [
                                        new ImageBlock({
                                            image_url: getStaticS3Asset('icons/back_wt_border.png'),
                                            width: new FixedSize({ value: 56 }),
                                            height: new FixedSize({ value: 56 }),
                                            action: {
                                                log_id: 'webview_go_back',
                                                url: 'centaur://local_command?local_commands=[{\'command\':\'webview_go_back\',\'layer\':\'DIALOG\'}]',
                                            },
                                            preload_required: 1,
                                        }),
                                    ],
                                }),
                            ],
                        }),
                    ]),
                    orientation: 'vertical',
                    width: new MatchParentSize(),
                    height: new MatchParentSize(),
                }),
            },
        ],
        log_id: 'webview',
    });
};
