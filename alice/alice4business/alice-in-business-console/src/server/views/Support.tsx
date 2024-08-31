import React, { FC } from 'react';
import { Request } from 'express';

import routes from '../lib/routes';
import config from '../lib/config';
import pick from 'lodash/pick';
import { SupportAppProps, className as b } from '../../client/pages/Support';
import { Logoaas } from "@yandex-lego/components/Header";
import { cnTheme } from '@yandex-lego/components/Theme';
import { theme } from '@yandex-lego/components/Theme/presets/default';
// @ts-ignore
import User2Vanilla from '@yandex-lego/serp-header/dist/base/user2.desktop';
import { SupportSSRProps } from '../../client/support';
import url from "url";


const pickUser = (user: ExpressBlackbox.User) => pick(user, ['displayName', 'avatar', 'uid', 'login']);

const SupportView: FC<{ req: Request }> = ({ req }) => {
    const props: SupportSSRProps = {
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

            apiRoot: config.app.apiSupportProxy,
            urlRoot: config.app.urlRoot,
            assetsRoot: config.app.assetsRoot,

            pollTimeout: config.app.pollInterval,
        },
    };

    return (
        <html>
        <head>
            <meta charSet='utf-8' />
            <title>Алиса в бизнесе</title>
            <meta name='viewport' content='width=device-width, initial-scale=1' />
            <meta name='referrer' content='no-referrer' />
            <link rel='shortcut icon' href={routes.assets('favicon.ico')} />
            <link rel='shortcut icon' type='image/png' sizes='16x16' href={routes.assets('favicon-16x16.png')} />
            <link rel='shortcut icon' type='image/png' sizes='32x32' href={routes.assets('favicon-32x32.png')} />

            <meta property='og:url' content='https://dialogs.yandex.ru/b2b' />
            <meta property='og:type' content='website' />
            <meta property='og:title' content='Интегрируйте Алису с вашим бизнесом' />
            <meta property='og:image' content={routes.assets('og.png')} />
            <meta
                property='og:description'
                content='Яндекс.Станция в роли консьержа, управление устройствами умного дома в номере и многое другое'
            />
            <meta
                name='twitter:description'
                content='Яндекс.Станция в роли консьержа, управление устройствами умного дома в номере и многое другое'
            />
            <meta name='twitter:image' content={routes.assets('og.png')} />

            <link rel='stylesheet' href={routes.assets('css/support.css')} />
            <script src={routes.assets('js/support.js')} />
        </head>
        <body className={cnTheme(theme)}>
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
                            },
                        }),
                    }}
                />
            )}
        </div>
        <div
            id='app'
            data-props={JSON.stringify({
                user: { ...pickUser(req.blackbox!), connect: req.blackbox!.connect },
                users: (req.blackbox!.users || []).filter((user) => user.uid != null).map(pickUser),
                yu: props.yu,
                secretkey: props.secretkey,
                config: props.config,
            } as Partial<SupportAppProps>)}
        />
        </body>
        </html>
    );
}

export default SupportView;

