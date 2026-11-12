#!/usr/bin/env python3
from plot_common import OUTPUTS, plot_structure_bars

if __name__ == "__main__":
    plot_structure_bars(OUTPUTS / "prim.xml", "Rendimiento de Prim vs. casos de prueba")
