import React from 'react';
import { cn } from '@bem-react/classname';
import { Text } from '@yandex-lego/components/Text/touch-pad/bundle'
import { Button } from '@yandex-lego/components/Button/touch-pad/bundle'

import { useCustomerAppCtx } from '../../../pages/Customer';

import './styles.scss';

const b = cn('Login');

export const Login = () => {
    const appContext = useCustomerAppCtx();

    return (
        <div className={b()}>
            <div className={b('Content')}>
                <Text typography="body-short-xl" weight="medium" color="inverse">
                    Войдите в аккаунт
                </Text>
                <Text typography="body-short-l" color="inverse" className={b('Content-Subtitle')}>
                    Так Алиса сможет включать вашу любимую музыку и&nbsp;запоминать&nbsp;лайки
                </Text>
                <Button
                    size="m"
                    view="action"
                    type="link"
                    url={appContext.config.passportHost + '/auth?retpath=' + appContext.retpath}
                >
                    Войти
                </Button>
            </div>
        </div>
    )
}
