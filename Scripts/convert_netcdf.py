import netCDF4 as nc
import numpy as np
import struct
import sys

def convert_netcdf_to_binary(input_file, output_file, target_nx=700, target_ny=400):
    # Read NetCDF file
    ds = nc.Dataset(input_file)
    
    # Get original dimensions
    x = ds.variables['x'][:]
    y = ds.variables['y'][:]
    z = ds.variables['z'][:].T  # Transpose to match C++ memory layout
    
    # Resample data to target dimensions
    from scipy.ndimage import zoom
    zoom_x = target_nx / z.shape[0]
    zoom_y = target_ny / z.shape[1]
    z_resampled = zoom(z, (zoom_x, zoom_y))
    
    # Calculate new grid parameters
    dx = (x[-1] - x[0]) / (target_nx - 1)
    dy = (y[-1] - y[0]) / (target_ny - 1)
    
    # Write binary file
    with open(output_file, 'wb') as f:
        # Write header
        f.write(struct.pack('II', target_nx, target_ny))
        f.write(struct.pack('dd', x[0], y[0]))
        f.write(struct.pack('dd', dx, dy))
        
        # Write data
        z_resampled.astype(np.float64).tofile(f)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python convert_netcdf.py input.nc output.bin")
        sys.exit(1)
    
    convert_netcdf_to_binary(sys.argv[1], sys.argv[2])