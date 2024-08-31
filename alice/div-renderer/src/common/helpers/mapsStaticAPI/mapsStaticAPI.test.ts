import MapsStaticAPI from './mapsStaticAPI';

const apiKey = 'apiKey';
const stringSecret = 'c3RyaW5nU2VjcmV0';

describe('Check Maps Static API', () => {
    it('Normal use', () => {
        expect(
            (new MapsStaticAPI(
                new URL('https://static-maps.yandex.ru/1.x/?l=map&ll=30.315868,59.939095&z=8'),
                apiKey,
                stringSecret,
            ))
                .toString(),
        )
            .toBe('https://static-maps.yandex.ru/1.x/?l=map&ll=30.315868%2C59.939095&z=8&api_key=apiKey&signature=fm1YtkswCCGmGtJFKUbGU4GIuzhPwIvWdL1u3lgYPLg%3D');
    });

    it('Use with parameters', () => {
        expect(
            (new MapsStaticAPI(
                new URL('https://static-maps.yandex.ru/1.x/?l=map&ll=30.315868,59.939095&z=8'),
                apiKey,
                stringSecret,
            ))
                .deleteLogoAndCopy()
                .setTheme('dark')
                .toString(),
        )
            .toBe('https://static-maps.yandex.ru/1.x/?l=map&ll=30.315868%2C59.939095&z=8&api_key=apiKey&cr=0&lg=0&theme=dark&signature=ghHvac8LAJdPnve_vLGZO3Yq0GS1OtgIZ4XYU5rjQBI%3D');
    });

    it('Few usages', () => {
        const map = (new MapsStaticAPI(
            new URL('https://static-maps.yandex.ru/1.x/?l=map&ll=30.315868,59.939095&z=8'),
            apiKey,
            stringSecret,
        ))
            .deleteLogoAndCopy()
            .setTheme('dark');
        expect(
            map.toString(),
        )
            .toBe(map.toString());
    });
});

