# -*- coding: utf-8 -*-

import click
import numpy as np
from dataset import VinsDataset


def _calc_device_state_info(device_state):
    current_screen_mapping = {
        "bluetooth": 1,
        "description": 2,
        "gallery": 3,
        "main": 4,
        "music_player": 5,
        "payment": 6,
        "season_gallery": 7,
        "splash": 8,
        "video_player": 9
    }
    is_tv_plugged_in = 0
    if 'is_tv_plugged_in' in device_state:
        is_tv_plugged_in = 2 if device_state['is_tv_plugged_in'] else 1

    player_pause = 0
    if 'music' in device_state and 'player' in device_state['music']:
        player_pause = 2 if device_state['music']['player'] else 1

    current_screen = 0
    if 'video' in device_state and 'current_screen' in device_state['video']:
        current_screen = current_screen_mapping.get(device_state['video']['current_screen'], 0)

    return [is_tv_plugged_in, player_pause, current_screen]


def _add_device_state(path):
    dataset = VinsDataset.restore(path)

    feature = []
    for additional_info in dataset._additional_infos:
        feature.append(_calc_device_state_info(additional_info.device_state))

    dataset.add_feature(
        feature_name='device_state_feature',
        feature_type=VinsDataset.FeatureType.DENSE,
        feature_matrix=np.array(feature),
        feature_mapping={'is_tv_plugged_in': 0, 'music_player': 1, 'current_screen': 2}
    )
    dataset.save(path)


@click.command()
@click.option('--train-data-path', type=click.Path(exists=True))
@click.option('--val-data-path', type=click.Path(exists=True))
def main(train_data_path, val_data_path):
    _add_device_state(train_data_path)
    _add_device_state(val_data_path)


if __name__ == '__main__':
    main()
