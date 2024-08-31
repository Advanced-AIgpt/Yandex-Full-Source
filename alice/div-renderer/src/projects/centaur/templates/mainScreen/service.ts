import {
    ContainerBlock,
    FixedSize,
    ImageBlock,
    MatchParentSize,
    TemplateCard,
    Templates,
    TextBlock,
    WrapContentSize,
} from 'divcard2';
import { NAlice } from '../../../../protos';
import {
    directivesAction,
    Directive,
} from '../../../../common/actions';
import { text32r } from '../../style/Text/Text';
import { createAndroidIntentClientAction, createWebviewClientAction } from '../../actions/client';

export const renderMainScreenService = ({
    WebviewUrl,
    ImageUrl,
    Id,
    Title,
}: NAlice.NData.ITCentaurMainScreenWebviewCardData) =>
    new TemplateCard(new Templates({}), {
        log_id: 'main_screen.service',
        states: [
            {
                state_id: 0,
                div: new ContainerBlock({
                    width: new MatchParentSize(),
                    height: new MatchParentSize(),
                    content_alignment_horizontal: 'center',
                    items: [
                        new ImageBlock({
                            width: new FixedSize({ value: 168 }),
                            height: new FixedSize({ value: 168 }),
                            image_url: ImageUrl ?? '',
                            action: {
                                log_id: 'main_screen.service.click.' + Id,
                                url: directivesAction([createAction(WebviewUrl, Id)]),
                            },
                            preload_required: 1,
                            border: {
                                corner_radius: 28,
                            },
                        }),
                        new TextBlock({
                            ...text32r,
                            text: Title ?? '',
                            width: new MatchParentSize(),
                            height: new WrapContentSize(),
                            auto_ellipsize: 1,
                            text_alignment_horizontal: 'center',
                            margins: { top: 24 },
                        }),
                    ],
                }),
            },
        ],
    });

type protoString = string | null | undefined;
function createAction(WebviewUrl: protoString, Id: protoString): Directive {
    // tmp hack for testing android intent directive with click
    return Id === 'rickroll' ?
        createAndroidIntentClientAction('android.intent.action.VIEW', 'vnd.youtube:dQw4w9WgXcQ') :
        createWebviewClientAction(WebviewUrl ?? '');
}
