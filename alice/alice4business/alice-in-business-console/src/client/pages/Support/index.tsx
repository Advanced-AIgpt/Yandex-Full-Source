import React, { createContext, useContext } from 'react';
import { cn } from '@bem-react/classname';
import { positions } from 'react-alert';

import SupportApiProvider from '../../context/support-api';
import RoutesProvider from '../../context/routes';
import '../common.scss';

import { SupportPage } from './SupportPage';
import { ModalWrap } from '../../utils/modals/service';
import { AlertWrap } from '../../utils/alerts/service';

const b = cn('App');
export const className = b;

export interface SupportAppProps {
    users?: Array<Partial<ExpressBlackbox.User>>;
    user?: Pick<ExpressBlackbox.User, 'uid' | 'login' | 'displayName' | 'avatar' | 'havePlus'>;
    yu: string;
    secretkey: string;
    config: {
        passportHost: string;
        passportAccounts: string;
        avatarHost: string;

        apiRoot: string;
        urlRoot: string;
        assetsRoot: string;

        pollTimeout: number;
    };
}

const SupportAppCtx = createContext<SupportAppProps>({} as SupportAppProps);

export const SupportAppCtxProvider = SupportAppCtx.Provider;
export const useSupportAppCtx = () => useContext(SupportAppCtx);

export default (props: SupportAppProps) => {
    return (
        <>
            <RoutesProvider
                appUrl={props.config.urlRoot}
                assetsUrl={props.config.assetsRoot}
                apiUrl={props.config.apiRoot}
                connectUrl=''
                secretkey={props.secretkey}
            >
                <SupportApiProvider
                    rootUrl={props.config.apiRoot}
                    secretkey={props.secretkey}
                    pollingInterval={props.config.pollTimeout}
                >
                    <SupportAppCtxProvider value={{ ...props }}>
                        <AlertWrap timeout={60000} position={positions.BOTTOM_CENTER}>
                            <ModalWrap>
                                <SupportPage />
                            </ModalWrap>
                        </AlertWrap>
                    </SupportAppCtxProvider>
                </SupportApiProvider>
            </RoutesProvider>
        </>
    );
};
