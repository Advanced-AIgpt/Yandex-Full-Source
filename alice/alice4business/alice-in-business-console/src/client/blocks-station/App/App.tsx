import React, { createContext, FC, useCallback, useContext, useEffect, useState } from 'react';
import { cn } from '@bem-react/classname';
import qrcodeGenerator from 'qrcode-generator';
import './App.scss';

const b = cn('App');

const formatCode = (code: string) => {
    if(code.length >= 9) {
        return code
            .split('')
            .map((s, i) => (i > 0 && i % 3 === 0 ? '–' + s : s))
            .join('');
    } else {
        return code;
    }
}

const App: FC = () => {
    const appCtx = useStationAppCtx();

    const customerPage = `https://${appCtx.config.host}${appCtx.config.urlRoot}/customer`;

    const getQrCode = useCallback((code: string) => {
        const qr = qrcodeGenerator(0, 'M');

        qr.addData(`${customerPage}?code=${code}`);
        qr.make();

        return qr.createSvgTag({ cellSize: 1, margin: 1, scalable: true });
    }, []);

    const [activationCode, setActivationCode] = useState(appCtx.activationCode || '0000');
    const [qrCode, _setQrCode] = useState(() => getQrCode(activationCode));

    useEffect(() => {
        _setQrCode(getQrCode(activationCode));

        const quasar = window?.quasar;
        if (quasar) {
            quasar.setState({
                alice4business: true,
                currentScreen: 'device_lock',
                activationCode,
            });
        }
    }, [activationCode]);

    useEffect(() => {
        const quasar = window?.quasar;
        if (quasar) {
            quasar.pageLoaded();

            let interval: number;

            const getCode = () => {
                quasar.callServer('alice4business:get_code', {}).catch(() => {});
            };

            const onCodeUpdate = (event: quasar.NativeCommandEvent) => {
                if (event.type !== 'alice4business:set_code') {
                    return;
                }

                let payload: quasar.SetCodeNativeCommandEventPayload | undefined;
                try {
                    payload = JSON.parse(event?.detail?.meta ?? '');
                } catch (e) {
                    console.warn('Failed to parse event payload', e);
                    return;
                }

                payload?.code && setActivationCode(payload.code);

                clearInterval(interval);
                interval = window.setInterval(getCode, appCtx.config.codeRefreshInterval);
            };

            quasar.setAvailableNavDirections({ up: false, right: false, down: false, left: false });
            interval = window.setInterval(getCode, appCtx.config.codeRefreshInterval);
            quasar.onCommand('alice4business:set_code', onCodeUpdate);

            quasar.ready();

            return () => {
                clearInterval(interval);
                quasar.offCommand('alice4business:set_code', onCodeUpdate);
            };
        }
    }, []);

    return (
        <div className={b()}>
            <div className={b('Content')}>
                <h1>Разблокируйте Яндекс.Станцию</h1>
                <ol>
                    <li>
                        Сканируйте QR-код или перейдите по ссылке <a>ya.cc/hotel-login</a>
                    </li>
                    <li>Введите на открывшейся странице пароль:</li>
                </ol>
            </div>

            <h2 className={b('Code', {Length: activationCode.length >= 9 ? 'Long' : 'Short'})}>
                {formatCode(activationCode)}
            </h2>

            <div className={b('QR')} dangerouslySetInnerHTML={{ __html: qrCode }} />
        </div>
    );
};

export interface StationAppProps {
    activationCode?: string;
    config: {
        urlRoot: string;
        assetsRoot: string;
        host: string;
        codeRefreshInterval: number;
    };
}

const StationAppCtx = createContext<StationAppProps>({} as any);

export const useStationAppCtx = () => useContext(StationAppCtx);

export default (props: StationAppProps) => {
    return (
        <StationAppCtx.Provider value={props}>
            <App />
        </StationAppCtx.Provider>
    );
};
