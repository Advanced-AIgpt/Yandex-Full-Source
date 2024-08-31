import React from 'react';
import routes from '../lib/routes';
import config from '../lib/config';
import pick from 'lodash/pick';
import { AppProps } from '../../client/blocks/App/App';

const pickUser = (user: ExpressBlackbox.User) => pick(user, ['displayName', 'avatar', 'uid', 'login']);

export default function IndexView(props: { blackbox: ExpressBlackbox.Result; yu: string; secretkey: string }) {
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

                <link rel='stylesheet' href={routes.assets('css/app.css')} />
                <script src={routes.assets('js/app.js')} />
            </head>

            <body>
                <div
                    id='app'
                    data-props={JSON.stringify({
                        user: { ...pickUser(props.blackbox), connect: props.blackbox.connect },
                        users: (props.blackbox.users || []).filter((user) => user.uid != null).map(pickUser),
                        yu: props.yu,
                        secretkey: props.secretkey,
                        config: config.client(),
                    } as AppProps)}
                />
            </body>
        </html>
    );
}
