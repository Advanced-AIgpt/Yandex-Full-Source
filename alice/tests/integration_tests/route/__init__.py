import re

CoordEps = 7e-3


def get_point(string, arg_name):
    matches = re.search(fr'{arg_name}=(?P<point>\d+.\d+)', string)
    assert matches, f'No match {arg_name} in "{string}"'
    return float(matches['point'])
