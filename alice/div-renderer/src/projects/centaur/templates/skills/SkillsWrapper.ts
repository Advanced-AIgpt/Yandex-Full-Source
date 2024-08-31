import {
    Div,
} from 'divcard2';
import FullScreen from '../../components/FullScreen';
import TitleLine from '../../components/TitleLine/TitleLine';
import { SuggestsBlockNormal } from '../suggests';
import {
    gradientToBlackBottom,
    gradientToBlackTop,
    offsetFromEdgeOfScreen,
} from '../../style/constants';
import { Types } from './types';
import { defaultActionsCloseLayer } from '../../components/CloseButton';
import { directivesAction } from '../../../../common/actions';
import { createSemanticFrameAction } from '../../../../common/actions/server';

export const DeviceName = 'SmartSpeaker';

interface Props {
    data: Readonly<Types>;
    children: Readonly<Div[]>;
}

export default function SkillsWrapper({ data, children }: Props) {
    const { skillInfo, response } = data;

    const skillName = skillInfo.name;
    const skillLogo = skillInfo.logo;
    const suggests = response.suggests;

    return FullScreen({
        children: [
            ...children,
            TitleLine({
                title: skillName,
                icon: skillLogo,
                needClose: true,
                options: { background: gradientToBlackTop },
                closeButtonOptions: {
                    options: {
                        actions: [
                            {
                                log_id: 'action_close_skill',
                                url: directivesAction(
                                    createSemanticFrameAction(
                                        {
                                            external_skill_force_deactivate_semantic_frame: {
                                                dialog_id: {
                                                    string_value: skillInfo.dialogId,
                                                },
                                                silent_response: {
                                                    bool_value: true,
                                                },
                                            },
                                        },
                                        'Dialogovo',
                                        DeviceName,
                                        'alice.external_skill_force_deactivate',
                                    ),
                                ),
                            },
                            ...defaultActionsCloseLayer,
                        ],
                    },
                },
            }),
            SuggestsBlockNormal(
                suggests.map(el => ({ Title: el.text, ActionId: el.url, transparent: false })),
                {
                    alignment_vertical: 'bottom',
                    alignment_horizontal: 'left',
                    paddings: {
                        left: offsetFromEdgeOfScreen,
                        right: offsetFromEdgeOfScreen,
                        bottom: offsetFromEdgeOfScreen,
                    },
                    background: gradientToBlackBottom,
                },
            ),
        ],
        options: {
            orientation: 'overlap',
        },
    });
}
