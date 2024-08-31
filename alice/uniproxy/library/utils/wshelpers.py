from alice.uniproxy.library.settings import config
from voicetech.library.proto_api.voiceproxy_pb2 import ConnectionRequest, AdvancedASROptions


def create_advanced_asr_options(data):
    if 'advancedOptions' in data:
        if 'use_snr' not in data['advancedOptions']:
            data['advancedOptions']['use_snr'] = True
        return AdvancedASROptions(**data.get('advancedOptions'))
    else:
        opts = AdvancedASROptions(
            partial_results=data.get('partialResults', True),
            beam=data.get('beam', -1),
            lattice_beam=data.get('latticeBeam', -1),
            lattice_nbest=data.get('latticeNbest', -1),
            utterance_silence=data.get('utteranceSilence', 120),
            allow_multi_utt=data.get('allowMultiUtterance', True),
            chunk_process_limit=data.get('chunkProcessLimit', 100),
            cmn_window=data.get('cmnWindow', 600),
            cmn_latency=data.get('cmnLatency', 150),
            capitalize=data.get('capitalize', False),
            expected_num_count=int(data.get('expNumCount', 0)),
            grammar=data.get('customGrammar', []),
            biometry=data.get('biometry', ''),
            use_snr=data.get('use_snr', True),
            snr_flags=data.get('snr_flags', []),
            biometry_group=data.get('biometry_group', ''),
            manual_punctuation=data.get('manualPunctuation', False)
        )
        if "srgs" in data:
            opts.srgs = data.get('srgs')
        return opts


def default_init_request(msg):
    request = ConnectionRequest(
        speechkitVersion="jsapi",
        serviceName="",
        uuid=msg.get('uuid'),
        apiKey=msg.get('apikey') or msg.get('key') or msg.get('apiKey'),
        applicationName=msg.get('applicationName', 'local'),
        device=msg.get("device", "unknown"),
        coords="0, 0",
        topic=msg.get('model', 'notes'),
        lang=msg.get('lang', 'ru-RU'),
        format=msg.get('format'),
        punctuation=msg.get('punctuation', True),
        advancedASROptions=create_advanced_asr_options(msg),
        disableAntimatNormalizer=msg.get('allowStrongLanguage', False)
    )
    #  VOICE-4041
    if request.apiKey in config.get("wsproxy", {}).get("offendTolerant", []):
        request.disableAntimatNormalizer = True
    if 'yandexuid' in msg:
        request.yandexuid = msg.get('yandexuid')
    return request
