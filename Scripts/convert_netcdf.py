import netCDF4 as nc
import numpy as np
import struct
import sys

def read_netcdf_info(filename):
    """Read basic information from NetCDF file"""
    ds = nc.Dataset(filename)
    x = ds.variables['x'][:]
    y = ds.variables['y'][:]
    z = ds.variables['z'][:]
    dx = x[1] - x[0]
    dy = y[1] - y[0]
    origin_x = x[0] - dx / 2.0
    origin_y = y[0] - dy / 2.0
    boundary_x_max = origin_x + dx * len(x)
    boundary_y_max = origin_y + dy * len(y)
    
    return {
        'x': x,
        'y': y,
        'z': z,
        'dx': dx,
        'dy': dy,
        'origin_x': origin_x,
        'origin_y': origin_y,
        'boundary_x_max': boundary_x_max,
        'boundary_y_max': boundary_y_max
    }

def convert_netcdf_to_binary(input_file, output_file, target_nx, target_ny, 
                           force_boundaries=None):
    """
    Converts NetCDF file to binary format, matching TsunamiScenario's sampling logic.
    force_boundaries: Optional dictionary with x_min, x_max, y_min, y_max to force specific boundaries
    """
    print(f"\nProcessing {input_file} -> {output_file}")
    print(f"Target dimensions: {target_nx} x {target_ny}")
    
    # Read data
    info = read_netcdf_info(input_file)
    
    # Use forced boundaries if provided, otherwise use data boundaries
    if force_boundaries:
        origin_x = force_boundaries['x_min']
        origin_y = force_boundaries['y_min']
        boundary_x_max = force_boundaries['x_max']
        boundary_y_max = force_boundaries['y_max']
        print("Using forced boundaries:")
    else:
        origin_x = info['origin_x']
        origin_y = info['origin_y']
        boundary_x_max = info['boundary_x_max']
        boundary_y_max = info['boundary_y_max']
        print("Using natural boundaries:")
    
    print(f"  X: [{origin_x}, {boundary_x_max}]")
    print(f"  Y: [{origin_y}, {boundary_y_max}]")
    
    # Calculate new cell sizes
    new_dx = (boundary_x_max - origin_x) / target_nx
    new_dy = (boundary_y_max - origin_y) / target_ny
    
    print(f"Grid spacing:")
    print(f"  Original: dx={info['dx']}, dy={info['dy']}")
    print(f"  New: dx={new_dx}, dy={new_dy}")
    
    # Create new array
    z_new = np.zeros((target_ny, target_nx), dtype=np.float32)
    
    # Sample data using TsunamiScenario's exact method
    for j in range(target_ny):
        for i in range(target_nx):
            # Calculate physical coordinates
            x = origin_x + i * new_dx
            y = origin_y + j * new_dy
            
            # Calculate indices in original grid
            orig_i = int(round((x - info['origin_x']) / info['dx'] - 0.5))
            orig_j = int(round((y - info['origin_y']) / info['dy'] - 0.5))
            
            # Bounds check
            if (orig_i >= 0 and orig_i < len(info['x']) and 
                orig_j >= 0 and orig_j < len(info['y'])):
                z_new[j, i] = info['z'][orig_j, orig_i]
    
    print(f"Data range: [{z_new.min():.2f}, {z_new.max():.2f}]")
    
    # Write binary file
    with open(output_file, 'wb') as f:
        f.write(struct.pack('II', target_nx, target_ny))
        f.write(struct.pack('dd', origin_x, origin_y))
        f.write(struct.pack('dd', new_dx, new_dy))
        z_new.astype(np.float32).tofile(f)
    
    print(f"Successfully wrote {output_file}")
    return {
        'x_min': origin_x,
        'x_max': boundary_x_max,
        'y_min': origin_y,
        'y_max': boundary_y_max
    }

if __name__ == '__main__':
    
    # First read bathymetry boundaries
    bath_file = "tohoku_gebco_ucsb3_2000m_hawaii_bath.nc"
    displ_file = "tohoku_gebco_ucsb3_2000m_hawaii_displ.nc"
    
    print("Reading bathymetry boundaries...")
    bath_info = read_netcdf_info(bath_file)
    bath_bounds = {
        'x_min': bath_info['origin_x'],
        'x_max': bath_info['boundary_x_max'],
        'y_min': bath_info['origin_y'],
        'y_max': bath_info['boundary_y_max']
    }
    
    # Convert bathymetry first
    convert_netcdf_to_binary(bath_file, "tohoku_bath.bin", 700, 400)
    
    # Then convert displacement using bathymetry boundaries
    convert_netcdf_to_binary(displ_file, "tohoku_displ.bin", 250, 400, 
                            force_boundaries=bath_bounds)
    