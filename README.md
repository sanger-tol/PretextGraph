[![test](https://github.com/wtsi-hpag/PretextGraph/actions/workflows/test.yml/badge.svg)](https://github.com/wtsi-hpag/PretextGraph/actions/workflows/test.yml)
[![Anaconda-Server Badge](https://anaconda.org/bioconda/pretext-suite/badges/installer/conda.svg)](https://conda.anaconda.org/bioconda)
[![Anaconda-Server Badge](https://anaconda.org/bioconda/pretextgraph/badges/downloads.svg)](https://anaconda.org/bioconda/pretextgraph)
# PretextGraph
Converts bedgraph formatted data and embeds inside a Pretext contact map.

Version 0.1.0


# Bioconda
**Note: currently seems not able to install this tool via Bioconda. (will be fixed soon I hope...)**

All commandline Pretext tools for Unix (Linux and Mac) are available on [bioconda](https://bioconda.github.io/).<br/>

The full suite of Pretext tools can be installed with
```sh
> conda install pretext-suite
```
Or, just PretextGraph can be installed with
```sh
> conda install pretextgraph
```

# Usage
One track, from stdin or a file:

```bash
zcat bedgraph.file.gz | PretextGraph -i input.pretext -n "graph name"
PretextGraph -i input.pretext -n "graph name" < /path/to/bedgraph.file
PretextGraph -i input.pretext -f /path/to/bedgraph.file -n "graph name" [-o output.pretext] [-nf 0|1]
```

Several tracks in one run. Each `-tN` file is one track and the matching `-nN` is its extension name. Do not mix this form with a bare `-n` or `-f`.

```bash
PretextGraph -i input.pretext \
  -t1 gap.bedgraph -n1 "gap" \
  -t2 telomere.bedgraph -n2 "telomere" \
  -t3 repeat_density.bigwig -n3 "repeat_density" \
  -t4 alg.bedgraph -n4 "alg" \
  -o output.pretext
```

`-i` input Pretext file, required. Sequence names in the Pretext file must match sequence names in the bedgraph data; although relative sort order is unimportant.<br/>
`-n` / `-nN` graph name. A name for the graph. Known names (`gap`, `telomere`, `repeat_density`, `coverage`, `coverage_avg`, `not_weighted`) select how values are accumulated. Any other name, such as `alg`, uses the default weighting and is still saved under that name.<br/>
`-tN` track file for the multi-track form. `.bedgraph` is read directly. `.bigwig` and `.bw` are converted as described below.<br/>
`-o` output Pretext file, optional. If no output is specified the graph data will be appended to the input file.<br/>
`-nf`: disable (0) or enable (1) the `noise_filter` (default: 1), which will only be applied to `coverage` or `repeat_density`, even this is enabled while process the `gap`, `coverage_average`.

## bigWig

`.bigwig` and `.bw` files are converted by running UCSC's `bigWigToBedGraph`, which must be on `PATH`. That program is a short wrapper over Jim Kent's C library in the UCSC Kent tree (`kent/src/lib`). It walks each chromosome with `bigWigIntervalQuery`, merges adjacent intervals that touch and share the same value, and prints them with `%g`. Calling that binary is what keeps the bedGraph identical to the UCSC tool.


## `.bedgraph` file format
NOTE: while using the `bedgraph` as the input for extension file, the newline character must be `\n`, and the separator must be `\t`. There is an example `repeat_density.bedgraph` [file](data_for_test). The file is as follows:
```
#1_usercol      2_usercol       3_usercol       N_density
chr1    0       10000   5107
chr1    10000   20000   3579
chr1    20000   30000   4850
chr1    30000   40000   2643
chr1    40000   50000   2309
chr1    50000   60000   3308
chr1    60000   70000   3605
chr1    70000   80000   3908
```
A numeric fourth column must be a non-negative integer and is stored under the `-n` name. A label in that column (`d1`, `d2`, `d3`, `d4`, `d5`, `d6`, `NA`, ...) stays in that same track. Each label is stored as its own positive integer, so PretextView colours the intervals by group. The run prints the mapping as `Group N: label`.

## NOTE: Special process different extensions

There are different extensions types, such as the `coverage`, `repeat_density`, `gap`...

Currently, there are 3 ways to transform the data into extension graphs:
```cpp
std::unordered_map<std::string, int> data_type_dic{  // use this data_type 
    {"default", 0, },
    {"repeat_density", 1},  // as this is counted in every single bin, so we need to normalise this within the bin
    {"gap", 2}, 
    {"coverage", 3},
    {"coverage_avg", 4},
    {"telomere", 5},
    {"not_weighted", 6}
};
```
- `0`: default, just add the weighted value of every bin to the `graph->values[index]`;
- `1`: `repeat_density`, before add the value of every bin to the `graph->values[index]`, the value is normlised by the `bin_size` as the `repeat.bedgraph` counts number of repeat bps in one single bin;
- `2`: `gap`, if there is gap in the related pixel, then `graph->values[index]` is set to `1` (no matter how many gaps), if no gaps within the range related with the pixel, the value is `0`.
- `3`: coverage, the weighted value of every bin is added to `graph->values[index]`
- `4`: averaged coverage, the weighted value of every bin is added to `graph->values[index]`
- `5`: telomere, the weighted value of every bin is added to `graph->values[index]`
- `6`: not_weighted, if `not` and `weighted` are both contained in the extension name, then the extension will not be weighted while accumulating the value. NOTE: while the supported maximum value for the extension is `u32` which should be not more than `2^32 - 1` or there will be un-expected problem.


# Requirments, running
4 cpu cores <br/>
128M RAM <br/>

# Viewing graphs
[PretextView](https://github.com/wtsi-hpag/PretextView) version 0.1.2 or later.

# Third-Party acknowledgements
PretextGraph uses the following third-party libraries:<br/>
* [libdeflate](https://github.com/ebiggers/libdeflate)<br/>
* [stb_sprintf.h](https://github.com/nothings/stb/blob/master/stb_sprintf.h)<br/>
* [bigWigToBedGraph](https://github.com/ucscGenomeBrowser/kent/tree/master/src/utils/bigWigToBedGraph), from Jim Kent's UCSC Kent utilities (`kent/src/lib`). PretextGraph runs this program to convert `.bigwig` and `.bw` files. [UCSC license](https://genome.ucsc.edu/license/).

# Installation

## meson
Requires:
* clang >= 11.0.0
* meson >= 0.57.1
```bash
git submodule update --init --recursive
env CXX=clang meson setup --buildtype=release --unity on --prefix=<installation prefix> builddir
cd builddir
meson compile
meson test
meson install
```

## cmake
```bash
git submodule update --init --recursive
cd libdeflate
make || exit 1
cd ..
cmake -DCMAKE_BUILD_TYPE=Release -B build_cmake || exit 1
cd build_cmake 
make 
```