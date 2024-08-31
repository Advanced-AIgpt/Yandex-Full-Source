#!/usr/bin/env python
# encoding: utf-8
import re

message = r"""SESSIONLOG: b'{\"Directive\"={\"ForEvent\"=\"1abf2aa9-811e-4147-9209-18a454ed6f80\";\"type\"=\"VinsRequest\";\"Body\"={\"request\"={\"location\"={\"lon\"=37.45170974731445;\"lat\"=55.53000259399414;};\"event\"={\"type\"=\"voice_input\";\"asr_result\"=[{\"confidence\"=1.;\"words\"=[{\"confidence\"=1.;\"value\"=\"\\\\xD0\\\\xBF\\\\xD0\\\\xBE\\\\xD0\\\\xB3\\\\xD0\\\\xBE\\\\xD0\\\\xB4\\\\xD0\\\\xB0\";};{\"confidence\"=1.;\"value\"=\"\\\\xD0\\\\xBC\\\\xD0\\\\xBE\\\\xD1\\\\x81\\\\xD0\\\\xBA\\\\xD0\\\\xB2\\\\xD1\\\\x83\";};];\"utterance\"=\"\\\\xD0\\\\xBF\\\\xD0\\\\xBE\\\\xD0\\\\xB3\\\\xD0\\\\xBE\\\\xD0\\\\xB4\\\\xD0\\\\xB0 \\\\xD0\\\\xBC\\\\xD0\\\\xBE\\\\xD1\\\\x81\\\\xD0\\\\xBA\\\\xD0\\\\xB2\\\\xD1\\\\x83 \";};];};\"experiments\"=[\"\";];\"voice_session\"=%true;\"additional_options\"={\"bass_options\"={\"filtration_level\"=1;};};\"reset_session\"=%false;};\"header\"={\"request_id\"=\"8947279d-7b11-438d-9d66-f6faf3835458\";};\"application\"={\"app_id\"=\"ru.yandex.searchplugin.beta\";\"platform\"=\"android\";\"app_version\"=\"6.45\";\"lang\"=\"ru-RU\";\"timestamp\"=\"1500631447\";\"uuid\"=\"63a9ca38f0a05637db9454cacb1092a1\";\"client_time\"=\"20170721T130407\";\"os_version\"=\"6.0.1\";\"timezone\"=\"Europe/Mosco"""

print re.search(r'(?<=\\"request_id\\"=\\")[^\\]+', message).group()

print re.search(r'(?<=\\"uuid\\"=\\")[^\\]+', message).group()

print re.search(r'(?<=\\"ForEvent\\"=\\")[^\\]+', message).group()

