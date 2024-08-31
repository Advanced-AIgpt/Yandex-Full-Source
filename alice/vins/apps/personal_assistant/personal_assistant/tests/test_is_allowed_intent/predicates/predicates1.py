from personal_assistant.is_allowed_intent.predicate_registrant import PredicateRegistrant

reg = PredicateRegistrant()


@reg
def is_div_5(a):
    return a % 5 == 0


@reg
def is_div_7(a):
    return a % 7 == 0


@reg
def is_eq_b(b):
    return b == 'b'


@reg
def is_eq_c(b):
    return b == 'c'


@reg
def is_eq_a(b):
    return b == 'a'


@reg
def is_eq_e(b):
    return b == 'e'


@reg
def is_eq_d(b):
    return b == 'd'
