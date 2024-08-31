#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import argparse
import urllib2
import re

def pic_scale(name) :
    size_extension = name.split('.', 1)[1] # Gift.3.5.png -> 3.5.png
    size = '.'.join(size_extension.split('.')[:-1]) #  3.5.png -> 3.5
    return float(size)

def output_row(output, common_name, icons) :
    output.write('<tr>')
    output.write('<td rowspan=2>')
    output.write('<h2>' + common_name + '</h2>')
    output.write('</td>')
    icons_sorted = sorted(icons, key=pic_scale)
    for name in icons_sorted :
        output.write('<td>')
        output.write(name)
        output.write('</td>')
    output.write('</tr>')

    output.write('<tr>')
    for name in icons_sorted :
        output.write('<td>')
        output.write('<a href=' + icons[name] + '><img src=' + icons[name] + '></a>')
        output.write('</td>')
    output.write('</tr>')

def output_table(output, header, pics) :
    output.write('<h1>' + header + '</h1>')
    output.write('<table>')

    names_sorted = sorted(pics)
    for common_name in names_sorted :
        output_row(output, common_name, pics[common_name])

    output.write('</table>')

def output_style(output) :
    output.write('<style type="text/css">')
    output.write('td {border:1px solid black; text-align:center; vertical-align:middle; padding: 10px}')
    output.write('table {border:1px solid black; border-collapse:collapse; width:100%}')
    output.write('</style>')


def generate_html(avatars_json, filename) :
    output = open(filename, 'w')
    output.write('<html><body>')
    output_style(output)

    groups_sorted = sorted(avatars_json)
    for group_name in groups_sorted :
        group_avatars = avatars_json[group_name]
        pics = {}
        for name in group_avatars:
            common_name = name.split('.')[0]
            if common_name not in pics :
                pics[common_name] = {}
            url = group_avatars[name]["https"]
            pics[common_name][name] = url
        output_table(output, group_name, pics)

    output.write('</body></html>')
    output.close()

def find_resource_id(yamake_filename) :
    yamake = open(yamake_filename, 'r')
    # search for string like:
    # "FROM_SANDBOX(FILE 440658026 OUT_NOAUTO avatars.json"
    from_sandbox_string = re.search('FROM_SANDBOX.*avatars.json\)', yamake.read()).group(0)
    resource_id = from_sandbox_string.split(' ')[1]
    yamake.close()
    return resource_id

def main():
    parser = argparse.ArgumentParser(description="""Generates html-file with all current alice icons.
        Specify avatars.json file,
        sandbox resource id or just ya.make file with 'FROM_SANDBOX(...avatars.json)' string""")
    parser.add_argument('--avatars_json', '-a', help='avatars.json file')
    parser.add_argument('--resource_id', '-r', help='avatars.json sandbox resource id')
    parser.add_argument('--makefile', '-f', help='ya.make file with "FROM_SANDBOX(...avatars.json)" string')
    args = parser.parse_args()

    if not args.resource_id and not args.makefile and not args.avatars_json:
        print "Please specify avatars.json file, resource_id or ya.make file"
        return

    avatars_json_str = ''
    if args.avatars_json :
        avatars_file = open(args.avatars_json)
        avatars_json_str = avatars_file.read()
        avatars_file.close()
    else :
        resource_id = args.resource_id
        if not resource_id :
            resource_id = find_resource_id(args.makefile)
        sandbox_url = 'https://proxy.sandbox.yandex-team.ru/'  + resource_id
        avatars_json_str = urllib2.urlopen(sandbox_url).read()

    avatars_json = json.loads(avatars_json_str)
    generate_html(avatars_json, 'alice_icons.html')

if __name__ == '__main__':
    main()
