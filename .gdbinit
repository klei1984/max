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
 
class PlotVector(PlotterBase):
    """
    Plot a flat contiguous vector/array of integers as a 2D heat map.

    Supports variable base integer types (int8/uint8/int16/uint16/int32/uint32),
    reading from:
      - std::vector<T>
      - raw T* pointer (or pointer field)
      - common members: m_values, m_data, values, data, buffer, m_field
      - as a fallback, index as a 2D array via val[x][y]

    Modes (same as PlotDistanceField):
      0 - BMP color heatmap (no dependencies)
      1 - Text file with decimal values
      2 - Multi-view: 2D heatmap + 3D surface (requires matplotlib/numpy)

    Example:
      py PlotVector("UnitsManager_TeamInfo[team].heat_map_complete", (112,112), 0).plot()
      py PlotVector("some_int_array_ptr", (W,H), 2).plot()
    """
    def __init__(self, name, dimensions, mode):
        self.name = name
        self.val = gdb.parse_and_eval(name)
        self.dim = dimensions
        self.mode = mode

    def _detect_data_ptr(self):
        """Return (data_ptr, elem_bits, is_unsigned) or (None, None, None) to fall back to val[x][y]."""
        # 1) std::vector<T>
        try:
            impl = self.val['_M_impl']
            start = impl['_M_start']
            tgt = start.type.target()
            elem_bits = tgt.sizeof * 8
            is_unsigned = 'unsigned' in str(tgt)
            return start, elem_bits, is_unsigned
        except Exception:
            pass

        # 2) Raw pointer
        try:
            if self.val.type.code == gdb.TYPE_CODE_PTR:
                tgt = self.val.type.target()
                elem_bits = tgt.sizeof * 8
                is_unsigned = 'unsigned' in str(tgt)
                return self.val, elem_bits, is_unsigned
        except Exception:
            pass

        # 3) Common member names
        candidates = ['m_values', 'm_data', 'values', 'data', 'buffer', 'm_field']
        try:
            for f in self.val.type.fields():
                fname = f.name
                if not fname:
                    continue
                if fname in candidates:
                    try:
                        sub = self.val[fname]
                    except Exception:
                        continue
                    # vector-like
                    try:
                        impl = sub['_M_impl']
                        start = impl['_M_start']
                        tgt = start.type.target()
                        elem_bits = tgt.sizeof * 8
                        is_unsigned = 'unsigned' in str(tgt)
                        return start, elem_bits, is_unsigned
                    except Exception:
                        pass
                    # pointer-like
                    try:
                        if sub.type.code == gdb.TYPE_CODE_PTR:
                            tgt = sub.type.target()
                            elem_bits = tgt.sizeof * 8
                            is_unsigned = 'unsigned' in str(tgt)
                            return sub, elem_bits, is_unsigned
                    except Exception:
                        pass
        except Exception:
            pass

        return None, None, None

    def _read_linear(self, ptr, idx):
        try:
            return int((ptr + idx).dereference())
        except Exception:
            return 0

    def _read_2d(self, x, y):
        # Try val[x][y] first (matches existing PlotMatrix convention)
        try:
            return int(self.val[x][y])
        except Exception:
            # Fallback to val[y][x]
            try:
                return int(self.val[y][x])
            except Exception:
                return 0

    def _normalize(self, data, min_val, max_val):
        # Special handling for heat_map_complete: emphasize presence vs absence
        if 'heat_map_complete' in self.name:
            # Compute non-zero max and fraction occupied
            total = self.dim[0] * self.dim[1]
            nonzero = sum(1 for row in data for v in row if v > 0)
            nz_max = max((v for row in data for v in row if v > 0), default=1)
            # Occupancy-focused scaling: 0 -> 0.0, nonzero -> 0.5..1.0
            if nz_max <= 0:
                return [[0.0 for _ in row] for row in data]
            scale = 0.5 / nz_max
            normalized = [[0.0 if v <= 0 else 0.5 + v * scale for v in row] for row in data]
            print(f"  heat_map_complete: occupied {nonzero}/{total} ({100*nonzero/total:.1f}%), max count={nz_max}")
            return normalized

        # General linear scaling
        if max_val > min_val:
            return [[(v - min_val) / (max_val - min_val) for v in row] for row in data]
        else:
            return [[0.0 for _ in row] for row in data]

    def _extract_data(self):
        width, height = self.dim[0], self.dim[1]
        data_ptr, elem_bits, is_unsigned = self._detect_data_ptr()

        data = []
        min_val = float('inf')
        max_val = float('-inf')

        if data_ptr is not None:
            for y in range(height):
                row = []
                base = y * width
                for x in range(width):
                    v = self._read_linear(data_ptr, base + x)
                    row.append(v)
                    if v < min_val: min_val = v
                    if v > max_val: max_val = v
                data.append(row)
        else:
            # 2D indexing fallback
            for y in range(height):
                row = []
                for x in range(width):
                    v = self._read_2d(x, y)
                    row.append(v)
                    if v < min_val: min_val = v
                    if v > max_val: max_val = v
                data.append(row)

        if min_val == float('inf'):
            min_val, max_val = 0, 1

        normalized = self._normalize(data, min_val, max_val)
        return data, normalized, min_val, max_val

    def plot(self):
        data, normalized, min_val, max_val = self._extract_data()
        width, height = self.dim[0], self.dim[1]

        # Derive output base name from expression
        base_name = self.name.replace('->', '.').split('.')[-1]
        base_name = base_name.replace('*', '').replace('&', '').strip()

        if self.mode == 1:
            filename = f"{base_name}.txt"
            with open(filename, "wt", encoding="utf-8") as f:
                f.write(f"# VectorPlot: {self.name}\n")
                f.write(f"# Data range: [{min_val}, {max_val}]\n")
                # Print rows with a sign-or-space separator:
                # replace the inter-value space with '-' for negative numbers to avoid shifting
                for y in range(height):
                    if width > 0:
                        f.write(str(int(data[y][0])))
                    for x in range(1, width):
                        v = int(data[y][x])
                        if v < 0:
                            f.write("-" + str(-v))
                        else:
                            f.write(" " + str(v))
                    f.write("\n")
            print(f"Saved {filename} (range: {min_val}-{max_val})")

        elif self.mode == 0:
            filename = f"{base_name}_heatmap.bmp"
            pixel_data = [[self._apply_colormap(normalized[y][x]) for x in range(width)] for y in range(height)]
            self._save_bmp_24bit(filename, width, height, pixel_data)
            print(f"Saved {filename} (range: {min_val}-{max_val})")

        elif self.mode == 2:
            try:
                import numpy as np
                import matplotlib
                matplotlib.use('Agg')
                import matplotlib.pyplot as plt
                from mpl_toolkits.mplot3d import Axes3D

                z = np.array(data, dtype=float)
                x = np.arange(0, width)
                y = np.arange(0, height)
                X, Y = np.meshgrid(x, y)

                fig = plt.figure(figsize=(16, 12))

                ax1 = fig.add_subplot(2, 2, 1)
                vmin = 0 if min_val >= 0 else min_val
                im1 = ax1.imshow(z, cmap='turbo', aspect='equal', origin='upper', vmin=vmin, vmax=max_val)
                ax1.set_title('Top-Down Heatmap', fontsize=14, fontweight='bold')
                ax1.set_xlabel('X')
                ax1.set_ylabel('Y')
                plt.colorbar(im1, ax=ax1, label='Value')

                ax2 = fig.add_subplot(2, 2, 2, projection='3d')
                ax2.plot_surface(X, Y, z, cmap='turbo', vmin=vmin, vmax=max_val, linewidth=0, antialiased=True, alpha=0.9)
                ax2.set_title('3D Surface (High Angle)', fontsize=14, fontweight='bold')
                ax2.set_xlabel('X')
                ax2.set_ylabel('Y')
                ax2.set_zlabel('Value')
                ax2.invert_xaxis()
                ax2.view_init(elev=60, azim=45)

                ax3 = fig.add_subplot(2, 2, 3, projection='3d')
                ax3.plot_surface(X, Y, z, cmap='turbo', vmin=vmin, vmax=max_val, linewidth=0, antialiased=True, alpha=0.9)
                ax3.set_title('Isometric View (Low Angle)', fontsize=14, fontweight='bold')
                ax3.set_xlabel('X')
                ax3.set_ylabel('Y')
                ax3.set_zlabel('Value')
                ax3.invert_yaxis()
                ax3.view_init(elev=25, azim=225)

                ax4 = fig.add_subplot(2, 2, 4)
                contour = ax4.contourf(X, Y, z, levels=20, cmap='turbo', vmin=vmin, vmax=max_val)
                ax4.contour(X, Y, z, levels=20, colors='black', linewidths=0.5, alpha=0.3)
                ax4.set_title('Contour Map', fontsize=14, fontweight='bold')
                ax4.set_xlabel('X')
                ax4.set_ylabel('Y')
                ax4.invert_yaxis()
                ax4.set_aspect('equal')
                plt.colorbar(contour, ax=ax4, label='Value')

                plt.suptitle(f'Vector Visualization: {self.name}\nRange: {min_val}-{max_val}', fontsize=16, fontweight='bold')
                plt.tight_layout()

                filename = f"{base_name}_multiview.png"
                plt.savefig(filename, dpi=150, bbox_inches='tight')
                plt.close()

                print(f"Saved {filename} (range: {min_val}-{max_val})")
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
    Plot TerrainDistanceField data (range² values with traversable/pending flags).

    Compatible with legacy and updated terraindistancefield layouts by:
      - Locating the contiguous data buffer from either a std::vector or common members
        (m_range_field, m_values, m_data, values, data, buffer)
      - Adapting bit masks based on 16-bit or 32-bit element size
      - Treating RANGE_MASK and RANGE_MASK|FLAG as pending/uninitialized
      - Treating FLAG|RANGE_MASK as "traversable-only" sentinel (shown as 1)

    Modes:
      0 - BMP color heatmap (no dependencies)
      1 - Text file with decimal values
      2 - Multi-view: 2D heatmap + 3D surface (requires matplotlib/numpy)

    Examples:
      py PlotDistanceField("terrain_distance_field.m_land_unit_range_field", (112,112), 0).plot()
      py PlotDistanceField("terrain_distance_field.m_water_unit_range_field", (112,112), 2).plot()
      py PlotDistanceField("terraindistancefield.m_range_field", (W,H), 0).plot()
    """
    def __init__(self, name, dimensions, mode):
        self.val = gdb.parse_and_eval(name)
        self.dim = dimensions
        self.mode = mode
        self.name = name

    def _detect_data_ptr_and_masks(self):
        """Find the underlying contiguous data pointer and choose masks by element size."""
        # Try direct std::vector layout
        try:
            impl = self.val['_M_impl']
            start = impl['_M_start']
            elem_bits = start.type.target().sizeof * 8
            return start, elem_bits
        except Exception:
            pass

        # Try raw pointer
        try:
            if self.val.type.code == gdb.TYPE_CODE_PTR:
                elem_bits = self.val.type.target().sizeof * 8
                return self.val, elem_bits
        except Exception:
            pass

        # Try common member names that may hold a vector or pointer
        candidates = ['m_range_field', 'm_values', 'm_data', 'values', 'data', 'buffer', 'm_field']
        try:
            for f in self.val.type.fields():
                fname = f.name
                if not fname:
                    continue
                if fname in candidates:
                    try:
                        sub = self.val[fname]
                    except Exception:
                        continue

                    # sub as vector
                    try:
                        impl = sub['_M_impl']
                        start = impl['_M_start']
                        elem_bits = start.type.target().sizeof * 8
                        return start, elem_bits
                    except Exception:
                        pass

                    # sub as raw pointer
                    try:
                        if sub.type.code == gdb.TYPE_CODE_PTR:
                            elem_bits = sub.type.target().sizeof * 8
                            return sub, elem_bits
                    except Exception:
                        pass
        except Exception:
            pass

        raise RuntimeError("PlotDistanceField: couldn't find a contiguous data buffer from '{}'".format(self.name))

    def _extract_data(self):
        """Extract and process TerrainDistanceField data with filtering."""
        data_ptr, elem_bits = self._detect_data_ptr_and_masks()

        # Choose masks from element size
        if elem_bits == 16:
            RANGE_MASK = 0x7FFF
            FLAG_MASK = 0x8000
        else:
            # Default to 32-bit encoding
            RANGE_MASK = 0x7FFFFFFF
            FLAG_MASK = 0x80000000

        MOVEMENT_RANGE_ONLY = (RANGE_MASK | FLAG_MASK)

        width, height = self.dim[0], self.dim[1]
        data = []
        min_val = float('inf')
        max_val = float('-inf')

        traversable_count = 0
        pending_count = 0
        computed_count = 0

        for y in range(height):
            row = []
            for x in range(width):
                idx = x + y * width
                try:
                    encoded = int((data_ptr + idx).dereference())
                except Exception:
                    encoded = 0

                # Pending/uninitialized
                if (encoded & RANGE_MASK) == RANGE_MASK:
                    decoded = 0
                    pending_count += 1
                # Traversable-only sentinel
                elif encoded == MOVEMENT_RANGE_ONLY:
                    decoded = 1
                    traversable_count += 1
                else:
                    decoded = (encoded & RANGE_MASK)
                    computed_count += 1
                    if decoded < min_val:
                        min_val = decoded
                    if decoded > max_val:
                        max_val = decoded

                row.append(decoded)
            data.append(row)

        total = width * height
        print(f"  Traversable cells: {traversable_count}/{total} ({100*traversable_count/total:.1f}%)")
        print(f"  Pending cells: {pending_count}/{total} ({100*pending_count/total:.1f}%)")
        print(f"  Computed range cells: {computed_count}/{total} ({100*computed_count/total:.1f}%)")

        if min_val == float('inf'):
            min_val = 0
            max_val = 1

        if max_val > min_val:
            normalized = [[(val - min_val) / (max_val - min_val) for val in row] for row in data]
        else:
            normalized = [[0.0 for _ in row] for row in data]

        return data, normalized, min_val, max_val

    def plot(self):
        data, normalized, min_val, max_val = self._extract_data()
        width, height = self.dim[0], self.dim[1]

        # Derive output base name from the expression tail
        base_name = self.name.replace('->', '.').split('.')[-1]
        base_name = base_name.replace('*', '').replace('&', '').strip()

        if self.mode == 1:
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
            filename = f"{base_name}_heatmap.bmp"
            pixel_data = [[self._apply_colormap(normalized[y][x]) for x in range(width)] for y in range(height)]
            self._save_bmp_24bit(filename, width, height, pixel_data)
            print(f"Saved {filename} (range²: {min_val}-{max_val})")

        elif self.mode == 2:
            try:
                import numpy as np
                import matplotlib
                matplotlib.use('Agg')
                import matplotlib.pyplot as plt
                from mpl_toolkits.mplot3d import Axes3D

                z = np.array(data, dtype=float)
                x = np.arange(0, width)
                y = np.arange(0, height)
                X, Y = np.meshgrid(x, y)

                fig = plt.figure(figsize=(16, 12))

                ax1 = fig.add_subplot(2, 2, 1)
                im1 = ax1.imshow(z, cmap='turbo', aspect='equal', origin='upper', vmin=0, vmax=max_val)
                ax1.set_title('Top-Down Heatmap (Linear Scale)', fontsize=14, fontweight='bold')
                ax1.set_xlabel('X')
                ax1.set_ylabel('Y')
                plt.colorbar(im1, ax=ax1, label='Range²')

                ax2 = fig.add_subplot(2, 2, 2, projection='3d')
                ax2.plot_surface(X, Y, z, cmap='turbo', vmin=0, vmax=max_val, linewidth=0, antialiased=True, alpha=0.9)
                ax2.set_title('3D Surface (High Angle)', fontsize=14, fontweight='bold')
                ax2.set_xlabel('X')
                ax2.set_ylabel('Y')
                ax2.set_zlabel('Range²')
                ax2.invert_xaxis()
                ax2.view_init(elev=60, azim=45)

                ax3 = fig.add_subplot(2, 2, 3, projection='3d')
                ax3.plot_surface(X, Y, z, cmap='turbo', vmin=0, vmax=max_val, linewidth=0, antialiased=True, alpha=0.9)
                ax3.set_title('Isometric View (Low Angle)', fontsize=14, fontweight='bold')
                ax3.set_xlabel('X')
                ax3.set_ylabel('Y')
                ax3.set_zlabel('Range²')
                ax3.invert_yaxis()
                ax3.view_init(elev=25, azim=225)

                ax4 = fig.add_subplot(2, 2, 4)
                contour = ax4.contourf(X, Y, z, levels=20, cmap='turbo', vmin=0, vmax=max_val)
                ax4.contour(X, Y, z, levels=20, colors='black', linewidths=0.5, alpha=0.3)
                ax4.set_title('Contour Map', fontsize=14, fontweight='bold')
                ax4.set_xlabel('X')
                ax4.set_ylabel('Y')
                ax4.invert_yaxis()
                ax4.set_aspect('equal')
                plt.colorbar(contour, ax=ax4, label='Range²')

                plt.suptitle(f'TerrainDistanceField: {self.name}\nRange²: {min_val}-{max_val}', fontsize=16, fontweight='bold')
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
