import { Directive } from '../../../../common/actions/index';
import { createSemanticFrameActionTypeSafe } from '../../../../common/actions/server/index';
import { TTypedSemanticFrame } from '../../../../protos/alice/megamind/protos/common/frame';

export function telegramVideoCallTo(
    contactUserId: string,
    contactDisplayName: string | null | undefined,
    videoEnabled: boolean = false,
): Directive {
    return createSemanticFrameActionTypeSafe(
        {
            VideoCallToSemanticFrame: {
                FixedContact: {
                    ContactData: {
                        TelegramContactData: {
                            DisplayName: contactDisplayName ? contactDisplayName : '',
                            UserId: contactUserId,
                        },
                    },
                },
                VideoEnabled: {
                    BoolValue: videoEnabled,
                },
            },
        } as TTypedSemanticFrame,
        'VideoCall',
        'alice_scenarios.video_call_to',
    );
}
