#!/usr/bin/python3
# vim:sw=4

import re
import sys
from getopt import gnu_getopt, GetoptError
from io import StringIO

REGISTERS = 'adbc'


class MKWrappers:
    input_filename = '-'
    input_file = None
    output_filename = '-'
    output_file = None
    functions = {}
    underscore = False
    prefix = 'ac_'
    debug = False
    stack_align = 4
    prologue_args = 0
    renamed_functions = {}

    @staticmethod
    def print_help():
        print(
            ("Usage: %s OPTIONS CONFIG\n"
             "\n"
             "Options:\n"
             "  -h  --help		    shows this help text\n"
             "  -g  --debug		    makes wrappers print debug output to stdout\n"
             "  -u  --underscore	    prefixes symbol names with an underscore\n"
             "  -s  --stack-align=N       aligns the stack to N bytes before function calls\n"
             "  -r  --rename-config=FILE  renames functions using the table from FILE\n"
             "  -o FILE		    outputs wrappers to FILE instead of stdout") % (sys.argv[0]))

    @staticmethod
    def strip_line(line):
        n = line.find("#")
        if n >= 0:
            line = line[:n]
        return line.strip()

    def read_input(self):
        ln = 0
        while True:
            line = self.input_file.readline()
            if not line:
                break
            ln += 1
            line = self.strip_line(line)
            if len(line) == 0:
                continue
            parts = re.split(r"\s+", line)
            if len(parts) < 2 or len(parts) > 3:
                print("Bad line %i: %s" % (ln, line), file=sys.stderr)
                continue
            if len(parts) == 2:
                parts.append('')
            self.functions[parts[0]] = (parts[1], parts[2])

    def wrap_name(self, name):
        return "%s%s" % (self.prefix, name)

    def sym_name(self, name):
        name = self.renamed_functions.get(name, name)
        if not self.underscore:
            return "%s" % name
        return "_%s" % name

    def generate_wrappers(self):
        self.read_input()
        self.wrln()
        self.wrln(".text")
        funcs = self.functions.keys()
        for func in sorted(funcs):
            typ, args = self.functions[func]
            self.generate_wrapper(func, typ, args)

    def generate_push_arguments(self, typ, arg_count):
        if typ in "vc":
            for n in range(arg_count):
                self.wrln("\t\tpush\t0x%x(%%ebp)" % ((arg_count - n + 1) * 4))
        else:
            reg_arg_count = min(4, arg_count)

            for x in range(reg_arg_count):
                self.wrln("\t\tpush\t%%e%sx" % REGISTERS[reg_arg_count - x - 1])

            # TODO: non-register arguments
            assert reg_arg_count == arg_count

    def generate_store_registers(self):
        # Store registers that should be preserved by a Watcom function.
        # The System V ABI for i386 says that cdecl functions preserve
        # ebx, esi, edi, ebp, esp, so we only need to store the remaining
        # registers.
        self.wrln('\t\tpush\t%ecx')
        self.wrln('\t\tpush\t%edx')

    def generate_restore_registers(self):
        self.wrln()
        self.wrln('\t\tpop\t%edx')
        self.wrln('\t\tpop\t%ecx')

    def generate_call(self, name, arg_count):
        self.wrln('\t\tcall\t%s' % self.sym_name(name))
        if arg_count == 0:
            return
        self.wrln('\t\tadd\t$0x%x,%%esp' % (arg_count * 4))

    def generate_prologue(self):
        self.wrln('\t\tpush\t%ebp')
        self.wrln('\t\tmov\t%esp,%ebp')
        if self.prologue_args > 0:
            self.wrln('\t\tsub\t$0x%x,%%esp' % (4 * self.prologue_args))
        self.wrln()

    def generate_epilogue(self):
        self.wrln()
        self.wrln('\t\tleave')

    def generate_format_string(self, name, args):
        self.wrln('.data')
        self.wrln('\t0:\t.string "%s (%s)\\n"' % (name, self.arg_string(args)))
        self.wrln('.text')

    def generate_printf_call(self, name, typ, args):
        if not self.debug:
            return
        self.generate_align_push(len(args) + 1, 1)
        if len(args) > 0:
            self.wrln()
            self.generate_push_arguments(typ, len(args))
        self.wrln('\t\tpush\t$0f')
        self.generate_call('printf', len(args) + 1)
        self.generate_align_pop(1)

    def generate_ret(self):
        self.wrln('\t\tret')

    def generate_align_push(self, n_args, st_var_index=0):
        if self.stack_align <= 4:
            return
        self.wrln()
        self.wrln('\t\tmov\t%%esp,-0x%x(%%ebp)' % (4 * (st_var_index + 1)))
        if n_args > 0:
            self.wrln('\t\tsub\t$0x%x,%%esp' % (4 * n_args))
        self.wrln('\t\tand\t$-0x%x,%%esp' % self.stack_align)
        if n_args > 0:
            self.wrln('\t\tadd\t$0x%x,%%esp' % (4 * n_args))

    def generate_align_pop(self, st_var_index=0):
        if self.stack_align <= 4:
            return
        self.wrln()
        self.wrln('\t\tmov\t-0x%x(%%ebp),%%esp' % (4 * (st_var_index + 1)))

    def wrln(self, line=''):
        self.output_file.write(line)
        self.output_file.write('\n')

    def generate_wrapper(self, name, typ, args):
        self.wrln()
        self.wrln('.global %s' % self.wrap_name(name))
        self.wrln('%s: /* %s %s */' % (self.wrap_name(name), typ, args))

        if typ == 'w':
            if self.stack_align > 4:
                self.generate_prologue()
            self.generate_store_registers()
            self.generate_align_push(len(args))
            if len(args) > 0:
                self.wrln()
                self.generate_push_arguments(typ, len(args))
            self.generate_printf_call(name, typ, args)
            self.wrln()
            self.generate_call(name, len(args))
            self.generate_align_pop()
            self.generate_restore_registers()
            if self.stack_align > 4:
                self.generate_epilogue()
            self.generate_ret()
        elif typ == 'c':
            self.generate_prologue()
            self.generate_store_registers()
            self.generate_align_push(len(args))
            if len(args) > 0:
                self.wrln()
                self.generate_push_arguments(typ, len(args))
            self.generate_printf_call(name, typ, args)
            self.wrln()
            self.generate_call(name, len(args))
            self.generate_align_pop()
            self.generate_restore_registers()
            self.generate_epilogue()
            self.generate_ret()
        elif typ == 'v':
            self.generate_prologue()
            self.generate_store_registers()
            self.generate_align_push(len(args))
            assert len(args) > 0
            self.wrln()
            self.wrln('\t\tlea\t0x%x(%%ebp),%%eax' % ((len(args) + 1) * 4))
            self.wrln('\t\tpush\t%eax')
            self.generate_push_arguments(typ, len(args) - 1)
            self.generate_printf_call(name, typ, args)
            self.wrln()
            self.generate_call('v%s' % name, len(args))
            self.generate_align_pop()
            self.generate_restore_registers()
            self.generate_epilogue()
            self.generate_ret()
        else:
            print('Unknown wrapper type `%s\'' % typ, file=sys.stderr)

        if self.debug:
            self.generate_format_string(name, args)

    @staticmethod
    def arg_string(args):
        buf = StringIO()
        for n, arg in enumerate(args):
            if n > 0:
                buf.write(', ')
            if arg == 'd':
                buf.write('%lf')
            if arg == 'i':
                buf.write('%i')
            elif arg == 'x':
                buf.write('0x%x')
            elif arg == 'p':
                buf.write('%p')
            elif arg == 's':
                buf.write('\\"%s\\"')
            elif arg == 'c':
                buf.write('\'%c\'')
            elif arg == 'v':
                buf.write('...')
            elif arg == 'l':
                buf.write('(va_list) %p')
        return buf.getvalue()

    @staticmethod
    def open_file(name, mode='r'):
        if name == '-':
            if mode.find('r') >= 0:
                return sys.stdin
            return sys.stdout
        return open(name, mode)

    @staticmethod
    def close_file(file, name):
        if name == '-':
            return
        file.close()

    def append_rename_config(self, name):
        # Open file
        config_file = self.open_file(name)

        # Parse
        ln = 0
        while True:
            line = config_file.readline()
            if len(line) == 0:
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

    def main(self):
        try:
            opts, args = gnu_getopt(sys.argv[1:], 'ho:gur:s:',
                                    ('help', 'debug', 'underscore',
                                     'rename-config=', 'stack-align='))

        except GetoptError as message:
            print('Error:', message, file=sys.stderr)
            sys.exit(1)

        for opt, arg in opts:
            if opt in ('-h', '--help'):
                self.print_help()
                sys.exit(0)
            elif opt in ('-o',):
                self.output_filename = arg
            elif opt in ('-u', '--underscore'):
                self.underscore = True
            elif opt in ('-g', '--debug'):
                self.debug = True
            elif opt in ('-r', '--rename-config'):
                self.append_rename_config(arg)
            elif opt in ('-s', '--stack-align'):
                try:
                    self.stack_align = int(arg)
                    if (self.stack_align % 4 != 0 and self.stack_align != 1) \
                            or self.stack_align < 0:
                        raise ValueError
                except ValueError:
                    print('Error: Invalid stack alignment "%s".' % arg, file=sys.stderr)
                    sys.exit(1)

        if self.stack_align > 4:
            self.prologue_args += 1
            if self.debug:
                self.prologue_args += 1

        if len(args) == 1:
            self.input_filename = args[0]
        elif len(args) > 1:
            print('Error: Too many arguments', file=sys.stderr)
            sys.exit(1)

        self.input_file = self.open_file(self.input_filename)
        self.output_file = self.open_file(self.output_filename, 'w')

        self.wrln('/* Automatically generated by mkwrappers ' + '-- do not edit */\n')

        self.generate_wrappers()

        self.close_file(self.output_file, self.output_filename)
        self.close_file(self.input_file, self.input_filename)
        self.input_file = None
        self.output_file = None


MKWrappers().main()
