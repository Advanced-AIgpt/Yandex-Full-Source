import { createClearQueueAction } from '../../actions/client';
import { directivesAction } from '../../../../common/actions';

export const MUSIC_PLAYER_SECONDARY_BLOCK = 'music_player_secondary_block';
export const MUSIC_PLAYER_SECONDARY_BLOCK_COVER = 'cover';
export const MUSIC_PLAYER_SECONDARY_BLOCK_LIST = 'list';

export const MUSIC_PLAYER_SHUFFLE_TRIGGER = 'music_player_shuffle_trigger';
export const MUSIC_PLAYER_SHUFFLE_TRIGGER_ON = 'on';
export const MUSIC_PLAYER_SHUFFLE_TRIGGER_OFF = 'off';

export const MUSIC_ICON_SIZE = 72;
export const MUSIC_ICON_IMAGE_SIZE = 48;

export const ACTION_CLOSE_MUSIC = directivesAction(createClearQueueAction());

export const MUSIC_PLAYER_TOP_PART_HEIGHT = 160;
export const MUSIC_PLAYER_BOTTOM_PART_HEIGHT = 200;
