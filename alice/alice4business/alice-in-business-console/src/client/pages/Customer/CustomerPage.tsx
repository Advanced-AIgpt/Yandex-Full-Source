import React, { createContext, Dispatch, SetStateAction, useContext, useEffect, useState } from 'react';
import { cn } from '@bem-react/classname';
import { observer, useLocalStore } from 'mobx-react';
import { Spin } from '@yandex-lego/components/Spin/touch-pad/bundle'
import { MessageBox } from "@yandex-lego/components/MessageBox/touch-pad/bundle";

import { CustomerDeviceListStore } from '../../store/customer-device-list';
import { ActivationDataStore } from '../../store/activation-data';
import { useCustomerApi } from '../../context/customer-api';
import { ERRORS, INPUT_ERRORS } from '../../utils/errors';

import { useCustomerAppCtx } from './index';
import { Activate } from './Activate';
import { Success } from './Success';
import './CustomerPage.scss'

export enum STEPS {
    Activate,
    Success
}

export type ErrorType = { text: string, status: ERRORS } | undefined;

export interface CustomerPageProps {
    error?: ErrorType;
    setError: Dispatch<SetStateAction<ErrorType>>;
    setStep: Dispatch<SetStateAction<STEPS>>;
    deviceStore: CustomerDeviceListStore;
    activationStore: ActivationDataStore;
}

const CustomerPageCtx = createContext<CustomerPageProps>({} as any);
export const CustomerPageCtxProvider = CustomerPageCtx.Provider;
export const useCustomerPageCtx = () => useContext(CustomerPageCtx);

const b = cn('CustomerPage');

const renderStep = (step: STEPS, loading: boolean) => {
    if (loading) {
        return <div className={b('Spin')}>
            <Spin progress view="default" size="l"/>
        </div>
    }

    switch (step) {
        case STEPS.Activate:
            return <Activate/>
        case STEPS.Success:
            return <Success/>
        default:
            return ;
    }
}

export const CustomerPage = observer(() => {
    const [step, setStep] = useState(STEPS.Activate);
    const [error, setError] = useState<ErrorType>();

    const api = useCustomerApi();
    const { activationId } = useCustomerAppCtx();
    // Initialize store and fetch active devices by puid
    const deviceStore = useLocalStore(() => new CustomerDeviceListStore(api));
    const activationStore = useLocalStore(
        () => new ActivationDataStore(api, activationId, deviceStore),
    );

    useEffect(() => {
        deviceStore.update().catch(() => {});
    }, [])

    useEffect(() => {
        if(deviceStore.devices.length > 0) {
            setStep(STEPS.Success)
        } else {
            setStep(STEPS.Activate)
        }
    }, [deviceStore.devices, deviceStore.firstLoad])

    return (
        <div className={b()}>
            <div className={b('Content')}>
                <CustomerPageCtxProvider value={{setError, error, setStep, deviceStore, activationStore}}>
                    {renderStep(step, !deviceStore.firstLoad && !deviceStore.loadError)}
                    {error && error.text && !INPUT_ERRORS.includes(error.status) && (
                        <MessageBox size="m" view="default" className={b('Message')} onClose={()=>setError(undefined)}>
                            {error.text}
                        </MessageBox>
                    )}
                    {!error && deviceStore.isDeviceReseting && (
                        <MessageBox size="m" view="default" className={b('Message')}>
                            Отключение Станции может занять пару
                            минут. В это время устройство должно быть подключено к питанию и WiFi. В случае
                            возникновения проблем обратитесь на ресепшен
                        </MessageBox>
                    ) }
                </CustomerPageCtxProvider>
            </div>
        </div>
    )
})
