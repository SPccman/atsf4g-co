#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os, platform, locale
import shutil, re, string
from optparse import OptionParser
import configparser
import glob
import common.print_color
import common.project_utils as project

console_encoding = sys.getfilesystemencoding()
script_dir = os.path.dirname(os.path.realpath(__file__))

if __name__ == '__main__':
    os.chdir(script_dir)
    from mako.template import Template
    from mako.lookup import TemplateLookup
    project_lookup = TemplateLookup(directories=[
        os.path.join(script_dir, 'helper', 'template', 'etc'),
        os.path.join(script_dir, 'helper', 'template', 'script')
    ])

    parser = OptionParser("usage: %prog [options...]")
    parser.add_option("-e", "--env-prefix", action='store', dest="env_prefix", default='AUTOBUILD_', help="prefix when read parameters from environment variables")
    parser.add_option("-c", "--config", action='store', dest="config", default=os.path.join(script_dir, 'config.conf'), help="configure file(default: {0})".format(os.path.join(script_dir, 'config.conf')))
    parser.add_option("-s", "--set", action='append' , dest="set_vars", default=[], help="set configures")
    parser.add_option("-n", "--number", action='store' , dest="reset_number", default=None, type='int', help="set default server numbers")
    parser.add_option("-i", "--id-offset", action='store' , dest="server_id_offset", default=0, type='int', help="set server id offset(default: 0)")
    parser.add_option("-p", "--port-offset", action='store' , dest="server_port_offset", default=1, type='int', help="set server port offset(default: 0)")

    (opts, args) = parser.parse_args()
    config = configparser.ConfigParser(inline_comment_prefixes=('#', ';'))
    config.read(opts.config)
    # all custon environment start with SYSTEM_MACRO_CUSTOM_
    # TODO reset all servers's number
    # TODO set all custom configures
    project.set_global_opts(config)
    # TODO parse all services
    #for generator in glob.glob(os.path.join(script_dir, 'helper', 'generator', '*')):
    #    set_server_inst(None, 'atproxy', 1)
    #    # TODO use all template
    #    # http://www.makotemplates.org/
    project.set_server_inst(None, 'atproxy', 1)
    mytemplate = Template(filename=os.path.join(script_dir, 'helper', 'template', 'etc', 'atproxy.conf'), lookup=project_lookup)
    print(mytemplate.render())