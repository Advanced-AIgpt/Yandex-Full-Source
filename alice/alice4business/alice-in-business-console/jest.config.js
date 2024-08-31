/// @ts-check
/// <reference types="jest"/>

// https://jestjs.io/docs/en/configuration.html

/** @type {jest.InitialOptions} */
const config = {
    preset: 'ts-jest',
    rootDir: 'src/test',
    moduleNameMapper: {
        '\\.(css|less|sass|scss)$': '<rootDir>/__mocks__/styleMock.ts',
    },
    setupFiles: ['<rootDir>/__setup__/index.ts'],
};

module.exports = config;
