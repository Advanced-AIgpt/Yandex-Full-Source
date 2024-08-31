import React, { Component } from 'react';
import { Devices } from './Devices';
import './App.css';

const POLLING_INTERVAL = 1_000;
const DEVICE_LIST_API_URL = '//example.ru/ui/devices';

export class App extends Component {
    state = {
        loaded: false,
        error: false,
        devices: [],
    };

    startPolling() {
        const poll = () => {
            this.loadDevices().then(() => {
                setTimeout(poll, POLLING_INTERVAL);
            });
        };

        poll();
    }

    loadDevices() {
        return fetch(DEVICE_LIST_API_URL).then(async (response) => {
            const { devices } = await response.json();
            this.setState({
                devices,
            });
        }, (error) => {
            if (!this.state.loaded) {
                this.setState({
                    error: error instanceof Error ? error.message : 'Что-то пошло не так',
                });
            }
        }).finally(() => {
            this.setState({
                loaded: true,
            });
        });
    }

    componentDidMount() {
        this.startPolling();
    }

    render() {
        const { error, loaded, devices } = this.state;

        if (error) {
            return (
                <div className="App App_loading">
                    {error}
                </div>
            );
        }

        if (!loaded) {
            return (
                <div className="App App_loading">
                    Подождите
                </div>
            );
        }

        return (
            <div className="App">
                <Devices devices={devices} />
            </div>
        );
    }
}
