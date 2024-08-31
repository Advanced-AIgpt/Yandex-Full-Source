declare module 'lego-on-react' {
    import { Component } from 'react';

    type MetrikaCounter = string;

    interface MetrikaConfig {
        id: MetrikaCounter;
        clickmap?: boolean;
        trackLinks?: boolean;
        accurateTrackBounce?: boolean;
        webvisor?: boolean;
        trackHash?: boolean;
        ut?: 'noindex';
    }

    export class BStatcounter extends Component<{
        metrika?: {
            type?: 'js';
            counter: MetrikaCounter | MetrikaConfig;
        };
    }> {}

    export interface RadioBoxRadioProps {
        value?: string | number;
    }

    export class Header2 extends Component {
        public static Main: typeof Component;
        public static Right: typeof Component;
        public static Logo: typeof HeaderLogo;
    }

    export interface TextInputProps {
        showListOnFocus?: boolean;
        suggestUrl?: string;
        updateOnEnter?: boolean;
        zIndexGroupLevel?: number;
        suggest?: boolean;
        popupProps?: object;
    }
}
