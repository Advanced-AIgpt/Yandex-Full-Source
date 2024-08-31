import {
    ContainerBlock,
    GradientBackground,
    ImageBackground,
    MatchParentSize,
    SolidBackground,
    TemplateCard,
    Templates,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { compact } from 'lodash';
import EmptyDiv from '../../components/EmptyDiv';
import { NAlice } from '../../../../protos';
import { MMRequest } from '../../../../common/helpers/MMRequest';
import { ExpFlags, hasExperiment } from '../../expFlags';
import { text28r } from '../../style/Text/Text';
import { colorWhiteOpacity50 } from '../../style/constants';

export default function(
    data: NAlice.NData.ITScreenSaverData,
    mmRequest: MMRequest,
) {
    if (!data.ImageUrl) {
        throw new Error('ImageUrl must exists');
    }
    return new TemplateCard(new Templates({}), {
        log_id: 'gallery-screensaver',
        states: [
            {
                state_id: 0,
                div: hasExperiment(mmRequest, ExpFlags.photoFrameDebugUpdateData) ?
                    new ContainerBlock({
                        width: new MatchParentSize(),
                        height: new MatchParentSize(),
                        content_alignment_vertical: 'bottom',
                        background: compact([
                            new ImageBackground({
                                image_url: data.ImageUrl,
                                preload_required: 1,
                            }),
                            new SolidBackground({
                                color: '#2E000000',
                            }),
                            new GradientBackground({
                                colors: [
                                    '#29000000',
                                    '#1F000000',
                                    '#00000000',
                                ],
                                angle: 74,
                            }),
                        ]),
                        items: [
                            new TextBlock({
                                ...text28r,
                                text_color: colorWhiteOpacity50,
                                text: `обновлено: ${new Date().toISOString()}`,
                                alignment_vertical: 'bottom',
                                alignment_horizontal: 'left',
                                height: new WrapContentSize(),
                                letter_spacing: 0.75,
                                alpha: 0.8,
                                paddings: {
                                    top: 36,
                                    left: 36,
                                    bottom: 36,
                                },
                            }),
                        ],
                    }) :
                    new EmptyDiv({
                        width: new MatchParentSize(),
                        height: new MatchParentSize(),
                        background: compact([
                            new ImageBackground({
                                image_url: data.ImageUrl,
                                preload_required: 1,
                            }),
                            new SolidBackground({
                                color: '#2E000000',
                            }),
                            new GradientBackground({
                                colors: [
                                    '#29000000',
                                    '#1F000000',
                                    '#00000000',
                                ],
                                angle: 74,
                            }),
                        ]),
                    }),
            },
        ],
    });
}
