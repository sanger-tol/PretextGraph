project('PretextGraph', ['c', 'cpp'],
    license: 'MIT',
    meson_version: '>=0.57.1',
    version: '0.1.0',
    default_options: ['buildtype=release', 'cpp_std=c++11'],
)

deps = [
    dependency('threads'), 
    # dependency('pybind11')
    ]

# UCSC bigWigToBedGraph for this OS. Installed next to the PretextGraph binaries.
custom_target('ucsc_bigWigToBedGraph',
    output: 'bigWigToBedGraph',
    command: [find_program('scripts/fetch_ucsc_bigWigToBedGraph.sh'), '@OUTPUT@'],
    build_by_default: true,
    install: true,
    install_dir: get_option('bindir'),
)
meson.add_install_script(find_program('scripts/install_ucsc_bigWigToBedGraph.sh'))
base_flags = ['-Ofast']
if get_option('buildtype') == 'debug'
    base_flags = ['-O0', '-g', '-DDEBUG']
    deps += [meson.get_compiler('cpp').find_library('m')]
endif
base_flags += ['-DPV=' + meson.project_version()]

if host_machine.cpu_family() == 'aarch64'
    extensions = {
        '.arm64': [],
    }
else
    extensions = {
        '.avx2': ['-mavx2'],
        '.avx': ['-mavx'],
        '.sse42': ['-msse4.2'],
        '.sse41': ['-msse4.1'],
    }
endif

foreach ext, flag : extensions
    flags = base_flags + flag
    executable('PretextGraph' + ext, [ 'PretextGraph.cpp' ], dependencies : deps, install : true, cpp_args : flags, link_with : 
        static_library('deflate' + ext, 'libdeflate/lib/deflate_compress.c', 'libdeflate/lib/deflate_decompress.c', 'libdeflate/lib/utils.c', c_args : flags, link_with : 
            [   static_library('deflate_x86' + ext, 'libdeflate/lib/x86/cpu_features.c', c_args : flags, include_directories : 'libdeflate'),
                static_library('deflate_arm' + ext, 'libdeflate/lib/arm/cpu_features.c', c_args : flags, include_directories : 'libdeflate')], include_directories : 'libdeflate'), include_directories : [ 'libdeflate', 'src' ])
endforeach

test('Test', executable('PretextGraph', 'SIMDExtensionWrapper.cpp', install : true, cpp_args : base_flags + ['-DProgramName=PretextGraph']))
