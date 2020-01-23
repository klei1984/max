#!/usr/bin/python3
# vim:sw=4

"""
The code generator takes a *.in resource file and post-processes it into a re-compilable GAS compliant assembler
module, generates access providers to exposed routines and injects alternative implementations to replaced routines.
The settings are passed in a human readable json configuration file.

The script aims to support the various x86 calling conventions originally supported by the Watcom 10.x C/C++ compiler
including limited support for the big code model for __far calls and the big data model __far pointers.
"""

__author__ = "M.A.X. Port Team"
__copyright__ = "Copyright (c) 2020 M.A.X. Port Team"
__license__ = "MIT License"
__version__ = "0.3"

import re
import sys
import os
from getopt import gnu_getopt, GetoptError
from io import StringIO
import json


class Codegen:
    underscore = False
    config = None
    input_file = ""
    output_directory = ""
    function_regex = None

    @staticmethod
    def print_help():
        print(
            ("Usage: %s OPTIONS\n"
             "\n"
             "Options:\n"
             "  -h  --help		    shows this help text\n"
             "  -i  --input         input resource file to process"
             "  -o  --output        output directory to emit generated source files"
             "  -u  --underscore	prefixes symbol names with an underscore\n") % (
                sys.argv[0]))

    @staticmethod
    def minify(file_path):
        """ Remove optional comments from JSON like configuration files and loads the resulting JSON string

        :param file_path: path to JSON like configuration file
        :return: object
        """
        with open(file_path, 'r') as f:
            json_list = json.loads('\n'.join([row for row in f.readlines() if len(row.split('//')) == 1]))
        return json_list

    def read_config(self):
        """Build a regexp that will match all the function names that are
        meant to be replaced, and save it in self.function_regex."""
        all_regex = StringIO()
        all_regex.write(r"(^\s*)\b((")

        for function in self.config["functions"]:
            if function["mode"] == "replace":
                if all_regex.tell() > 9:
                    all_regex.write("|")
                all_regex.write(function["symbol"])
        all_regex.write(r"):)(.*)")
        self.function_regex = re.compile(all_regex.getvalue())

    def prefix_name(self, name):
        if not self.underscore:
            return "%s" % name
        return "_%s" % name

    def create_resource_header(self):
        file_name, _ = os.path.splitext(os.path.basename(self.input_file))

        output_file = open(self.output_directory + "/" + file_name + ".h", "w")
        output_file.write(self.config["copyrights"]["port"])
        output_file.write("/* Helper macros */\n")
        output_file.write("#define GLOBAL(sym) .global sym\n")
        output_file.write("#define REPLACE(sym) REMOVED_##sym\n")
        output_file.write("\n/* Replaced and exposed symbols */\n")

        for function in self.config["functions"]:
            if function["mode"] == "replace":
                output_file.write("#define %s %s\n" % (function["symbol"], self.prefix_name(function["name"])))
            elif function["mode"] == "expose":
                output_file.write("GLOBAL(%s)\n" % (function["symbol"]))

        output_file.close()

    def process_resource_file(self):
        file_name, _ = os.path.splitext(os.path.basename(self.input_file))

        input_file = open(self.input_file, 'r')
        output_file = open(self.output_directory + "/" + file_name + ".S", "w")
        output_file.write(self.config["copyrights"]["proprietary"])
        output_file.write("#include \"%s.h\"\n\n" % file_name)

        while True:
            line = input_file.readline().rstrip(" \t")
            if len(line) == 0:
                break

            while True:
                m = self.function_regex.match(line)
                if m is None:
                    break
                line = "%sREPLACE(%s):%s\n" % (m.group(1), m.group(3), m.group(4))

            output_file.write(line)
        output_file.close()
        input_file.close()

    def main(self):
        try:
            opts, args = gnu_getopt(sys.argv[1:], 'hi:o:u', ('help', 'input=', 'output=', 'underscore'))

        except GetoptError as message:
            print('Error:', message, file=sys.stderr)
            sys.exit(1)

        for opt, arg in opts:
            if opt in ('-h', '--help'):
                self.print_help()
                sys.exit(0)
            elif opt in ('-i', '--input'):
                self.input_file = arg
            elif opt in ('-o', '--output'):
                self.output_directory = arg
            elif opt in ('-u', '--underscore'):
                self.underscore = True

        if len(args) == 1:
            self.config = self.minify(args[0])
        elif len(args) > 1:
            print('Error: Too many arguments', file=sys.stderr)
            sys.exit(1)

        self.read_config()
        self.process_resource_file()
        self.create_resource_header()


Codegen().main()
