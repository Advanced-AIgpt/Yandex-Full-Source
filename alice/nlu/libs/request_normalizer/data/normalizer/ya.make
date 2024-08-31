LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

# Russian

FROM_SANDBOX(
    693301554
    OUT_NOAUTO
    ru/address.expand_address.fst
    ru/address.expand_city_with_name.fst
    ru/address.expand_metro.fst
    ru/case_control.insert_control_marks.fst
    ru/case_control.remove_control_marks.fst
    ru/date.convert_date.fst
    ru/flags.txt
    ru/hyphen_deletion.hyphen_deletion.fst
    ru/ip_addr.convert_ip_address.fst
    ru/music_domain.music_top_transcriptions.fst
    ru/names.name_abbreviation.fst
    ru/number.convert_cardinal.fst
    ru/number.convert_fraction.fst
    ru/number.convert_ordinal.fst
    ru/number.convert_upper_register.fst
    ru/number.numbers_case_control.fst
    ru/number.prepare_cardinal.fst
    ru/number.prepare_ordinal.fst
    ru/number.use_ordinal_markers.fst
    ru/prepare_for_g2p.cvt.fst
    ru/roman.convert_roman.fst
    ru/sequence.txt
    ru/simple_conversions.collapse_spaces.fst
    ru/simple_conversions.convert_digit_sequences.fst
    ru/simple_conversions.convert_slash_between_numbers.fst
    ru/simple_conversions.final_spaces_adjustment.fst
    ru/simple_conversions.glue_punctuation.fst
    ru/simple_conversions.lower_and_whitespace.fst
    ru/simple_conversions.marriage.fst
    ru/simple_conversions.number_ranges.fst
    ru/simple_conversions.plus_minus.fst
    ru/simple_conversions.replace_sharp.fst
    ru/simple_conversions.replace_unconditionally.fst
    ru/simple_conversions.space_at_start_end.fst
    ru/simple_conversions.spaces_around_punctuation.fst
    ru/simple_conversions.split_l2d.fst
    ru/simple_conversions.transform_email.fst
    ru/simple_conversions.transform_url.fst
    ru/simple_conversions.words_to_phrases.fst
    ru/spec_codes.special_codes.fst
    ru/sport.expand_score.fst
    ru/symbols.sym
    ru/telephone.convert_telephone.fst
    ru/time_cvt.convert_time.fst
    ru/tts_norm_tests_critical_ans.txt
    ru/tts_norm_tests_critical_in.txt
    ru/tts_norm_tests_full_ans.txt
    ru/tts_norm_tests_full_in.txt
    ru/units.convert_number_abbr.fst
    ru/units.convert_size.fst
    ru/units.convert_units.fst
    ru/units.convert_units_with_slash.fst
)

RESOURCE(
    ru/address.expand_address.fst /nlu/request_fst_normalizer/normalizer/ru/address.expand_address.fst
    ru/address.expand_city_with_name.fst /nlu/request_fst_normalizer/normalizer/ru/address.expand_city_with_name.fst
    ru/address.expand_metro.fst /nlu/request_fst_normalizer/normalizer/ru/address.expand_metro.fst
    ru/case_control.insert_control_marks.fst /nlu/request_fst_normalizer/normalizer/ru/case_control.insert_control_marks.fst
    ru/case_control.remove_control_marks.fst /nlu/request_fst_normalizer/normalizer/ru/case_control.remove_control_marks.fst
    ru/date.convert_date.fst /nlu/request_fst_normalizer/normalizer/ru/date.convert_date.fst
    ru/flags.txt /nlu/request_fst_normalizer/normalizer/ru/flags.txt
    ru/hyphen_deletion.hyphen_deletion.fst /nlu/request_fst_normalizer/normalizer/ru/hyphen_deletion.hyphen_deletion.fst
    ru/ip_addr.convert_ip_address.fst /nlu/request_fst_normalizer/normalizer/ru/ip_addr.convert_ip_address.fst
    ru/music_domain.music_top_transcriptions.fst /nlu/request_fst_normalizer/normalizer/ru/music_domain.music_top_transcriptions.fst
    ru/names.name_abbreviation.fst /nlu/request_fst_normalizer/normalizer/ru/names.name_abbreviation.fst
    ru/number.convert_cardinal.fst /nlu/request_fst_normalizer/normalizer/ru/number.convert_cardinal.fst
    ru/number.convert_fraction.fst /nlu/request_fst_normalizer/normalizer/ru/number.convert_fraction.fst
    ru/number.convert_ordinal.fst /nlu/request_fst_normalizer/normalizer/ru/number.convert_ordinal.fst
    ru/number.convert_upper_register.fst /nlu/request_fst_normalizer/normalizer/ru/number.convert_upper_register.fst
    ru/number.numbers_case_control.fst /nlu/request_fst_normalizer/normalizer/ru/number.numbers_case_control.fst
    ru/number.prepare_cardinal.fst /nlu/request_fst_normalizer/normalizer/ru/number.prepare_cardinal.fst
    ru/number.prepare_ordinal.fst /nlu/request_fst_normalizer/normalizer/ru/number.prepare_ordinal.fst
    ru/number.use_ordinal_markers.fst /nlu/request_fst_normalizer/normalizer/ru/number.use_ordinal_markers.fst
    ru/prepare_for_g2p.cvt.fst /nlu/request_fst_normalizer/normalizer/ru/prepare_for_g2p.cvt.fst
    ru/roman.convert_roman.fst /nlu/request_fst_normalizer/normalizer/ru/roman.convert_roman.fst
    ru/sequence.txt /nlu/request_fst_normalizer/normalizer/ru/sequence.txt
    ru/simple_conversions.collapse_spaces.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.collapse_spaces.fst
    ru/simple_conversions.convert_digit_sequences.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.convert_digit_sequences.fst
    ru/simple_conversions.convert_slash_between_numbers.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.convert_slash_between_numbers.fst
    ru/simple_conversions.final_spaces_adjustment.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.final_spaces_adjustment.fst
    ru/simple_conversions.glue_punctuation.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.glue_punctuation.fst
    ru/simple_conversions.lower_and_whitespace.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.lower_and_whitespace.fst
    ru/simple_conversions.marriage.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.marriage.fst
    ru/simple_conversions.number_ranges.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.number_ranges.fst
    ru/simple_conversions.plus_minus.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.plus_minus.fst
    ru/simple_conversions.replace_sharp.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.replace_sharp.fst
    ru/simple_conversions.replace_unconditionally.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.replace_unconditionally.fst
    ru/simple_conversions.space_at_start_end.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.space_at_start_end.fst
    ru/simple_conversions.spaces_around_punctuation.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.spaces_around_punctuation.fst
    ru/simple_conversions.split_l2d.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.split_l2d.fst
    ru/simple_conversions.transform_email.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.transform_email.fst
    ru/simple_conversions.transform_url.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.transform_url.fst
    ru/simple_conversions.words_to_phrases.fst /nlu/request_fst_normalizer/normalizer/ru/simple_conversions.words_to_phrases.fst
    ru/spec_codes.special_codes.fst /nlu/request_fst_normalizer/normalizer/ru/spec_codes.special_codes.fst
    ru/sport.expand_score.fst /nlu/request_fst_normalizer/normalizer/ru/sport.expand_score.fst
    ru/symbols.sym /nlu/request_fst_normalizer/normalizer/ru/symbols.sym
    ru/telephone.convert_telephone.fst /nlu/request_fst_normalizer/normalizer/ru/telephone.convert_telephone.fst
    ru/time_cvt.convert_time.fst /nlu/request_fst_normalizer/normalizer/ru/time_cvt.convert_time.fst
    ru/tts_norm_tests_critical_ans.txt /nlu/request_fst_normalizer/normalizer/ru/tts_norm_tests_critical_ans.txt
    ru/tts_norm_tests_critical_in.txt /nlu/request_fst_normalizer/normalizer/ru/tts_norm_tests_critical_in.txt
    ru/tts_norm_tests_full_ans.txt /nlu/request_fst_normalizer/normalizer/ru/tts_norm_tests_full_ans.txt
    ru/tts_norm_tests_full_in.txt /nlu/request_fst_normalizer/normalizer/ru/tts_norm_tests_full_in.txt
    ru/units.convert_number_abbr.fst /nlu/request_fst_normalizer/normalizer/ru/units.convert_number_abbr.fst
    ru/units.convert_size.fst /nlu/request_fst_normalizer/normalizer/ru/units.convert_size.fst
    ru/units.convert_units.fst /nlu/request_fst_normalizer/normalizer/ru/units.convert_units.fst
    ru/units.convert_units_with_slash.fst /nlu/request_fst_normalizer/normalizer/ru/units.convert_units_with_slash.fst
)

# Turkish

FROM_SANDBOX(
    960471362
    OUT_NOAUTO
    tr/case_to_features.cvt.fst
    tr/convert_numbers.cvt.fst
    tr/date.cvt.fst
    tr/flags.txt
    tr/latitude_longitude.cvt.fst
    tr/roman.cvt.fst
    tr/sequence.txt
    tr/simple_conversions.collapse_spaces.fst
    tr/simple_conversions.final_spaces_adjustment.fst
    tr/simple_conversions.glue_punctuation_cvt.fst
    tr/simple_conversions.ignore_marks_near_numbers_cvt.fst
    tr/simple_conversions.lower_and_whitespace_cvt.fst
    tr/simple_conversions.ranges_cvt.fst
    tr/simple_conversions.remove_space_at_end.fst
    tr/simple_conversions.replace_punctuation_cvt.fst
    tr/simple_conversions.replace_unconditionally_cvt.fst
    tr/simple_conversions.space_at_start.fst
    tr/simple_conversions.spaces_around_punctuation.fst
    tr/simple_conversions.split_l2d_cvt.fst
    tr/symbols.sym
    tr/time_cvt.cvt.fst
    tr/units.cvt.fst
)

RESOURCE(
    tr/case_to_features.cvt.fst /nlu/request_fst_normalizer/normalizer/tr/case_to_features.cvt.fst
    tr/convert_numbers.cvt.fst /nlu/request_fst_normalizer/normalizer/tr/convert_numbers.cvt.fst
    tr/date.cvt.fst /nlu/request_fst_normalizer/normalizer/tr/date.cvt.fst
    tr/flags.txt /nlu/request_fst_normalizer/normalizer/tr/flags.txt
    tr/latitude_longitude.cvt.fst /nlu/request_fst_normalizer/normalizer/tr/latitude_longitude.cvt.fst
    tr/roman.cvt.fst /nlu/request_fst_normalizer/normalizer/tr/roman.cvt.fst
    tr/sequence.txt /nlu/request_fst_normalizer/normalizer/tr/sequence.txt
    tr/simple_conversions.collapse_spaces.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.collapse_spaces.fst
    tr/simple_conversions.final_spaces_adjustment.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.final_spaces_adjustment.fst
    tr/simple_conversions.glue_punctuation_cvt.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.glue_punctuation_cvt.fst
    tr/simple_conversions.ignore_marks_near_numbers_cvt.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.ignore_marks_near_numbers_cvt.fst
    tr/simple_conversions.lower_and_whitespace_cvt.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.lower_and_whitespace_cvt.fst
    tr/simple_conversions.ranges_cvt.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.ranges_cvt.fst
    tr/simple_conversions.remove_space_at_end.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.remove_space_at_end.fst
    tr/simple_conversions.replace_punctuation_cvt.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.replace_punctuation_cvt.fst
    tr/simple_conversions.replace_unconditionally_cvt.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.replace_unconditionally_cvt.fst
    tr/simple_conversions.space_at_start.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.space_at_start.fst
    tr/simple_conversions.spaces_around_punctuation.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.spaces_around_punctuation.fst
    tr/simple_conversions.split_l2d_cvt.fst /nlu/request_fst_normalizer/normalizer/tr/simple_conversions.split_l2d_cvt.fst
    tr/symbols.sym /nlu/request_fst_normalizer/normalizer/tr/symbols.sym
    tr/time_cvt.cvt.fst /nlu/request_fst_normalizer/normalizer/tr/time_cvt.cvt.fst
    tr/units.cvt.fst /nlu/request_fst_normalizer/normalizer/tr/units.cvt.fst
)

END()
