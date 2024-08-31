import React, { useCallback, useEffect } from 'react';
import { observer } from 'mobx-react';
import url from 'url';
import { cn } from '@bem-react/classname';
import { Text } from '@yandex-lego/components/Text/bundle'
import { Icon } from '@yandex-lego/components/Icon/bundle'
import { TooltipStateful } from '@yandex-lego/components/Tooltip/desktop/bundle'

import { metrica } from '../../utils/metrica';
import { ERRORS } from '../../utils/errors';
import API from '../../lib/api';
import { useRoutes } from '../../context/routes';
import { Code } from '../../components/Customer/Code'
import { Login } from '../../components/common/Login'
import { useCustomerApi } from '../../context/customer-api';

import { useCustomerAppCtx } from './index';
import { STEPS, useCustomerPageCtx } from './CustomerPage';
import './Activate.scss'

const CODE_VALIDATION_ERROR = 422;

const b = cn('Activate');

const removeCodeFromQuery = () => {
    try {
        const query = url.parse(location.search || '?', true).query;
        delete query.code;
        delete query.activationId;
        history.replaceState(
            null,
            document.title,
            url.format({
                pathname: location.pathname,
                query,
            }),
        );
    } catch (e) {
        console.warn(e);
    }
};

export const Activate = observer(() => {
    const routes = useRoutes();

    const api = useCustomerApi()
    const { user } = useCustomerAppCtx();
    const { activationStore, deviceStore, ...pageState } = useCustomerPageCtx();

    const isLogin = Boolean(user);

    useEffect(() => {
        metrica.action('show_code', isLogin);
    }, []);

    const onSubmitCallback = useCallback(async (code) => {
        metrica.action('submit_code', isLogin);

        const activationCode = code.join('');

        try {
            pageState.setError(undefined)
            const [ok, kolonkishId] = await activationStore.activate(activationCode, isLogin);

            if (!ok) {
                return;
            }

            if(!isLogin && kolonkishId) {
                metrica.puid(kolonkishId)
            }
        } catch (error) {
            let errorObj;
            if(error.payload && error.payload.text) {
                if (error.payload.text === 'Код активации устарел. Пожалуйста, обновите страницу и попробуйте еще раз.') {
                    errorObj = { text: 'Код введён с ошибкой. Повторите попытку', status: ERRORS.WRONG_CODE }
                } else {
                    errorObj = { text: error.payload.text, status: ERRORS.CUSTOM }
                }
            } else {
                if (error.code === CODE_VALIDATION_ERROR) {
                    errorObj = { text: 'Код введён с ошибкой. Повторите попытку', status: ERRORS.WRONG_CODE }
                } else {
                    metrica.error('Unexpected_error', error.code);
                    errorObj = { text: 'Произошла ошибка. Обратитесь на ресепшен', status: ERRORS.FATAL }
                }
            }

            pageState.setError(errorObj)
            return;
        } finally {
            removeCodeFromQuery()
        }

        try {
            if(isLogin && user && !user.havePlus) {
                const promocodeOk = await api.applyPromocode();
                if (!promocodeOk) {
                    return;
                }
            } else {
                pageState.setStep(STEPS.Success);
            }
        } catch (error) {
            let errorObj;
            if (error.message !== 'Promo code has already been redeemed') {
                if (API.is4xxError(error)) {
                    errorObj = {
                        text: 'Не удалось активировать Яндекс.Плюс. Доступ к контенту ограничен. Обратитесь на ресепшен за помощью.',
                        status: ERRORS.INFO
                    }
                } else {
                    errorObj = { text: 'Произошла ошибка. Обратитесь на ресепшен', status: ERRORS.FATAL }
                }
            }
            pageState.setError(errorObj)
        }
    }, []);

    return (
        <div className={b()}>
            <div className={b('Title')}>
                <Text typography="headline-l" weight="medium" className={b('Title-Text')}>
                    Введите код
                </Text>
                <span>
                    <Text typography="body-short-m">
                        Который продиктовала Алиса
                    </Text>

                    <TooltipStateful
                        hasTail
                        trigger="click"
                        direction="bottom-end"
                        className={b('Tooltip')}
                        secondaryOffset={38}
                        size="m"
                        content={
                            <div className={b('Tooltip-Content')}>
                                <Text typography={"headline-xs"} weight={"medium"} color={"inverse"}>
                                    Попросите Алису назвать код
                                </Text>
                                <Text typography={"subheader-s"} color={"inverse"}>
                                    {
                                        "Например так:\n" +
                                        "— Алиса, какой пароль?\n" +
                                        "— Алиса, какой код активации\n" +
                                        "— Алиса, повтори код"
                                    }
                                </Text>
                            </div>
                        }
                    >
                        <span>
                              <Icon
                                  className={b('Help')}
                                  url={routes.assets('images/help.svg')}
                                  style={{ width: 16, height: 16 }}
                              />
                        </span>
                    </TooltipStateful>
                </span>
            </div>
            <Code onSubmitCallback={onSubmitCallback} className={b('Code')}/>
            {!isLogin && <Login />}
        </div>
    )
});
