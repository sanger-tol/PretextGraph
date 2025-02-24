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
> PretextGraph -i input.pretext -n "graph name" < /path/to/bedgraph.file
> bigWigToBedGraph bigwig.file /dev/stdout | PretextGraph -i input.pretext -n "graph name"
```

```bash
PretextGraph -i /path/to/input/.pretext/file -n "name_of_extension" [-o /path/to/the/output/.pretext/file] [-nf 0|1] < /path/to/extension/data/file
``` 

`-i` input Pretext file, required. Sequence names in the Pretext file must match sequence names in the bedgraph data; although relative sort order is unimportant.<br/>
`-n` graph name, required. A name for the graph.<br/>
`-o` output Pretext file, optional. If no output is specified the graph data will be appended to the input file.<br/>
`-nf`: disable (0) or enable (1) the `noise_filter` (default: 1), which will only be applied to `coverage` or `repeat_density`, even this is enabled while process the `gap`, `coverage_average`.


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
***Important: only non-negative integer data is supported. All values in 4th column should be `int`.***

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
    {"telomere", 5}
};
```
- `0`: default, just add the weighted value of every bin to the `graph->values[index]`;
- `1`: `repeat_density`, before add the value of every bin to the `graph->values[index]`, the value is normlised by the `bin_size` as the `repeat.bedgraph` counts number of repeat bps in one single bin;
- `2`: `gap`, if there is gap in the related pixel, then `graph->values[index]` is set to `1` (no matter how many gaps), if no gaps within the range related with the pixel, the value is `0`.
- `3`: coverage, the weighted value of every bin is added to `graph->values[index]`
- `4`: averaged coverage, the weighted value of every bin is added to `graph->values[index]`
- `5`: telomere, the weighted value of every bin is added to `graph->values[index]`


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
make || exit 1
cd ..
cmake -DCMAKE_BUILD_TYPE=Release -B build_cmake || exit 1
cd build_cmake 
make 
```