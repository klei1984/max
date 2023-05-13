set disassembly-flavor intel
skip -file smartpointer.hpp
skip -file smartobjectarray.hpp
skip -file smartstring.cpp
skip -file smartstring.hpp
skip -file sortedarray.hpp
skip -file sortedenum.hpp
skip -file smartarray.hpp
skip -file smartlist.hpp
skip -file point.hpp
skip -file ailog.cpp

python
class RegisterControl(gdb.printing.PrettyPrinter):
    def __init__(self, typename, printer):
        super().__init__(typename)
        self.printer = printer

    def __call__(self, val):
        if val.type.name == self.name:
            return self.printer(val)
        else:
            return None

def type_printer(typename):
    def _register_printer(printer):
        gdb.printing.register_pretty_printer(
            None,
            RegisterControl(typename, printer)
        )
    return _register_printer

@type_printer("Point")
class PrinterPoint:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return f"{self.val['x']}, {self.val['y']}"

@type_printer("SmartPointer")
class PrinterSmartPointer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return f"SmartPointer<{self.val['object_pointer'].type.target()}> containing {self.val['object_pointer'].dynamic_type}"

@type_printer("StringObject")
class PrinterStringObject:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return f"{self.val.address} \"{self.val['buffer'].string(length = self.val['length'])}\""

@type_printer("SmartString")
class PrinterSmartString:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return self.val['object_pointer'].dereference()

class PlotMatrix:
    """example: py PlotMatrix("this.map", (112,112), 0).plot()"""
    def __init__(self, name, dimensions, mode):
        self.val = gdb.parse_and_eval(name)
        self.dim = dimensions
        self.mode = mode

    def plot(self):
        if self.mode:
            with open("map.txt", "wt") as f:
                fmt = "{0:0" + str(len(str(pow(2,self.val[0][0].type.strip_typedefs().sizeof * 8)))) + "} "
                for y in range(self.dim[0]):
                    for x in range(self.dim[1]):
                        f.write(fmt.format(int(self.val[x][y])))
                    f.write("\n")
        else:
            with open("matrix.data", "wb") as f:
                min = 0
                max = 0
                for y in range(self.dim[0]):
                    for x in range(self.dim[1]):
                        value = int(self.val[x][y])
                        if min > value:
                            min = value
                        if max < value:
                            max = value
                for y in range(self.dim[0]):
                    for x in range(self.dim[1]):
                        value = int(self.val[x][y])
                        value = int(((value - min) * 255) / (max - min))
                        f.write(bytes([value]))

class PlotIndexedImage:
    """example: py PlotIndexedImage("image.buffer", (640,480)).plot()"""
    def __init__(self, name, dimensions):
        self.val = gdb.parse_and_eval(name)
        self.dim = dimensions

    def plot(self):
        with open("image.data", "wb") as f:
            for y in range(self.dim[1]):
                for x in range(self.dim[0]):
                    f.write(bytes([int(self.val[x + y * self.dim[0]])]))

class FunctionEnterBreakpoint(gdb.Breakpoint):
    def __init__(self, spec):
        self._function_name = spec
        super(FunctionEnterBreakpoint, self).__init__(spec, internal=True)

    def stop(self):
        max_depth = 3
        frame = gdb.newest_frame()

        gdb.write("\n", gdb.STDLOG)

        if "SetParent" in frame.function().name:
            gdb.write(f"Parent: {frame.read_var('parent')} ", gdb.STDLOG)

        if "GetParent" in frame.function().name:
            gdb.write(f"Parent: {gdb.parse_and_eval('this.parent_unit')} ", gdb.STDLOG)

        while frame and max_depth > 0:
            gdb.write(f"{'  ' * (3 - max_depth)}{frame.function().name} {frame.find_sal()}\n", gdb.STDLOG)
            frame = frame.older()
            max_depth -= 1

        return False

class TraceFunctionCommand(gdb.Command):
    """example: trace-function "UnitInfo::SetParent" """
    def __init__(self):
        super(TraceFunctionCommand, self).__init__(
            'trace-function',
            gdb.COMMAND_SUPPORT,
            gdb.COMPLETE_NONE,
            True)

    def invoke(self, symbol_name, from_tty):
            FunctionEnterBreakpoint(symbol_name)

TraceFunctionCommand()

class DumpSmartList:
    """example: py DumpSmartList("UnitsManager_StationaryUnits", "unit_type").dump() """
    def __init__(self, name, symbol):
        self.name = name
        self.symbol = symbol

    def dump(self):
        it = gdb.parse_and_eval(self.name + ".Begin()")

        with open(f"list_{self.name}.txt", "wt") as f:
            while(str(it["object_pointer"]) != "0x0"):
                node = it["object_pointer"]
                object = node["object"]["object_pointer"]
                if str(object) != "0x0":
                    f.write(f"{object[self.symbol]}\n")
                it = node["next"]
end
