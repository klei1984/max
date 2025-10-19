set disassembly-flavor intel
set print elements 0
skip -rfunction '^std::'
skip -file smartpointer.hpp
skip -file smartobjectarray.hpp
skip -file smartstring.hpp
skip -file sortedarray.hpp
skip -file smartarray.hpp
skip -file fileobject.hpp
skip -file registerarray.cpp
skip -file registerarray.hpp
skip -file smartlist.hpp
skip -file point.hpp
skip -file ailog.cpp
skip regex std::.*
skip regex __gnu_cxx::.*
skip regex __cxxabiv1::.*

python
import sys
import gdb.printing
import subprocess

try:
    gcc_version = subprocess.check_output(['gcc', '-dumpfullversion'], encoding='utf-8').strip()
except Exception:
    gcc_version = subprocess.check_output(['gcc', '-dumpversion'], encoding='utf-8').strip()

sys.path.insert(0, sys.path[0] + '/../../gcc-' + gcc_version + '/python')
from libstdcxx.v6.printers import register_libstdcxx_printers
register_libstdcxx_printers (None)

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

class PlotterBase:
    """
    Base class for all plotting utilities with shared BMP and colormap functionality
    """
    
    def _apply_colormap(self, value):
        """Apply a temperature-style colormap (blue->cyan->green->yellow->red)"""
        if value < 0.25:
            # Blue to Cyan
            t = value / 0.25
            r = int(0 * (1-t) + 0 * t)
            g = int(0 * (1-t) + 255 * t)
            b = int(255 * (1-t) + 255 * t)
        elif value < 0.5:
            # Cyan to Green
            t = (value - 0.25) / 0.25
            r = int(0 * (1-t) + 0 * t)
            g = int(255 * (1-t) + 255 * t)
            b = int(255 * (1-t) + 0 * t)
        elif value < 0.75:
            # Green to Yellow
            t = (value - 0.5) / 0.25
            r = int(0 * (1-t) + 255 * t)
            g = int(255 * (1-t) + 255 * t)
            b = int(0 * (1-t) + 0 * t)
        else:
            # Yellow to Red
            t = (value - 0.75) / 0.25
            r = int(255 * (1-t) + 255 * t)
            g = int(255 * (1-t) + 0 * t)
            b = int(0 * (1-t) + 0 * t)
        
        return (r, g, b)

    def _save_bmp_24bit(self, filename, width, height, pixel_data):
        """Save 24-bit RGB BMP file (no dependencies needed)"""
        import struct
        
        # BMP requires rows to be padded to 4-byte boundaries
        row_size = width * 3
        padding = (4 - (row_size % 4)) % 4
        padded_row_size = row_size + padding
        
        # BMP header
        file_size = 54 + (padded_row_size * height)
        bmp_header = struct.pack('<2sIHHI', b'BM', file_size, 0, 0, 54)
        
        # DIB header (BITMAPINFOHEADER)
        dib_header = struct.pack('<IiiHHIIiiII',
            40,  # Header size
            width, height,
            1,   # Color planes
            24,  # Bits per pixel
            0,   # Compression (none)
            padded_row_size * height,
            2835, 2835,  # Print resolution (72 DPI)
            0, 0)  # Palette colors
        
        with open(filename, 'wb') as f:
            f.write(bmp_header)
            f.write(dib_header)
            
            # Write pixel data (BMP stores bottom-to-top)
            for y in range(height - 1, -1, -1):
                for x in range(width):
                    r, g, b = pixel_data[y][x]
                    f.write(struct.pack('BBB', b, g, r))  # BGR order
                # Write padding bytes
                f.write(b'\x00' * padding)

    def _save_bmp_8bit(self, filename, width, height, pixel_data, palette):
        """Save 8-bit indexed BMP file with custom palette"""
        import struct
        
        # BMP requires rows to be padded to 4-byte boundaries
        padding = (4 - (width % 4)) % 4
        padded_row_size = width + padding
        
        # BMP header
        palette_size = 256 * 4  # 256 colors × 4 bytes (BGRA)
        file_size = 54 + palette_size + (padded_row_size * height)
        bmp_header = struct.pack('<2sIHHI', b'BM', file_size, 0, 0, 54 + palette_size)
        
        # DIB header (BITMAPINFOHEADER)
        dib_header = struct.pack('<IiiHHIIiiII',
            40,  # Header size
            width, height,
            1,   # Color planes
            8,   # Bits per pixel (indexed)
            0,   # Compression (none)
            padded_row_size * height,
            2835, 2835,  # Print resolution (72 DPI)
            256, 0)  # Palette colors
        
        with open(filename, 'wb') as f:
            f.write(bmp_header)
            f.write(dib_header)
            
            # Write palette (256 BGRA entries)
            for i in range(256):
                if i < len(palette):
                    r, g, b = palette[i]
                else:
                    r = g = b = 0
                f.write(struct.pack('BBBB', b, g, r, 0))
            
            # Write pixel data (BMP stores bottom-to-top)
            for y in range(height - 1, -1, -1):
                for x in range(width):
                    f.write(struct.pack('B', pixel_data[y][x]))
                # Write padding bytes
                f.write(b'\x00' * padding)

    def _create_grayscale_palette(self):
        """Create a 256-color grayscale palette"""
        return [(i, i, i) for i in range(256)]

    def _create_gradient_palette(self):
        """Create a 256-color temperature gradient palette"""
        palette = []
        for i in range(256):
            value = i / 255.0
            r, g, b = self._apply_colormap(value)
            palette.append((r, g, b))
        return palette

class PlotMatrix(PlotterBase):
    """
    Plot 2D array/matrix data in various formats
    
    Modes:
      0 - BMP color heatmap (no dependencies)
      1 - Text file with decimal values
      2 - Multi-view: 2D heatmap + 3D surface (requires matplotlib/numpy)
    
    Examples:
      py PlotMatrix("this.map", (112,112), 0).plot()
      py PlotMatrix("access_map", (112,112), 2).plot()
    """
    def __init__(self, name, dimensions, mode):
        self.val = gdb.parse_and_eval(name)
        self.dim = dimensions
        self.mode = mode

    def _extract_data(self):
        """Extract data from 2D array and normalize to 0-1 range"""
        # Extract all values
        data = []
        min_val = float('inf')
        max_val = float('-inf')
        
        for y in range(self.dim[1]):
            row = []
            for x in range(self.dim[0]):
                value = int(self.val[x][y])
                row.append(value)
                if value < min_val:
                    min_val = value
                if value > max_val:
                    max_val = value
            data.append(row)
        
        # Normalize to 0-1 range
        if max_val > min_val:
            normalized = [[(val - min_val) / (max_val - min_val) for val in row] for row in data]
        else:
            normalized = [[0.0 for val in row] for row in data]
        
        return data, normalized, min_val, max_val

    def plot(self):
        data, normalized, min_val, max_val = self._extract_data()
        width, height = self.dim[0], self.dim[1]
        
        if self.mode == 1:
            # Text mode - output formatted decimal values
            with open("matrix.txt", "wt") as f:
                f.write(f"# Data range: [{min_val}, {max_val}]\n")
                fmt = "{0:0" + str(len(str(max_val))) + "} "
                for y in range(height):
                    for x in range(width):
                        f.write(fmt.format(data[y][x]))
                    f.write("\n")
            print(f"Saved matrix.txt (range: {min_val}-{max_val})")
        
        elif self.mode == 0:
            # BMP color heatmap (no dependencies)
            pixel_data = [[self._apply_colormap(normalized[y][x]) 
                          for x in range(width)] 
                          for y in range(height)]
            self._save_bmp_24bit("matrix_heatmap.bmp", width, height, pixel_data)
            print(f"Saved matrix_heatmap.bmp (range: {min_val}-{max_val})")
        
        elif self.mode == 2:
            # Multi-view with matplotlib (requires matplotlib/numpy)
            try:
                import numpy as np
                import matplotlib
                matplotlib.use('Agg')  # Non-interactive backend
                import matplotlib.pyplot as plt
                from mpl_toolkits.mplot3d import Axes3D
                
                # Convert to numpy array
                z = np.array(data, dtype=float)
                x = np.arange(0, width)
                y = np.arange(0, height)
                X, Y = np.meshgrid(x, y)
                
                # Create figure with 2x2 subplots
                fig = plt.figure(figsize=(16, 12))
                
                # 1. Top-down heatmap (REFERENCE - correct orientation)
                ax1 = fig.add_subplot(2, 2, 1)
                im1 = ax1.imshow(z, cmap='turbo', aspect='equal', origin='upper',
                                vmin=min_val, vmax=max_val)
                ax1.set_title('Top-Down Heatmap', fontsize=14, fontweight='bold')
                ax1.set_xlabel('X')
                ax1.set_ylabel('Y')
                plt.colorbar(im1, ax=ax1, label='Value')
                
                # 2. 3D Surface plot (high angle - flatter perspective)
                ax2 = fig.add_subplot(2, 2, 2, projection='3d')
                surf = ax2.plot_surface(X, Y, z, cmap='turbo', 
                                       vmin=min_val, vmax=max_val,
                                       linewidth=0, antialiased=True, alpha=0.9)
                ax2.set_title('3D Surface (High Angle)', fontsize=14, fontweight='bold')
                ax2.set_xlabel('X')
                ax2.set_ylabel('Y')
                ax2.set_zlabel('Value')
                ax2.invert_xaxis()  # Flip X to match top-down view
                ax2.view_init(elev=60, azim=45)  # Higher elevation = flatter view
                
                # 3. Isometric-style 3D view (low angle - more dramatic depth)
                ax3 = fig.add_subplot(2, 2, 3, projection='3d')
                surf2 = ax3.plot_surface(X, Y, z, cmap='turbo',
                                        vmin=min_val, vmax=max_val,
                                        linewidth=0, antialiased=True, alpha=0.9)
                ax3.set_title('Isometric View (Low Angle)', fontsize=14, fontweight='bold')
                ax3.set_xlabel('X')
                ax3.set_ylabel('Y')
                ax3.set_zlabel('Value')
                ax3.invert_yaxis()  # Flip Y to match top-down view
                ax3.view_init(elev=25, azim=225)  # Lower elevation = more dramatic
                
                # 4. Contour plot (flipped to match heatmap orientation)
                ax4 = fig.add_subplot(2, 2, 4)
                contour = ax4.contourf(X, Y, z, levels=20, cmap='turbo',
                                      vmin=min_val, vmax=max_val)
                ax4.contour(X, Y, z, levels=20, colors='black', linewidths=0.5, alpha=0.3)
                ax4.set_title('Contour Map', fontsize=14, fontweight='bold')
                ax4.set_xlabel('X')
                ax4.set_ylabel('Y')
                ax4.invert_yaxis()  # Flip Y to match top-down view
                ax4.set_aspect('equal')
                plt.colorbar(contour, ax=ax4, label='Value')
                
                plt.suptitle(f'Matrix Visualization (Range: {min_val}-{max_val})', 
                            fontsize=16, fontweight='bold')
                plt.tight_layout()
                plt.savefig('matrix_multiview.png', dpi=150, bbox_inches='tight')
                plt.close()
                
                print(f"Saved matrix_multiview.png (range: {min_val}-{max_val})")
                print("  Contains: 2D heatmap, 3D surface, isometric view, contour map")
                
            except ImportError as e:
                print(f"Mode 2 requires matplotlib and numpy: {e}")
                print("Falling back to BMP mode...")
                self.mode = 0
                self.plot()
        
        else:
            print(f"Unknown mode {self.mode}. Use 0 (BMP), 1 (text), or 2 (multi-view)")

class PlotDistanceField(PlotterBase):
    """
    Plot TerrainDistanceField data (range² values with traversable flags)
    
    Handles TerrainDistanceField-specific encoding:
    - Bits 0-30 (RANGE_MASK): Range² value or pending marker (0x7FFFFFFF)
    - Bit 31 (TRAVERSABLE_FLAG): Metadata flag (meaning unclear, not used for visualization)
    - Filters out pending (RANGE_MASK = 0x7FFFFFFF) cells to avoid polluting visualization
    - Displays all other range² values regardless of flag state
    - Linear scaling of range² values for visualization
    
    Modes:
      0 - BMP color heatmap (no dependencies)
      1 - Text file with decimal values
      2 - Multi-view: 2D heatmap + 3D surface (requires matplotlib/numpy)
    
    Examples:
      py PlotDistanceField("terrain_distance_field.m_land_unit_range_field", (112,112), 0).plot()
      py PlotDistanceField("terrain_distance_field.m_water_unit_range_field", (112,112), 2).plot()
    """
    def __init__(self, name, dimensions, mode):
        self.val = gdb.parse_and_eval(name)
        self.dim = dimensions
        self.mode = mode
        self.name = name
        
        # TerrainDistanceField constants
        self.RANGE_MASK = 0x7FFFFFFF
        self.TRAVERSABLE_FLAG = 0x80000000

    def _extract_data(self):
        """Extract and process TerrainDistanceField data with filtering"""
        # Access the underlying array in std::vector
        vec_impl = self.val['_M_impl']
        vec_start = vec_impl['_M_start']
        
        # Extract all values and decode TerrainDistanceField encoding
        data = []
        min_val = float('inf')
        max_val = float('-inf')
        
        # Statistics for debugging
        traversable_count = 0
        pending_count = 0
        computed_count = 0
        
        for y in range(self.dim[1]):
            row = []
            for x in range(self.dim[0]):
                idx = x + y * self.dim[0]
                encoded_value = int((vec_start + idx).dereference())
                
                # Check for MOVEMENT_RANGE_ONLY (0xFFFFFFFF) - traversable terrain
                if encoded_value == (self.TRAVERSABLE_FLAG | self.RANGE_MASK):
                    # Traversable - display as 1 (minimum non-zero range)
                    decoded = 1
                    traversable_count += 1
                else:
                    # Extract range² from bits 0-30
                    range_value = encoded_value & self.RANGE_MASK
                    decoded = range_value
                    
                    # Track statistics
                    if range_value == self.RANGE_MASK:
                        pending_count += 1
                    else:
                        computed_count += 1
                    
                    # Track min/max for normalization (exclude traversable=1)
                    if decoded < min_val:
                        min_val = decoded
                    if decoded > max_val:
                        max_val = decoded
                
                row.append(decoded)
            data.append(row)
        
        # Print diagnostics
        total = self.dim[0] * self.dim[1]
        print(f"  Traversable cells: {traversable_count}/{total} ({100*traversable_count/total:.1f}%)")
        print(f"  Pending cells: {pending_count}/{total} ({100*pending_count/total:.1f}%)")
        print(f"  Computed range cells: {computed_count}/{total} ({100*computed_count/total:.1f}%)")
        
        # Handle edge case where all cells are traversable or pending
        if min_val == float('inf'):
            min_val = 0
            max_val = 1
        
        # Normalize to 0-1 range (linear scaling)
        if max_val > min_val:
            normalized = [[(val - min_val) / (max_val - min_val) for val in row] for row in data]
        else:
            normalized = [[0.0 for val in row] for row in data]
        
        return data, normalized, min_val, max_val

    def plot(self):
        data, normalized, min_val, max_val = self._extract_data()
        width, height = self.dim[0], self.dim[1]
        
        # Extract base filename from rightmost part of variable name
        # e.g., "terrain_distance_field.m_land_unit_range_field" -> "m_land_unit_range_field"
        # Handle both . and -> separators, sanitize for valid filenames
        base_name = self.name.replace('->', '.').split('.')[-1]
        # Remove any remaining invalid filename characters
        base_name = base_name.replace('*', '').replace('&', '').strip()
        
        if self.mode == 1:
            # Text mode - output formatted decimal values
            filename = f"{base_name}.txt"
            with open(filename, "wt", encoding="utf-8") as f:
                f.write(f"# TerrainDistanceField: {self.name}\n")
                f.write(f"# Data range (range²): [{min_val}, {max_val}]\n")
                fmt = "{0:0" + str(len(str(max_val))) + "} "
                for y in range(height):
                    for x in range(width):
                        f.write(fmt.format(data[y][x]))
                    f.write("\n")
            print(f"Saved {filename} (range²: {min_val}-{max_val})")
        
        elif self.mode == 0:
            # BMP color heatmap (no dependencies)
            filename = f"{base_name}_heatmap.bmp"
            pixel_data = [[self._apply_colormap(normalized[y][x]) 
                          for x in range(width)] 
                          for y in range(height)]
            self._save_bmp_24bit(filename, width, height, pixel_data)
            print(f"Saved {filename} (range²: {min_val}-{max_val})")
        
        elif self.mode == 2:
            # Multi-view with matplotlib (requires matplotlib/numpy)
            try:
                import numpy as np
                import matplotlib
                matplotlib.use('Agg')  # Non-interactive backend
                import matplotlib.pyplot as plt
                from mpl_toolkits.mplot3d import Axes3D
                
                # Convert to numpy array (use original data, not normalized)
                z = np.array(data, dtype=float)
                x = np.arange(0, width)
                y = np.arange(0, height)
                X, Y = np.meshgrid(x, y)
                
                # Create figure with 2x2 subplots
                fig = plt.figure(figsize=(16, 12))
                
                # 1. Top-down heatmap (REFERENCE - correct orientation)
                ax1 = fig.add_subplot(2, 2, 1)
                im1 = ax1.imshow(z, cmap='turbo', aspect='equal', origin='upper',
                                vmin=0, vmax=max_val)
                ax1.set_title('Top-Down Heatmap (Linear Scale)', fontsize=14, fontweight='bold')
                ax1.set_xlabel('X')
                ax1.set_ylabel('Y')
                plt.colorbar(im1, ax=ax1, label='Range² Value')
                
                # 2. 3D Surface plot (high angle - flatter perspective)
                ax2 = fig.add_subplot(2, 2, 2, projection='3d')
                surf = ax2.plot_surface(X, Y, z, cmap='turbo', 
                                       vmin=0, vmax=max_val,
                                       linewidth=0, antialiased=True, alpha=0.9)
                ax2.set_title('3D Surface (High Angle)', fontsize=14, fontweight='bold')
                ax2.set_xlabel('X')
                ax2.set_ylabel('Y')
                ax2.set_zlabel('Range²')
                ax2.invert_xaxis()  # Flip X to match top-down view
                ax2.view_init(elev=60, azim=45)  # Higher elevation = flatter view
                
                # 3. Isometric-style 3D view (low angle - more dramatic depth)
                ax3 = fig.add_subplot(2, 2, 3, projection='3d')
                surf2 = ax3.plot_surface(X, Y, z, cmap='turbo',
                                        vmin=0, vmax=max_val,
                                        linewidth=0, antialiased=True, alpha=0.9)
                ax3.set_title('Isometric View (Low Angle)', fontsize=14, fontweight='bold')
                ax3.set_xlabel('X')
                ax3.set_ylabel('Y')
                ax3.set_zlabel('Range²')
                ax3.invert_yaxis()  # Flip Y to match top-down view
                ax3.view_init(elev=25, azim=225)  # Lower elevation = more dramatic
                
                # 4. Contour plot (flipped to match heatmap orientation)
                ax4 = fig.add_subplot(2, 2, 4)
                contour = ax4.contourf(X, Y, z, levels=20, cmap='turbo',
                                      vmin=0, vmax=max_val)
                ax4.contour(X, Y, z, levels=20, colors='black', linewidths=0.5, alpha=0.3)
                ax4.set_title('Contour Map', fontsize=14, fontweight='bold')
                ax4.set_xlabel('X')
                ax4.set_ylabel('Y')
                ax4.invert_yaxis()  # Flip Y to match top-down view
                ax4.set_aspect('equal')
                plt.colorbar(contour, ax=ax4, label='Range²')
                
                plt.suptitle(f'TerrainDistanceField: {self.name}\nRange²: {min_val}-{max_val}', 
                            fontsize=16, fontweight='bold')
                plt.tight_layout()
                
                filename = f"{base_name}_multiview.png"
                plt.savefig(filename, dpi=150, bbox_inches='tight')
                plt.close()
                
                print(f"Saved {filename} (range²: {min_val}-{max_val})")
                print("  Contains: 2D heatmap, 3D surface, isometric view, contour map")
                
            except ImportError as e:
                print(f"Mode 2 requires matplotlib and numpy: {e}")
                print("Falling back to BMP mode...")
                self.mode = 0
                self.plot()
        
        else:
            print(f"Unknown mode {self.mode}. Use 0 (BMP), 1 (text), or 2 (multi-view)")

class PlotIndexedImage(PlotterBase):
    """
    Plot indexed/palette-based image data in various formats
    
    Modes:
      0 - BMP with grayscale palette (no dependencies)
      1 - Raw indexed data (original behavior)
      2 - BMP with color gradient palette (no dependencies)
    
    Examples:
      py PlotIndexedImage("image.buffer", (640,480), 0).plot()
      py PlotIndexedImage("sprite_data", (32,32), 2).plot()
    """
    def __init__(self, name, dimensions, mode=1):
        self.val = gdb.parse_and_eval(name)
        self.dim = dimensions
        self.mode = mode

    def _extract_data(self):
        """Extract indexed image data"""
        data = []
        min_val = 255
        max_val = 0
        
        for y in range(self.dim[1]):
            row = []
            for x in range(self.dim[0]):
                value = int(self.val[x + y * self.dim[0]])
                row.append(value)
                if value < min_val:
                    min_val = value
                if value > max_val:
                    max_val = value
            data.append(row)
        
        return data, min_val, max_val

    def plot(self):
        data, min_val, max_val = self._extract_data()
        width, height = self.dim[0], self.dim[1]
        
        if self.mode == 1:
            # Raw indexed data (original behavior)
            with open("image.data", "wb") as f:
                for y in range(height):
                    for x in range(width):
                        f.write(bytes([data[y][x]]))
            print(f"Saved image.data (indexed values: {min_val}-{max_val})")
        
        elif self.mode == 0:
            # BMP with grayscale palette
            palette = self._create_grayscale_palette()
            self._save_bmp_8bit("image_grayscale.bmp", width, height, data, palette)
            print(f"Saved image_grayscale.bmp (indexed values: {min_val}-{max_val})")
        
        elif self.mode == 2:
            # BMP with color gradient palette
            palette = self._create_gradient_palette()
            self._save_bmp_8bit("image_gradient.bmp", width, height, data, palette)
            print(f"Saved image_gradient.bmp (indexed values: {min_val}-{max_val})")
        
        else:
            print(f"Unknown mode {self.mode}. Use 0 (grayscale BMP), 1 (raw), or 2 (gradient BMP)")

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
