import { TemplateCard } from 'divcard2';
import { createClientAction } from '../../../../common/actions/client';
import { Layer } from '../../common/layers';
import { WebviewCard } from '../../components/cards';
import { ITemplateCard } from '../../helpers/helpers';

export enum EnumLayer {
    content = 'content',
    dialog = 'dialog',
}

export enum EnumInactivityTimeout {
    short = 'Short',
    medium = 'Medium',
    long = 'Long',
    infinity = 'Infinity',
}

export const localCommands = (commands: unknown[]) => {
    return `centaur://local_command?local_commands=[${commands.map(command => encodeURIComponent(JSON.stringify(command)))}]`;
};

export function createDirectiveToShowView({
    div2CardBody,
    doNotShowCloseButton = null,
    layer = EnumLayer.content,
    inactivityTimeout = EnumInactivityTimeout.infinity,
    actionSpaceId,
}: {
    div2CardBody: TemplateCard<string> | ITemplateCard,
    doNotShowCloseButton: boolean | null,
    layer: EnumLayer,
    inactivityTimeout: EnumInactivityTimeout,
    actionSpaceId: String | undefined,
}) {
    return {
        layer: {
            [layer]: {},
        },
        div2_card: {
            body: div2CardBody,
        },
        do_not_show_close_button: doNotShowCloseButton,
        inactivity_timeout: inactivityTimeout,
        action_space_id: actionSpaceId,
    };
}

export const localCommand = (command: unknown) => {
    return localCommands([command]);
};

export const closeLayerLocalAction = (layer = Layer.DIALOG, stopConversation = true) => {
    return localCommand({
        command: 'close_layer',
        layer: Layer[layer],
        stop_conversation: stopConversation,
    });
};

export const closeLayerAction = (logId: string, layer = Layer.DIALOG, stopConversation = true) => {
    return {
        log_id: logId,
        url: closeLayerLocalAction(layer, stopConversation),
    };
};


export const createAndroidIntentClientAction = (action: string, uri: string) => {
    return createClientAction('send_android_app_intent', {
        name: 'send_android_app_intent',
        action,
        uri,
    });
};

export const createClearQueueAction = () => {
    return createClientAction('clear_queue');
};

export const createWebviewClientAction = (webviewUrl: string) => {
    return createShowViewClientAction(WebviewCard(webviewUrl));
};

export const createShowViewClientAction = (
    div2CardBody: TemplateCard<string> | ITemplateCard,
    doNotShowCloseButton: boolean | null = null,
    layer = EnumLayer.content,
    inactivityTimeout = EnumInactivityTimeout.infinity,
    actionSpaceId: String | undefined = undefined,
) => {
    return createClientAction('show_view', createDirectiveToShowView({
        div2CardBody,
        doNotShowCloseButton,
        layer,
        inactivityTimeout,
        actionSpaceId,
    }));
};
