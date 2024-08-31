/**
 * copied from https://github.yandex-team.ru/search-interfaces/frontend/blob/master/services/station-ui/src/typings/quasar.ts
 */
declare namespace quasar {
    export interface QuasarApi {
        callServer: (command: string, payload: any) => Promise<any>;
        onNavigation: (callback: (event: NativeNavigationEvent) => void) => void;
        offNavigation: (callback: (event: NativeNavigationEvent) => void) => void;
        onCommand: (command: string, callback: (event: NativeCommandEvent) => void) => void;
        offCommand: (command: string, callback: (event: NativeCommandEvent) => void) => void;
        pageLoaded: () => void;
        ready: () => void;
        setPageTitle: (title: string) => void;
        setPageUrl: (pageUrl: string) => void;
        setAvailableNavDirections: (navDirections: NavDirections) => void;
        updateSuggest: (suggests: string[]) => void;
        updateAsr: (asr: Asr[]) => void;
        setState: (state: QuasarAppState) => void;
    }

    export interface NativeCommandEvent {
        type: string;
        detail?: {
            meta?: string;
        };
    }

    export interface SetCodeNativeCommandEventPayload {
        code: string;
    }

    export interface NativeNavigationEvent {
        detail: {
            direction: directionTypes;
            scroll_amount: ScrollAmount;
        };
    }

    export enum directionTypes {
        right = 'right',
        left = 'left',
        up = 'up',
        down = 'down',
    }

    export type ScrollAmount = 1 | 2 | 3 | 4 | 5 | 'till_end' | 'many' | 'few';

    export type NavDirections = Record<directionTypes, boolean>;

    export interface Asr {
        text: string;
    }

    export interface QuasarAppState {
        alice4business?: boolean;
        currentScreen: string;
        activationCode: string;
    }
}

interface Window {
    quasar: quasar.QuasarApi;
}
