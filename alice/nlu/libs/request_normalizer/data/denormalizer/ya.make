LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

FROM_SANDBOX(
    693302669
    OUT_NOAUTO
    ru/flags.txt
    ru/number.convert_fraction_symbol.fst
    ru/number.convert_number_segment.fst
    ru/number.convert_size.fst
    ru/punctuation_cvt.capitalize.fst
    ru/punctuation_cvt.glue_punctuation.fst
    ru/reverse_conversion.dates.fst
    ru/reverse_conversion.make_substitution_group.fst
    ru/reverse_conversion.number_sequence.fst
    ru/reverse_conversion.numbers.fst
    ru/reverse_conversion.phones.fst
    ru/reverse_conversion.profanity.fst
    ru/reverse_conversion.remove_space_at_start.fst
    ru/reverse_conversion.remove_unk.fst
    ru/reverse_conversion.space_at_start.fst
    ru/reverse_conversion.times.fst
    ru/sequence.txt
    ru/symbols.sym
    ru/units.converter.fst
)

RESOURCE(
    ru/flags.txt /nlu/request_fst_normalizer/denormalizer/ru/flags.txt
    ru/number.convert_fraction_symbol.fst /nlu/request_fst_normalizer/denormalizer/ru/number.convert_fraction_symbol.fst
    ru/number.convert_number_segment.fst /nlu/request_fst_normalizer/denormalizer/ru/number.convert_number_segment.fst
    ru/number.convert_size.fst /nlu/request_fst_normalizer/denormalizer/ru/number.convert_size.fst
    ru/punctuation_cvt.capitalize.fst /nlu/request_fst_normalizer/denormalizer/ru/punctuation_cvt.capitalize.fst
    ru/punctuation_cvt.glue_punctuation.fst /nlu/request_fst_normalizer/denormalizer/ru/punctuation_cvt.glue_punctuation.fst
    ru/reverse_conversion.dates.fst /nlu/request_fst_normalizer/denormalizer/ru/reverse_conversion.dates.fst
    ru/reverse_conversion.make_substitution_group.fst /nlu/request_fst_normalizer/denormalizer/ru/reverse_conversion.make_substitution_group.fst
    ru/reverse_conversion.number_sequence.fst /nlu/request_fst_normalizer/denormalizer/ru/reverse_conversion.number_sequence.fst
    ru/reverse_conversion.numbers.fst /nlu/request_fst_normalizer/denormalizer/ru/reverse_conversion.numbers.fst
    ru/reverse_conversion.phones.fst /nlu/request_fst_normalizer/denormalizer/ru/reverse_conversion.phones.fst
    ru/reverse_conversion.profanity.fst /nlu/request_fst_normalizer/denormalizer/ru/reverse_conversion.profanity.fst
    ru/reverse_conversion.remove_space_at_start.fst /nlu/request_fst_normalizer/denormalizer/ru/reverse_conversion.remove_space_at_start.fst
    ru/reverse_conversion.remove_unk.fst /nlu/request_fst_normalizer/denormalizer/ru/reverse_conversion.remove_unk.fst
    ru/reverse_conversion.space_at_start.fst /nlu/request_fst_normalizer/denormalizer/ru/reverse_conversion.space_at_start.fst
    ru/reverse_conversion.times.fst /nlu/request_fst_normalizer/denormalizer/ru/reverse_conversion.times.fst
    ru/sequence.txt /nlu/request_fst_normalizer/denormalizer/ru/sequence.txt
    ru/symbols.sym /nlu/request_fst_normalizer/denormalizer/ru/symbols.sym
    ru/units.converter.fst /nlu/request_fst_normalizer/denormalizer/ru/units.converter.fst
)

FROM_SANDBOX(
    960481142
    OUT_NOAUTO
    tr/flags.txt
    tr/reverse_conversion.numbers_step.fst
    tr/reverse_conversion.remove_space_at_start.fst
    tr/reverse_conversion.space_at_start.fst
    tr/sequence.txt
    tr/symbols.sym
)

RESOURCE(
    tr/flags.txt /nlu/request_fst_normalizer/denormalizer/tr/flags.txt
    tr/reverse_conversion.numbers_step.fst /nlu/request_fst_normalizer/denormalizer/tr/reverse_conversion.numbers_step.fst
    tr/reverse_conversion.remove_space_at_start.fst /nlu/request_fst_normalizer/denormalizer/tr/reverse_conversion.remove_space_at_start.fst
    tr/reverse_conversion.space_at_start.fst /nlu/request_fst_normalizer/denormalizer/tr/reverse_conversion.space_at_start.fst
    tr/sequence.txt /nlu/request_fst_normalizer/denormalizer/tr/sequence.txt
    tr/symbols.sym /nlu/request_fst_normalizer/denormalizer/tr/symbols.sym
)

END()
