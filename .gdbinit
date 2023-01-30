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
end
