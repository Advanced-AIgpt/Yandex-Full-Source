import { renderUpperShutter } from './upperShutter';

jest.mock('../../../../common/logger');

describe('Upper Shutter', () => {
    it('should equal snapshot default data', () => {
        expect(JSON.parse(JSON.stringify(renderUpperShutter({
            Id: 'id',
            SmartHomeData: {
                Id: 'id2',
                IoTUserData: {
                    Rooms: [
                        {
                            Id: 'r1',
                            Name: 'Комната 1',
                        },
                    ],
                    Devices: [
                        {
                            Id: 'id3',
                            IconURL: 'icon_url',
                            Name: 'Название девайса',
                            Status: 1,
                            Capabilities: [
                                {
                                    Type: 1,
                                },
                            ],
                        },
                        {
                            Id: 'id4',
                            RoomId: 'r1',
                            Capabilities: [
                                {
                                    Type: 1,
                                },
                            ],
                        },
                    ],
                },
            },
        })))).toMatchSnapshot();
    });

    it('should equal snapshot without devices', () => {
        expect(JSON.parse(JSON.stringify(renderUpperShutter({
            Id: 'id',
            SmartHomeData: {
                Id: 'id2',
                IoTUserData: {
                    Rooms: [
                        {
                            Id: 'r1',
                            Name: 'Комната 1',
                        },
                    ],
                    Devices: [],
                },
            },
        })))).toMatchSnapshot();
    });

    it('should equal snapshot without data', () => {
        expect(JSON.parse(JSON.stringify(renderUpperShutter({
            Id: 'id',
            SmartHomeData: null,
        })))).toMatchSnapshot();
    });
});
