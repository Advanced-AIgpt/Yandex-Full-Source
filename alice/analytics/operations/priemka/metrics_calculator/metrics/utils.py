def calc_diff_percent(prod, test, eps=0.0001):
    diff_abs = abs(test - prod)
    prod_abs = abs(prod)
    sign = 1 if test >= prod else -1

    if prod_abs > eps:
        return sign * 100.0 * diff_abs / prod_abs
    elif diff_abs > eps:
        return sign * 100.0
    else:
        return 0.0
