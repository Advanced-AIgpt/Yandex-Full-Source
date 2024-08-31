import React, { useCallback, useEffect } from 'react';
import { cn } from '@bem-react/classname';
import { Text } from '@yandex-lego/components/Text/bundle';
import { Button } from '@yandex-lego/components/Button/touch-pad/bundle';
import { Image } from '@yandex-lego/components/Image/touch-pad/bundle';

import { metrica } from '../../utils/metrica';
import { ERRORS } from '../../utils/errors';

import { useCustomerPageCtx } from './CustomerPage';
import { useCustomerAppCtx } from './index';
import './Success.scss'

const b = cn('Success');

export const Success = () => {
    const { deviceStore, setError } = useCustomerPageCtx();
    const { user } = useCustomerAppCtx();

    const onClick = useCallback(async () => {
        if (deviceStore.devices?.length > 0) {
            try {
                await deviceStore.reset(deviceStore.devices[0].deviceId)
            } catch {
                const errorObj = { text: 'Произошла ошибка. Обратитесь на ресепшен', status: ERRORS.FATAL }
                setError(errorObj)
            }
        }
    }, [deviceStore.reset, deviceStore.devices])

    const imgUrl = deviceStore.devices?.length > 0 ? deviceStore.devices[0]?.platform.substr(6) : 'station_2'

    useEffect(() => {
        metrica.action('show_success', Boolean(user));
    }, []);

    return (
        <div className={b()}>
            <div className={b('Title')}>
                <Text typography="headline-l" weight="medium">
                    Станция готова к&nbsp;использованию!
                </Text>
            </div>
            <ul className={b('Messages')}>
                <li>
                    Алиса, включи музыку
                </li>
                <li>
                    Алиса, разбуди меня в 8 утра
                </li>
                <li>
                    Алиса, сегодня будет дождь?
                </li>
            </ul>
            {
                deviceStore.devices.length > 0 && (
                    <div className={b('Footer')}>
                        <Text typography="body-short-xl" color="promo">
                            Когда вы будете покидать отель
                        </Text>
                        <Text typography="body-short-l" className={b('Footer-Subtitle')} color="promo">
                            Мы сами отвяжем станцию от аккаунта
                        </Text>
                        <Button
                            size="m"
                            view="default"
                            onClick={onClick}
                        >
                            Отвязать сейчас
                        </Button>
                    </div>
                )
            }
            <Image
                fallbackSrc={`https://yastatic.net/s3/dialogs/b2b/customer/images/station_2.png`}
                src={`https://yastatic.net/s3/dialogs/b2b/customer/images/${imgUrl}.png`}
                className={b('Image')}/>
        </div>
    )
}
