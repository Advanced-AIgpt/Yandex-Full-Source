import React, { useCallback, useState } from 'react';
import { cn } from '@bem-react/classname';
import { observer } from 'mobx-react';
import { Text } from '@yandex-lego/components/Text/touch-pad/bundle';
import { Button } from '@yandex-lego/components/Button/touch-pad/bundle';
import { Checkbox } from '@yandex-lego/components/Checkbox/touch-pad/bundle';
import { Link } from '@yandex-lego/components/Link/touch-pad/bundle';

import { useCustomerAppCtx } from '../../../pages/Customer';
import { useCustomerPageCtx } from '../../../pages/Customer/CustomerPage';
import { useRoutes } from '../../../context/routes';
import { CodeInput } from '../CodeInput';

import './styles.scss'

const b = cn('Code')

const anonymousAgreements = (legal:any) => [
    (<span>
        Я&nbsp;согласен&nbsp;с&nbsp;условиями&nbsp;
        <Link href={legal.userAgreement} className={b('AgreementLink')} target="_blank" rel="noreferrer nofollow">
            Пользовательского соглашения сервисов Яндекса
        </Link>
        ,{' '}
        <Link href={legal.licenseAgreement} className={b('AgreementLink')} target="_blank" rel="noreferrer nofollow">
            Лицензионного cоглашения
        </Link>
        &nbsp;и соглашаюсь на&nbsp;обработку моих данных на&nbsp;условиях{' '}
        <Link href={legal.privacyPolicy} className={b('AgreementLink')} target="_blank" rel="noreferrer nofollow">
            Политики конфиденциальности
        </Link>.
    </span>)
]

const agreements = (legal:any) => [
    (<span>
        Я соглашаюсь с условиями{' '}
        <Link  href={legal.licenseAgreement} className={b('AgreementLink')} target="_blank" rel="noreferrer nofollow">
            Лицензионного cоглашения
        </Link>.
    </span>),
    (<span>
        Я согласен получать рекламные и информационные сообщения от ООО “Яндекс”
    </span>),
]

interface CodeProps {
    onSubmitCallback: (code: string[]) => void;
    className: string;
}

export const Code = observer(({ onSubmitCallback, className }: CodeProps) => {
    const { legal } = useRoutes();
    const { user, activationCode, activationId } = useCustomerAppCtx();
    const { activationStore } = useCustomerPageCtx();

    const [code, setCode] = useState(activationCode ? activationCode.split('') : ['','','',''])
    const [checked, setChecked] = useState([true, true])

    const isCodeCompleted = activationId ? true : code.filter((el) => el).length === 4;

    const onClick = useCallback(async () => {
        onSubmitCallback(code)
    }, [code]);

    const userAgreements = user ? agreements : anonymousAgreements;

    return (
        <div className={b('', null, [className])}>
            <CodeInput setCode={setCode} initialCode={activationId ? ['9','2','1','5'] : code}/>
            <Button
                disabled={!isCodeCompleted || checked.length !== checked.filter((el) => el).length}
                view={isCodeCompleted ? 'action' : 'default'}
                size="l"
                progress={activationStore.isActivating}
                onClick={onClick}
                className={b('Button')}
            >
                Активировать
            </Button>
            {userAgreements(legal).map((agreement, index) => (
                <span key={`agreement-${index}`} className={b('Agreement')} >
                    <Checkbox
                        size="m"
                        view="default"
                        onChange={() => setChecked((prev) => {
                            prev[index] = !prev[index]
                            return [...prev]
                        })}
                        checked={checked[index]}
                    />

                    <Text typography="body-short-s" className={b('Agreement-Text')}>
                        {agreement}
                    </Text>
                </span>

            ))}
        </div>
    )
})
