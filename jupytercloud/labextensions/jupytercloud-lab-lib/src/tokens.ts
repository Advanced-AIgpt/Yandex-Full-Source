import { Token } from '@lumino/coreutils';
import { Menu } from '@lumino/widgets';

import { NotebookPanel } from '@jupyterlab/notebook';

export const IJupyterCloud = new Token<IJupyterCloud>('IJupyterCloud');

export interface IJupyterCloud {
    menu: Menu;
    isHubAvailable: (extensionName: string) => boolean;
    getHubUrl: (...parts: string[]) => string;
}

export const IArcadiaShare = new Token<IArcadiaShare>('IArcadiaShare');

export interface IShare {
    panel: NotebookPanel;
    showSuccessDialog?: boolean;
}

export type TShareFunc = ({ panel, showSuccessDialog }: IShare) => Promise<any>;

export interface IArcadiaShare {
    share: TShareFunc;
}

export type TUserParams = {
    [param: string]: string | number | boolean | null | TUserParams;
};
export type TSessionParams = {
    [param: string]:
        | string
        | number
        | boolean
        | null
        | TSessionParams
        | TSessionParams[];
};

export interface IReachGoal {
    target: string;
    sessionParams?: TSessionParams;
    callback?: () => void;
    ctx?: any;
}
export const IYandexMetrika = new Token<IYandexMetrika>('IYandexMetrika');
export interface IYandexMetrika {
    reachGoal: ({ target, sessionParams, callback, ctx }: IReachGoal) => void;
    setUserParams: (userParams: TUserParams) => void;
    setSessionParams: (sessionParams: TSessionParams) => void;
}
