import {
    ContainerBlock,
    DivCustomBlock,
    FixedSize,
    ImageBlock,
    MatchParentSize,
    TemplateCard,
    Templates,
    WrapContentSize,
} from 'divcard2';
import { getStaticS3Asset } from '../../helpers/assets';

export const DEFAULT_USER_AGENT = 'Mozilla/5.0 (Linux; Android 9; SM-A102U) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.136 Mobile Safari/537.36';

export const WebviewCard = (webviewUrl: string, userAgent = DEFAULT_USER_AGENT) => {
    return new TemplateCard(new Templates({}), {
        states: [
            {
                state_id: 0,
                div: new ContainerBlock({
                    items: [
                        new DivCustomBlock({
                            custom_type: 'centaur_webview',
                            custom_props: {
                                url: webviewUrl,
                                user_agent: userAgent,
                            },
                            width: new MatchParentSize(),
                            height: new MatchParentSize(),
                            alignment_vertical: 'bottom',
                        }),
                        new ContainerBlock({
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
                                                url: 'centaur://local_command?local_commands=[{\'command\':\'webview_go_back\',\'layer\':\'CONTENT\'}]',
                                            },
                                            preload_required: 1,
                                        }),
                                    ],
                                }),
                            ],
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
