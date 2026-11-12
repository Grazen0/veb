#!/usr/bin/env python3
from plot_common import OUTPUTS, plot_structure_bars

if __name__ == "__main__":
    plot_structure_bars(
        OUTPUTS / "dijkstra.xml", "Rendimiento de Dijkstra vs. casos de prueba"
    )
