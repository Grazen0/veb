#!/usr/bin/env bash
set -euo pipefail

OUTPUT_DIR="outputs"

meson compile -C build
mkdir -p "$OUTPUT_DIR"

build/veb_bench -r console -r xml::out="$OUTPUT_DIR/veb_bench.xml"
build/dijkstra_bench -r console -r xml::out="$OUTPUT_DIR/dijkstra.xml"
build/prim_bench -r console -r xml::out="$OUTPUT_DIR/prim.xml"
build/huffman_bench -r console -r xml::out="$OUTPUT_DIR/huffman.xml"
