import * as CryptoJS from 'crypto-js/index';

/**
 * Class for generate link to static maps api with use api_key and string_secret
 */
export default class MapsStaticAPI {
    constructor(private url: URL, apiKey: string, private stringSecret: string) {
        // Remove deprecated parameter if exists
        this.url.searchParams.delete('key');
        this.setAPIKey(apiKey);
    }

    /**
     * You can set another api_key after create the class
     * @param apiKey new api_key or null for delete api_key
     */
    public setAPIKey(apiKey: string | null) {
        if (apiKey === null) {
            this.url.searchParams.delete('api_key');
        } else {
            this.url.searchParams.set('api_key', apiKey);
        }
        return this;
    }

    /**
     * Change or remove (if null) theme
     * @param theme
     */
    public setTheme(theme: 'dark' | null) {
        if (theme === null) {
            this.url.searchParams.delete('theme');
        } else {
            this.url.searchParams.set('theme', theme);
        }
        return this;
    }

    /**
     * Remove or add (if isNeeded is false) logo "Yandex" and copyright
     * @param isNeeded
     */
    public deleteLogoAndCopy(isNeeded = true) {
        if (isNeeded) {
            this.url.searchParams.set('cr', '0');
            this.url.searchParams.set('lg', '0');
        } else {
            this.url.searchParams.delete('cr');
            this.url.searchParams.delete('lg');
        }
        return this;
    }

    /**
     * Set image sizes in px
     * @param width
     * @param height
     */
    public setSize(width: number, height: number) {
        this.url.searchParams.set('size', `${Math.ceil(width)},${Math.ceil(height)}`);
        return this;
    }

    private static toUrlSafe(base64: string) {
        return base64.replace(/\+/g, '-').replace(/\//g, '_');
    }

    private static fromUrlSafe(base64url: string) {
        return base64url.replace(/-/g, '+').replace(/_/g, '/');
    }

    private static makeUrlToSign(url: URL) {
        return url.pathname + url.search;
    }

    private static signUrl(url: URL, signingSecretInBase64: string) {
        url.searchParams.delete('signature');
        const urlToSign = MapsStaticAPI.makeUrlToSign(url);
        const signingSecret = CryptoJS.enc.Base64.parse(MapsStaticAPI.fromUrlSafe(signingSecretInBase64));
        const hash = CryptoJS.HmacSHA256(urlToSign, signingSecret);
        const hashInBase64 = CryptoJS.enc.Base64.stringify(hash);
        url.searchParams.set('signature', MapsStaticAPI.toUrlSafe(hashInBase64));
    }

    toString() {
        MapsStaticAPI.signUrl(this.url, this.stringSecret);
        return this.url.toString();
    }
}
