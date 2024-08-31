import url from 'url';
import { Request } from 'express';
import React, { FC } from 'react';
// @ts-ignore
import cssFontLib from '!!raw-loader!yandex-font/build/static/browser.css';
import yandexFont from 'yandex-font/build/meta';
import { cnTheme } from '@yandex-lego/components/Theme';
import { theme } from '@yandex-lego/components/Theme/presets/default';
import { Logoaas } from '@yandex-lego/components/Header/';
// @ts-ignore
import User2Vanilla from '@yandex-lego/serp-header/dist/base/user2.desktop';
import { getStaticPath } from '@yandex-lego/serp-header/staticPath';
import routes from '../lib/routes';
import config from '../lib/config';
import App, { className as b, CustomerAppProps } from '../../client/pages/Customer';
import { CustomerSSRProps } from '../../client/customer';

const CustomerView: FC<{ req: Request }> = ({ req }) => {
    const isFontCookieCorrected = req.cookies[yandexFont.fontCookieName] === yandexFont.fontVersion;
    const className = isFontCookieCorrected ? yandexFont.fontLoadedClassName : '';
    const includeFontScript = !req.cookies || !isFontCookieCorrected;

    const ssrProps: CustomerSSRProps = {
        user: req.blackbox?.uid
            ? {
                  uid: req.blackbox.uid,
                  login: req.blackbox.login,
                  avatar: req.blackbox.avatar,
                  displayName: req.blackbox.displayName,
                  havePlus: req.blackbox.havePlus,
              }
            : undefined,
        yu: req.cookies.yandexuid,
        secretkey: req.secretkey!,
        config: {
            passportHost: url.format(config.passport),
            passportAccounts: url.format(config.passportAccounts),
            avatarHost: url.format(config.avatar),

            apiRoot: config.app.apiProxy,
            urlRoot: config.app.urlRoot,
            assetsRoot: config.app.assetsRoot,

            pollTimeout: config.app.pollInterval,
        },
    };

    const props: CustomerAppProps = {
        isSSR: true,
        ...ssrProps,
        retpath: url.format({
            protocol: 'https',
            host: req.get('host'),
            pathname: req.originalUrl,
        }),
        activationCode: (Array.isArray(req.query.code) ? req.query.code[0] : req.query.code) || '',
        activationId:
            (Array.isArray(req.query.activationId)
                ? req.query.activationId[0]
                : req.query.activationId) || '',
        isManualOpen: req.cookies.manualOpen !== 'false',
    };

    return (
        <html className={className}>
            <head>
                <meta charSet='utf-8' />
                <title>Алиса в бизнесе</title>
                <meta
                    name='viewport'
                    content='width=device-width, initial-scale=1.0, user-scalable=no'
                />
                <meta name='referrer' content='no-referrer' />
                <link rel='shortcut icon' href={routes.assets('favicon.ico')} />
                <link
                    rel='shortcut icon'
                    type='image/png'
                    sizes='16x16'
                    href={routes.assets('favicon-16x16.png')}
                />
                <link
                    rel='shortcut icon'
                    type='image/png'
                    sizes='32x32'
                    href={routes.assets('favicon-32x32.png')}
                />

                <meta property='og:url' content='https://dialogs.yandex.ru/b2b/customer' />
                <meta property='og:type' content='website' />
                <meta property='og:title' content='Активируйте станцию в номере' />
                <meta property='og:image' content={routes.assets('og.png')} />
                <meta
                    property='og:description'
                    content='Активируйте Яндекс.Станцию собственным аккаунтом, чтобы получить персональный контент'
                />
                <meta
                    name='twitter:description'
                    content='Активируйте Яндекс.Станцию собственным аккаунтом, чтобы получить персональный контент'
                />
                <meta name='twitter:image' content={routes.assets('og.png')} />

                <link rel='stylesheet' href={routes.assets('css/customer.css')} />
                <style
                    dangerouslySetInnerHTML={{
                        __html: cssFontLib,
                    }}
                />
            </head>

            <body className={cnTheme(theme)}>
                <div className={b()}>
                    <div className={b('Header')}>
                        <div className={b('HeaderLogo')}>
                            <Logoaas color="000" size={27} />
                        </div>
                        {props.user && (
                            <div
                                className={b('HeaderUser')}
                                dangerouslySetInnerHTML={{
                                    __html: User2Vanilla.getContent({
                                        tld: 'ru',
                                        lang: 'ru',
                                        key: 'base',
                                        ctx: {
                                            tld: 'ru',
                                            lang: 'ru',
                                            bundleKey: 'base',

                                            passportHost: props.config.passportHost,
                                            accountsUrl: props.config.passportAccounts,
                                            avatarHost: props.config.avatarHost,

                                            noCounter: true,
                                            uid: props.user.uid!,
                                            yu: props.yu,
                                            name: props.user.displayName,
                                            avatarId: props.user.avatar!.default,
                                            secretKey: props.secretkey,

                                            retpath: props.retpath,
                                        },
                                    }),
                                }}
                            />
                        )}
                    </div>
                    <div className={b('Content')} id='app' data-props={JSON.stringify(ssrProps)}>
                        <App {...props} />
                    </div>
                </div>

                <script src={routes.assets('js/customer.js')} />
                <script
                    src={getStaticPath({
                        key: 'base',
                        platform: 'desktop',
                        service: 'base',
                        block: 'user2',
                    })}
                />
                {includeFontScript && (
                    <script
                        defer
                        src={yandexFont.staticPaths['browser.js']}
                    />
                )}
            </body>
        </html>
    );
};

export default CustomerView;
