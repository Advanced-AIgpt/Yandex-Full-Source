import { compact } from 'lodash';
import { IVideoCallLogoutedProps } from './types';
import { EnumLayer } from '../../../../actions/client';
import { ICardDataAdapter } from '../types';
import { directivesAction } from '../../../../../../common/actions';
import * as uuid from 'uuid';
import { telegramStartLoginDirective, telegramStartLoginShowView } from '../../../../actions/client/telegramClientActions';
import { createServerActionBasedOnTSF } from '../../../../../../common/actions/server';
import { IFavoriteContactItem, IVideoCallLoginedProps } from './VideoCallCard';
import { telegramVideoCallTo } from '../../../../actions/server/telegramServerActions';

export const DeviceName = 'SmartSpeaker';

export const getVideoCallLogoutedCardData: ICardDataAdapter<IVideoCallLogoutedProps> =
    function getVideoCallLogoutedCardData(card, requestState) {
        if (typeof card.VideoCallCardData !== 'undefined' && card.VideoCallCardData !== null) {
            const data = card.VideoCallCardData;
            if (data.LoggedOutCardData !== 'undefined' && data.LoggedOutCardData !== null) {
                const id = uuid.v4();
                const log_id = 'main_screen_video_call_action';
                const serverAction = createServerActionBasedOnTSF({
                    binaryTsf: card.TypedAction?.value,
                    productScenario: 'CentaurMainScreen',
                    purpose: log_id,
                });

                const url = (() => {
                    if (serverAction) {
                        return directivesAction(serverAction);
                    }

                    if (!card.Action) {
                        return directivesAction([
                            telegramStartLoginDirective(id),
                            telegramStartLoginShowView(id, requestState),
                        ]);
                    }

                    return card.Action;
                })();

                return {
                    type: 'video_call_logouted',
                    actions: compact([{
                        log_id,
                        url,
                    }]),
                    requestState,
                    layer: EnumLayer.dialog,
                };
            }
        }
        return null;
    };

export const getVideoCallLoginedCardData: ICardDataAdapter<IVideoCallLoginedProps> = (card, requestState, options) => {
    const telegramData = card.VideoCallCardData?.LoggedInCardData?.TelegramCardData;

    if (!telegramData) {
        return null;
    }

    const favorites = (() => {
        const data = telegramData.FavoriteContactData ?? [];

        return compact(data.map<IFavoriteContactItem | undefined>(({ DisplayName, UserId }) => {
            if (!DisplayName || !UserId) {
                return undefined;
            }

            const isChooseMode = options?.isChoice ?? false;

            return {
                title: DisplayName,
                userId: UserId,
                actions: compact([
                    isChooseMode === false && UserId && {
                        log_id: 'main_screen_video_call_to_action',
                        url: directivesAction(telegramVideoCallTo(
                            UserId,
                            DisplayName,
                        )),
                    },
                ]),
            };
        }));
    })();

    return {
        type: 'video_call_logined',
        requestState,
        layer: EnumLayer.content,
        contactsUploaded: telegramData?.ContactsUploaded,
        favorites,
    };
};
