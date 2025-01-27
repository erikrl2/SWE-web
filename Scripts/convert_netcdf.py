import netCDF4 as nc
import numpy as np
import struct
from dataclasses import dataclass
from typing import Optional, Dict
import inquirer

@dataclass
class GridConfig:
    nx: int
    ny: int
    origin_x: float  # in meters
    origin_y: float  # in meters
    domain_width: float  # in meters
    domain_height: float  # in meters
    input_file: str
    output_file: str

@dataclass
class ScenarioConfig:
    name: str
    bath_config: GridConfig
    displ_config: GridConfig

@dataclass
class DatasetInfo:
    x: np.ndarray  # in meters
    y: np.ndarray  # in meters
    z: np.ndarray
    dx: float  # in meters
    dy: float  # in meters
    origin_x: float  # in meters
    origin_y: float  # in meters
    boundary_x_max: float  # in meters
    boundary_y_max: float  # in meters
    width: float  # in meters
    height: float  # in meters
    nx: int
    ny: int

def read_netcdf_info(filename: str) -> DatasetInfo:
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
    
    return DatasetInfo(
        x=x, y=y, z=z, dx=dx, dy=dy,
        origin_x=origin_x, origin_y=origin_y,
        boundary_x_max=boundary_x_max,
        boundary_y_max=boundary_y_max,
        width=boundary_x_max - origin_x,
        height=boundary_y_max - origin_y,
        nx=len(x), ny=len(y)
    )

def get_effective_config(grid_config: GridConfig, info: DatasetInfo) -> GridConfig:
    return GridConfig(
        nx=info.nx if grid_config.nx == 0 else grid_config.nx,
        ny=info.ny if grid_config.ny == 0 else grid_config.ny,
        origin_x=info.origin_x if grid_config.origin_x == float('inf') else grid_config.origin_x,
        origin_y=info.origin_y if grid_config.origin_y == float('inf') else grid_config.origin_y,
        domain_width=info.width if grid_config.domain_width == 0 else grid_config.domain_width,
        domain_height=info.height if grid_config.domain_height == 0 else grid_config.domain_height,
        input_file=grid_config.input_file,
        output_file=grid_config.output_file
    )

def convert_netcdf_to_binary(grid_config: GridConfig) -> Dict[str, float]:
    print(f"\nProcessing {grid_config.input_file} -> {grid_config.output_file}")
    info = read_netcdf_info(grid_config.input_file)
    
    config = get_effective_config(grid_config, info)
    
    x_max = config.origin_x + config.domain_width
    y_max = config.origin_y + config.domain_height
    cell_size_x = config.domain_width / config.nx
    cell_size_y = config.domain_height / config.ny
    
    print(f"Grid configuration:")
    print(f"  Origin: ({config.origin_x/1000:.1f}, {config.origin_y/1000:.1f}) km")
    print(f"  Domain size: {config.domain_width/1000:.1f} x {config.domain_height/1000:.1f} km")
    print(f"  Cell size: ({cell_size_x/1000:.2f}, {cell_size_y/1000:.2f}) km")
    print(f"  Dimensions: {config.nx} x {config.ny} cells")
    
    z_new = np.zeros((config.ny, config.nx), dtype=np.float32)
    
    for j in range(config.ny):
        for i in range(config.nx):
            x = config.origin_x + i * cell_size_x
            y = config.origin_y + j * cell_size_y
            
            orig_i = int(round((x - info.origin_x) / info.dx - 0.5))
            orig_j = int(round((y - info.origin_y) / info.dy - 0.5))
            
            if (orig_i >= 0 and orig_i < len(info.x) and 
                orig_j >= 0 and orig_j < len(info.y)):
                z_new[j, i] = info.z[orig_j, orig_i]
    
    with open(config.output_file, 'wb') as f:
        f.write(struct.pack('II', config.nx, config.ny))
        f.write(struct.pack('dd', config.origin_x, config.origin_y))
        f.write(struct.pack('dd', cell_size_x, cell_size_y))
        z_new.astype(np.float32).tofile(f)
    
    return {
        'x_min': config.origin_x,
        'x_max': x_max,
        'y_min': config.origin_y,
        'y_max': y_max
    }

def get_scenario_configs() -> list[ScenarioConfig]:
    return [
        ScenarioConfig(
            name='Tohoku Downscale (700x400)',
            bath_config=GridConfig(
                nx=700, ny=400,
                origin_x=float('inf'), origin_y=float('inf'),
                domain_width=0, domain_height=0,
                input_file="tohoku_gebco_ucsb3_2000m_hawaii_bath.nc",
                output_file="tohoku_downscale_bath.bin"
            ),
            displ_config=GridConfig(
                nx=0, ny=0,
                origin_x=float('inf'), origin_y=float('inf'),
                domain_width=0, domain_height=0,
                input_file="tohoku_gebco_ucsb3_2000m_hawaii_displ.nc",
                output_file="tohoku_full_displ.bin"
            )
        ),
        ScenarioConfig(
            name='Tohoku Zoomed (700x400)',
            bath_config=GridConfig(
                nx=700, ny=400,
                origin_x=-500_000, origin_y=-450_000,  # in meters
                domain_width=1_400_000, domain_height=800_000,  # in meters
                input_file="tohoku_gebco_ucsb3_2000m_hawaii_bath.nc",
                output_file="tohoku_zoomed_bath.bin"
            ),
            displ_config=GridConfig(
                nx=0, ny=0,
                origin_x=float('inf'), origin_y=float('inf'),
                domain_width=0, domain_height=0,
                input_file="tohoku_gebco_ucsb3_2000m_hawaii_displ.nc",
                output_file="tohoku_full_displ.bin"
            )
        ),
        ScenarioConfig(
            name='Chile Downscale (800x600)',
            bath_config=GridConfig(
                nx=800, ny=600,
                origin_x=float('inf'), origin_y=float('inf'),
                domain_width=0, domain_height=0,
                input_file="chile_gebco_usgs_2000m_bath.nc",
                output_file="chile_downscale_bath.bin"
            ),
            displ_config=GridConfig(
                nx=555, ny=555,
                origin_x=float('inf'), origin_y=float('inf'),
                domain_width=0, domain_height=0,
                input_file="chile_gebco_usgs_2000m_displ.nc",
                output_file="chile_displ.bin"
            )
        )
    ]

def main():
    scenarios = get_scenario_configs()
    
    questions = [
        inquirer.List('scenario',
                     message="Select scenario",
                     choices=[s.name for s in scenarios])
    ]
    
    answers = inquirer.prompt(questions)
    selected = next(s for s in scenarios if s.name == answers['scenario'])
    
    convert_netcdf_to_binary(selected.bath_config)
    convert_netcdf_to_binary(selected.displ_config)

if __name__ == '__main__':
    main()