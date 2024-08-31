import React, { createContext, useContext, useEffect, useState } from 'react';
import { YMInitializer } from 'react-yandex-metrika';
import { cn } from '@bem-react/classname';

import { METRIKA_ID } from '../../utils/metrica';
import RoutesProvider from '../../context/routes';
import CustomerApiProvider from '../../context/customer-api';
import { CustomerPage } from './CustomerPage';
import '../common.scss';

const b = cn('App');
export const className = b;

export interface CustomerAppProps {
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
    isSSR: boolean;
    retpath: string;
    activationCode?: string;
    activationId?: string;
    isManualOpen?: boolean;
}

const CustomerAppCtx = createContext<CustomerAppProps>({} as CustomerAppProps);

export const CustomerAppCtxProvider = CustomerAppCtx.Provider;
export const useCustomerAppCtx = () => useContext(CustomerAppCtx);

export default (props: CustomerAppProps) => {
    const [isSSR, setSSR] = useState(true);
    useEffect(() => {
        if (!props.isSSR) {
            setSSR(false);
        }
    });

    return (
        <>
            <YMInitializer accounts={[METRIKA_ID]} options={{webvisor: true}}/>
            <RoutesProvider
                appUrl={props.config.urlRoot}
                assetsUrl={props.config.assetsRoot}
                apiUrl={props.config.apiRoot}
                connectUrl=''
                secretkey={props.secretkey}
            >
                <CustomerApiProvider
                    rootUrl={props.config.apiRoot}
                    secretkey={props.secretkey}
                    pollingInterval={props.config.pollTimeout}
                >
                    <CustomerAppCtxProvider value={{ ...props, isSSR }}>
                        <CustomerPage />
                    </CustomerAppCtxProvider>
                </CustomerApiProvider>
            </RoutesProvider>
        </>
    );
};
