import { ContainerBlock, DivCustomBlock, MatchParentSize } from 'divcard2';
import { createWebviewClientAction } from './client';
import { AnonymizeDataForSnapshot } from '../../../common/helpers/dev';
import { directivesAction } from '../../../common/actions';

describe('Client action directive', () => {
    it('matches snapshot', () => {
        expect(AnonymizeDataForSnapshot(directivesAction([{
            type: 'client_action', name: 'show_view', payload: {
                layer: {
                    content: {},
                },
                div2_card: {
                    body: {
                        card: {
                            states: [
                                {
                                    state_id: 0,
                                    div: new ContainerBlock({
                                        items: [
                                            new DivCustomBlock({
                                                custom_type: 'centaur_webview',
                                                custom_props: {
                                                    url: 'https://yandex.ru/quasar',
                                                },
                                                width: new MatchParentSize(),
                                                height: new MatchParentSize(),
                                                alignment_vertical: 'bottom',
                                            }),
                                        ],
                                        width: new MatchParentSize(),
                                        height: new MatchParentSize(),
                                        alignment_vertical: 'bottom',
                                    }),
                                },
                            ],
                            log_id: 'iot-web',
                        },
                        templates: {},
                    },
                },
            },
        }]))).toMatchSnapshot();
    });
});

describe('Create webview client action', () => {
    it('matches snapshot', () => {
        expect(AnonymizeDataForSnapshot(createWebviewClientAction('https://yandex.ru'))).toMatchSnapshot();
    });
});
