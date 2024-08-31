import React, { FC } from 'react';
import routes from '../lib/routes';
import config from '../lib/config';
import App, { StationAppProps } from '../../client/blocks-station/App/App';
import { StationSSRProps } from '../../client/station';

interface Props {
    activationCode?: string;
    host: string;
}

const StationView: FC<Props> = ({ activationCode, host }) => {
    const ssrProps: StationSSRProps = {
        assetsRoot: config.app.assetsRoot,
        urlRoot: config.app.urlRoot,
        codeRefreshInterval: config.app.activationCodeRefreshInterval,
    };

    const props: StationAppProps = {
        activationCode,
        config: {
            ...ssrProps,
            host,
        },
    };

    return (
        <html>
            <head>
                <meta charSet='utf-8' />
                <title>Станция заблокирована</title>
                <meta name='viewport' content='width=device-width, minimum-scale=1.0' />
                <meta name='referrer' content='no-referrer' />
                <link rel='shortcut icon' href={routes.assets('favicon.ico')} />
                <link rel='shortcut icon' type='image/png' sizes='16x16' href={routes.assets('favicon-16x16.png')} />
                <link rel='shortcut icon' type='image/png' sizes='32x32' href={routes.assets('favicon-32x32.png')} />

                <link rel='stylesheet' href={routes.assets('css/station.css')} />
            </head>

            <body>
                <div id='app' data-props={JSON.stringify(ssrProps)}>
                    <App {...props} />
                </div>
                <script src={routes.assets('js/station.js')} />
            </body>
        </html>
    );
};

export default StationView;
