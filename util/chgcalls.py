#!/usr/bin/python3
# vim:sw=4

from io import StringIO
from getopt import gnu_getopt, GetoptError
import re
import sys
import os


class ChgCalls:
    def __init__(self):
        self.input_filename = '-'
        self.input_file = None
        self.output_filename = '-'
        self.output_file = None
        self.wrapper_filename = ''
        self.wrapper_file = None
        self.function_regex = None
        self.underscore = False
        self.prefix = "ac_"
        self.debug = False
        self.renamed_functions = {}

    @staticmethod
    def print_help():
        print("Usage: %s CONFIGFILE < SRCFILE > DSTFILE" % (sys.argv[0]))

    @staticmethod
    def strip_line(line):
        n = line.find("#")
        if n >= 0:
            line = line[:n]
        return line.strip()

    def read_config(self):
        """Build a regexp that will match all the function names that are
        meant to be replaced, and save it in self.function_regex, based on the
        wrapper config."""
        all_regex = StringIO()
        all_regex.write(r"(^\s*)\b((")
        ln = 0
        while True:
            line = self.wrapper_file.readline()
            if len(line) == 0:
                break
            ln += 1
            line = self.strip_line(line)
            if len(line) == 0:
                continue
            parts = re.split(r"\s+", line)
            if len(parts) < 2 or len(parts) > 3:
                print("Bad line %i: %s" % (ln, line), file=sys.stderr)
                continue
            if all_regex.tell() > 9:
                all_regex.write("|")
            all_regex.write(self.sym_name(parts[0]))
        all_regex.write(r"):)(.*)")
        self.function_regex = re.compile(all_regex.getvalue())

    def wrap_name(self, name):
        return "%s%s" % (self.prefix, name)

    def sym_name(self, name):
        name = self.renamed_functions.get(name, name)
        if not self.underscore:
            return "%s" % name
        return "_%s" % name

    def append_rename_config(self, name):
        # Open file
        config_file = self.open_file(name)

        # Parse
        ln = 0
        while True:
            line = config_file.readline()
            if not line:
                break
            ln += 1
            line = self.strip_line(line)
            if len(line) == 0:
                continue
            parts = re.split(r'\s+', line)
            if len(parts) != 2:
                print('Bad line %i: %s' % (ln, line), file=sys.stderr)
                continue
            self.renamed_functions[parts[0]] = parts[1]

        # Done
        self.close_file(config_file, name)

    @staticmethod
    def open_file(name, mode="r"):
        if name == "-":
            if mode.find("r") >= 0:
                return sys.stdin
            return sys.stdout
        return open(name, mode)

    @staticmethod
    def close_file(file, name):
        if name == "-":
            return
        file.close()

    def change(self):
        file_name, _ = os.path.splitext(os.path.basename(self.input_filename))
        self.output_file.write("#include \"%s.h\"\n\n" % file_name)

        while True:
            line = self.input_file.readline()
            if len(line) == 0:
                break

            while True:
                m = self.function_regex.match(line)
                if m is None:
                    break
                line = "%sREPLACE(%s):%s\n" % (m.group(1), m.group(3), m.group(4))

            self.output_file.write(line)

    def main(self):
        try:
            opts, args = gnu_getopt(sys.argv[1:], "hr:", ("help", "rename-config="))
        except GetoptError as message:
            print("Error:", message, file=sys.stderr)
            sys.exit(1)

        for opt, arg in opts:
            if opt in ("-h", "--help"):
                self.print_help()
                sys.exit(0)
            elif opt in ('-r', '--rename-config'):
                self.append_rename_config(arg)

        if len(args) == 1:
            self.wrapper_filename = args[0]
        elif len(args) == 3:
            self.wrapper_filename = args[0]
            self.input_filename = args[1]
            self.output_filename = args[2]
        else:
            print("Error: Bad number of arguments (must be 1 or 3)", file=sys.stderr)
            sys.exit(1)

        self.wrapper_file = self.open_file(self.wrapper_filename)
        self.read_config()
        self.close_file(self.wrapper_file, self.wrapper_filename)

        self.input_file = self.open_file(self.input_filename)
        self.output_file = self.open_file(self.output_filename, "w")

        self.change()

        self.close_file(self.output_file, self.output_filename)
        self.close_file(self.input_file, self.input_filename)
        self.input_file = None
        self.output_file = None


ChgCalls().main()
