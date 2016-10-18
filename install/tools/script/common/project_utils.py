#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os, ctypes, platform
import cgi, re


global_opts = None
server_opts = None
server_name = ''
server_index = 1
server_cache_id = None
server_cache_full_name = None
server_cache_ip = {}

def set_global_opts(opts):
    global global_opts
    global_opts = opts

def set_server_inst(opts, key, index):
    global server_opts
    global server_name
    global server_index
    global server_cache_id
    global server_cache_full_name
    server_opts = opts
    server_name = key
    server_index = index
    server_cache_id = None
    server_cache_full_name = None

def get_ip_list_v4():
    global server_cache_ip
    if server_cache_ip.ipv4 is None:
        import socket
        server_cache_ip.ipv4 = []
        for ip_pair in socket.getaddrinfo(socket.gethostname(), 0, socket.AF_INET, socket.SOCK_STREAM):
            server_cache_ip.ipv4.push(ip_pair[4][0])
    return server_cache_ip.ipv4

def get_ip_list_v6():
    global server_cache_ip
    if server_cache_ip.ipv6 is None:
        import socket
        server_cache_ip.ipv6 = []
        for ip_pair in socket.getaddrinfo(socket.gethostname(), 0, socket.AF_INET6, socket.SOCK_STREAM):
            server_cache_ip.ipv6.push(ip_pair[4][0])
    return server_cache_ip.ipv6

def get_inner_ipv4():
    if 'SYSTEM_MACRO_INNER_IPV4' in os.environ:
        return os.environ['SYSTEM_MACRO_INNER_IPV4']
    # detect inner ip address
    res = get_ip_list_v4()
    if 0 == len(res):
        return '127.0.0.1'
    return res[0]


def get_outer_ipv4():
    if 'SYSTEM_MACRO_OUTER_IPV4' in os.environ:
        return os.environ['SYSTEM_MACRO_OUTER_IPV4']
    # detect inner ip address
    res = get_ip_list_v4()
    if 0 == len(res):
        return '0.0.0.0'
    return res[0]

def get_inner_ipv6():
    if 'SYSTEM_MACRO_INNER_IPV6' in os.environ:
        return os.environ['SYSTEM_MACRO_INNER_IPV6']
    # detect inner ip address
    res = get_ip_list_v6()
    if 0 == len(res):
        return '::1'
    return res[0]

def get_outer_ipv6():
    if 'SYSTEM_MACRO_OUTER_IPV6' in os.environ:
        return os.environ['SYSTEM_MACRO_OUTER_IPV6']
    # detect inner ip address
    res = get_ip_list_v6()
    if 0 == len(res):
        return '::'
    return res[0]

def get_hostname():
    if 'SYSTEM_MACRO_HOST_NAME' in os.environ:
        return os.environ['SYSTEM_MACRO_HOST_NAME']
    return ''

def get_global_option(section, key, default_val, env_name = None):
    global global_opts
    if not env_name is None and env_name in os.environ:
        return os.environ[env_name]
    
    if global_opts.has_option(section, key):
        return global_opts.get(section, key)
    
    return default_val

def get_global_list(section, key, default_val, env_name = None):
    res = get_global_option()
    ret = []
    for item in res:
        ret.push(item.strip())
    return ret

def get_global_list_to_hosts(section, key, default_val, env_name = None):
    res = get_global_list()
    ret = []
    mat = re.compile('([^:]*):(\d+)-(\d+)\s*$')
    for item in res:
        mat_res = mat.match(item)
        if mat_res:
            for i in range(int(mat_res.group(2)), int(mat_res.group(3)) + 1):
                ret.push('{0}:{1}'.format(mat_res.group(1), i))
        else:
            ret.push(item)
    return ret

def get_server_name():
    global server_name
    return server_name

def get_server_option(key, default_val, env_name = None):
    return get_global_option('server.{0}'.format(get_server_name()), key, default_val, env_name)

def get_server_list(key, default_val, env_name = None):
    return get_global_list('server.{0}'.format(get_server_name()), key, default_val, env_name)

def get_server_list_to_hosts(key, default_val, env_name = None):
    return get_global_list_to_hosts('server.{0}'.format(get_server_name()), key, default_val, env_name)

def get_server_index():
    global server_index
    return server_index

def get_server_id():
    global server_cache_id
    global global_opts
    if not server_cache_id is None:
        return server_cache_id

    if not global_opts.has_option('atservice', get_server_name()):
        return 0
    group_id = int(get_global_option('global', 'group_id', 1))
    group_step = int(get_global_option('global', 'group_step', 0x10000))
    type_step = int(get_global_option('global', 'type_step', 0x100))
    type_id = int(get_global_option('atservice', get_server_name(), 0))
    server_cache_id = group_id * group_step + type_step * type_id + get_server_index()
    return server_cache_id

def get_server_full_name():
    global server_cache_full_name
    if not server_cache_full_name is None:
        return server_cache_full_name
    server_cache_full_name = '{0}-{1}'.format(get_server_name(), get_server_index())
    return server_cache_full_name
