import config from './config';

const rtrim = (char: string, url: string) => {
    return url.endsWith(char) ? url.slice(0, -1) : url;
};

const assetsRoot = rtrim('/', config.app.assetsRoot);

export default {
    assets: (asset: string) => `${assetsRoot}/assets/${asset}`,
};
