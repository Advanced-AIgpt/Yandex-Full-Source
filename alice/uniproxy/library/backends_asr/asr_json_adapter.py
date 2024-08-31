def convert_init_request_asr0_to_asr1(init_request):
    old_advanced_options = init_request.pop('advancedASROptions', {})
    advanced_options = init_request.pop('advanced_options', old_advanced_options)
    init_request['advanced_options'] = advanced_options

    if 'mime' not in advanced_options:
        # import mime from old format field
        mime_format = init_request.pop('format', None)
        if mime_format:
            advanced_options['mime'] = mime_format

    if 'normalizer_options' not in init_request:
        # build modern normalizer_options from deprecated params
        normalizer_options = {
            'name': 'revnorm',
            'lang': init_request['lang'],
        }
        banlist = []
        if advanced_options.get('manual_punctuation', False):
            normalizer_options['name'] = 'revnorm_manual_punct'
        if not advanced_options.get('capitalize', False):
            banlist.append('punctuation_cvt.capitalize')
        if init_request.get('disableAntimatNormalizer', False):
            banlist.append('reverse_conversion.profanity')
            banlist.append('simple_conversions.profanity')
        normalizer_options['expected_num_count'] = advanced_options.get('expected_num_count', 0)
        if banlist:
            normalizer_options['banlist'] = banlist
        init_request['normalizer_options'] = normalizer_options
