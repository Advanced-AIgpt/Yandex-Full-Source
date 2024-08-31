import { Avatar } from './avatar';

describe('Testing Avatar class', () => {
    it('testing basic functional', () => {
        expect(
            Avatar
                .fromUrl('https://avatars-int.mdst.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/200x200?1641982016228'),
        )
            .toEqual({
                domain: 'avatars-int.mdst.yandex.net',
                groupId: '27701',
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '200x200',
                parameters: '?1641982016228',
                protocol: 'https',
            });

        expect(
            Avatar
                .fromUrl('https://avatars-int.mds.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/200x200?1641982016228'),
        )
            .toEqual({
                domain: 'avatars-int.mds.yandex.net',
                groupId: '27701',
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '200x200',
                parameters: '?1641982016228',
                protocol: 'https',
            });

        expect(
            Avatar
                .fromUrl('https://avatars.mds.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/200x200?1641982016228'),
        )
            .toEqual({
                domain: 'avatars.mds.yandex.net',
                groupId: '27701',
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '200x200',
                parameters: '?1641982016228',
                protocol: 'https',
            });

        expect(
            Avatar
                .fromUrl('https://avatars.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/200x200?1641982016228'),
        )
            .toEqual({
                domain: 'avatars.yandex.net',
                groupId: '27701',
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '200x200',
                parameters: '?1641982016228',
                protocol: 'https',
            });

        expect(
            Avatar
                .fromUrl('http://avatars.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/200x200?1641982016228'),
        )
            .toEqual({
                domain: 'avatars.yandex.net',
                groupId: '27701',
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '200x200',
                parameters: '?1641982016228',
                protocol: 'http',
            });

        expect(
            Avatar
                .fromUrl('avatars.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/200x200?1641982016228'),
        )
            .toEqual({
                domain: 'avatars.yandex.net',
                groupId: '27701',
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '200x200',
                parameters: '?1641982016228',
                protocol: 'https',
            });

        expect(
            Avatar
                .fromUrl('avatars.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/200x200?'),
        )
            .toEqual({
                domain: 'avatars.yandex.net',
                groupId: '27701',
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '200x200',
                parameters: '?',
                protocol: 'https',
            });

        expect(
            Avatar
                .fromUrl('avatars.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/200x200'),
        )
            .toEqual({
                domain: 'avatars.yandex.net',
                groupId: '27701',
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '200x200',
                parameters: '',
                protocol: 'https',
            });

        expect(
            (new Avatar({
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '200x200',
                groupId: '27701',
            })),
        )
            .toEqual({
                domain: 'avatars.yandex.net',
                groupId: '27701',
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '200x200',
                parameters: '',
                protocol: 'https',
            });

        expect(
            Avatar
                .fromUrl('avatars.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/'),
        )
            .toEqual({
                domain: 'avatars.yandex.net',
                groupId: '27701',
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '',
                parameters: '',
                protocol: 'https',
            });

        expect(
            Avatar
                .fromUrl('avatars.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/?123'),
        )
            .toEqual({
                domain: 'avatars.yandex.net',
                groupId: '27701',
                imageId: 'r5ldfjP1rJoson',
                namespace: 'music-user-playlist',
                typeName: '',
                parameters: '?123',
                protocol: 'https',
            });

        expect((new Avatar({
            domain: 'avatars.yandex.net',
            groupId: '27701',
            imageId: 'r5ldfjP1rJoson',
            namespace: 'music-user-playlist',
            typeName: 'mytype',
            parameters: '?123',
        })).setTypeName('newtype').toString()).toBe('https://avatars.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/newtype?123');

        expect((new Avatar({
            domain: 'avatars.yandex.net',
            groupId: '27701',
            imageId: 'r5ldfjP1rJoson',
            namespace: 'music-user-playlist',
            typeName: 'mytype',
            parameters: '?123',
            protocol: 'http',
        })).setTypeName('newtype').toString()).toBe('http://avatars.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/newtype?123');

        expect((new Avatar({
            domain: 'avatars.yandex.net',
            groupId: '27701',
            imageId: 'r5ldfjP1rJoson',
            namespace: 'music-user-playlist',
            typeName: 'mytype',
            parameters: '?123',
            protocol: 'https',
        })).setTypeName('newtype').toString()).toBe('https://avatars.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/newtype?123');

        expect(Avatar.fromUrl('http://not-avatar-link.yandex.net/get-music-user-playlist/27701/r5ldfjP1rJoson/newtype'))
            .toBeNull();
    });

    describe('setImageSize', () => {
        describe('altay namespace', () => {
            it('default', () => {
                expect(Avatar.setImageSize({
                    namespace: 'get-altay',
                    size: 'largeRectangle',
                    data: 'https://avatars.mds.yandex.net/get-altay/1899727/2a0000016a6a0741d0c2f55bd2edc9494d6c/orig',
                }))
                    .toBe('https://avatars.mds.yandex.net/get-altay/1899727/2a0000016a6a0741d0c2f55bd2edc9494d6c/h336');
            });

            it('namespace recognition', () => {
                expect(Avatar.setImageSize({
                    size: 'largeRectangle',
                    data: 'https://avatars.mds.yandex.net/get-altay/1899727/2a0000016a6a0741d0c2f55bd2edc9494d6c/orig',
                }))
                    .toBe('https://avatars.mds.yandex.net/get-altay/1899727/2a0000016a6a0741d0c2f55bd2edc9494d6c/h336');
            });
        });

        describe('i namespace', () => {
            it('default', () => {
                expect(Avatar.setImageSize({
                    namespace: 'i',
                    size: 'largeRectangle',
                    data: 'https://avatars.mds.yandex.net/i?id=1d6501959db11ac59cfb3879507fc850-5668465-images-thumbs&ref=oo_serp',
                }))
                    .toBe('https://avatars.mds.yandex.net/i?id=1d6501959db11ac59cfb3879507fc850-5668465-images-thumbs&ref=oo_serp&n=33&h=344');
            });

            it('namespace recognition', () => {
                expect(Avatar.setImageSize({
                    size: 'largeRectangle',
                    data: 'https://avatars.mds.yandex.net/i?id=1d6501959db11ac59cfb3879507fc850-5668465-images-thumbs&ref=oo_serp',
                }))
                    .toBe('https://avatars.mds.yandex.net/i?id=1d6501959db11ac59cfb3879507fc850-5668465-images-thumbs&ref=oo_serp&n=33&h=344');
            });
        });

        describe('entity_search namespace', () => {
            it('default', () => {
                expect(Avatar.setImageSize({
                    namespace: 'get-entity_search',
                    size: 'largeRectangle',
                    data: 'https://avatars.mds.yandex.net/get-entity_search/122335/157750127/S60x60',
                }))
                    .toBe('https://avatars.mds.yandex.net/get-entity_search/122335/157750127/S284x160');
            });

            it('namespace recognintion', () => {
                expect(Avatar.setImageSize({
                    size: 'largeRectangle',
                    data: 'https://avatars.mds.yandex.net/get-entity_search/122335/157750127/S60x60',
                }))
                    .toBe('https://avatars.mds.yandex.net/get-entity_search/122335/157750127/S284x160');
            });

            it('type', () => {
                expect(Avatar.setImageSize({
                    namespace: 'get-entity_search',
                    size: 'largeRectangle',
                    type: 'Face',
                    data: 'https://avatars.mds.yandex.net/get-entity_search/122335/157750127/S60x60',
                }))
                    .toBe('https://avatars.mds.yandex.net/get-entity_search/122335/157750127/S284x160Face');
            });
        });
    });
});
