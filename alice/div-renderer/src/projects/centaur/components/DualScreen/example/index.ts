import * as path from 'path';
import { ContainerBlock, TextBlock } from 'divcard2';
import divToFile from '../../../../../common/helpers/divToFile';
import FullScreen from '../../FullScreen';
import { TopLevelCard } from '../../../helpers/helpers';
import { EnumLayer, EnumInactivityTimeout } from '../../../actions/client';
import { text42m } from '../../../style/Text/Text';
import exampleSplitScreenImageAndText from './exampleSplitScreenImageAndText';
import exampleSplitScreenDoubleTextAndImage from './exampleSplitScreenDoubleTextAndImage';
import basicExampleDiv from './basicExampleDiv';
import { directivesAction } from '../../../../../common/actions';

const examples = [
    {
        title: 'Текст с двух сторон и картинка с затемнением',
        div: exampleSplitScreenDoubleTextAndImage,
    },
    {
        title: 'Текст и картинка',
        div: exampleSplitScreenImageAndText,
    },
];

divToFile(path.resolve(__dirname, 'test.json'), {
    div2CardBody: TopLevelCard({
        log_id: 'example',
        states: [
            {
                state_id: 0,
                div: FullScreen({
                    children: [
                        new ContainerBlock({
                            paddings: {
                                top: 48,
                                left: 48,
                                bottom: 48,
                                right: 48,
                            },
                            orientation: 'vertical',
                            items: examples.map((ex, i) => {
                                return new TextBlock({
                                    ...text42m,
                                    text: ex.title,
                                    margins: {
                                        bottom: 30,
                                    },
                                    actions: [
                                        {
                                            log_id: `example_${i}`,
                                            url: directivesAction(basicExampleDiv(ex.div())),
                                        },
                                    ],
                                });
                            }),
                        }),
                    ],
                }),
            },
        ],
    }),
    layer: EnumLayer.content,
    doNotShowCloseButton: false,
    inactivityTimeout: EnumInactivityTimeout.infinity,
});
