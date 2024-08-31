OWNER(g:alice)

UNION()

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

END()
