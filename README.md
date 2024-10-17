[![test](https://github.com/wtsi-hpag/PretextGraph/actions/workflows/test.yml/badge.svg)](https://github.com/wtsi-hpag/PretextGraph/actions/workflows/test.yml)
[![Anaconda-Server Badge](https://anaconda.org/bioconda/pretext-suite/badges/installer/conda.svg)](https://conda.anaconda.org/bioconda)
[![Anaconda-Server Badge](https://anaconda.org/bioconda/pretextgraph/badges/downloads.svg)](https://anaconda.org/bioconda/pretextgraph)
# PretextGraph
Converts bedgraph formatted data and embeds inside a Pretext contact map.


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
PretextGraph reads bedgraph formatted data from `stdin`, e.g:<br/>
```bash
> zcat bedgraph.file.gz | PretextGraph -i input.pretext -n "graph name"
> bigWigToBedGraph bigwig.file /dev/stdout | PretextGraph -i input.pretext -n "graph name"
```
Important: only non-negative integer data is supported.


## Inject extensions in to an existing `.pretext` file
Usage: 
```bash
PretextGraph -i /path/to/input/.pretext/file -n "name_of_extension" [-o /path/to/the/output/.pretext/file] < /path/to/extension/data/file
```
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
And values in 4th column should be `int`. 

## Options
-i input Pretext file, required. Sequence names in the Pretext file must match sequence names in the bedgraph data; although relative sort order is unimportant.<br/>
-n graph name, required. A name for the graph.<br/>

-o output Pretext file, optional. If no output is specified the graph data will be appended to the input file.<br/>

# Requirments, running
4 cpu cores <br/>
128M RAM <br/>

# Viewing graphs
[PretextView](https://github.com/wtsi-hpag/PretextView) version 0.1.2 or later.

# Third-Party acknowledgements
PretextGraph uses the following third-party libraries:<br/>
* [libdeflate](https://github.com/ebiggers/libdeflate)<br/>
* [stb_sprintf.h](https://github.com/nothings/stb/blob/master/stb_sprintf.h)

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
make
cd ..
cmake
make
```