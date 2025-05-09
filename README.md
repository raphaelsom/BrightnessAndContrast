# Convert PPM to grayscale image, adjust brightness and contrast

## Usage

```bash
./BrightnessAndContrast.out <input_file> -o <output_file> \
  --brightness <brightness_value> --contrast <contrast_value> \
  [-V <implementation>] \
  [--coeffs <a,b,c>] \
  [-B[<runs>]] \
  [--csv] \
  [--test] \
  [--sqrt] \
  [-h | --help]
```

### Positional Arguments
- `<input_file>`  
  The image to be processed (.ppm (P6), 24bpp)

### Required Arguments
- `-o <output_file>`  
  Output file

- `--brightness <brightness_value>`  
  Brightness shift amount in `[-255, 255]` (integer)

- `--contrast <contrast_value>`  
  Contrast shift amount in `[-255.0, 255.0]` (floating point value)

### Optional Arguments
- `--coeffs <a,b,c>`  
  Coefficients of the grayscale conversion, where `a`, `b`, and `c` are floating point values

- `-V <version>`  
  Implementation to be used. Available implementations:  
  `0` .. Assembly SIMD (default)  
  `1` .. C SIMD  
  `2` .. Assembly SISD  
  `3` .. C SISD  
  `4` .. C SISD Multithreaded  
  `5` .. C SISD using Heron's method to approximate square roots  
  `6` .. C SISD using square root approximation making use of the IEEE-754 representation

- `-B[<runs>]`  
  Perform benchmark test. If `<runs>` is given: measure average over `<runs>` runs.  
  Default: `%u` runs

- `--csv`  
  Benchmark all implementations (impl. given via `-V` is ignored) and write result to CSV file  
  Each implementation is benchmarked `%u` times with an increasing image size, each time over a given number of runs  
  (can be specified via `-B`, else the default setting is used).  
  The result of `%s` is written to the output file.  
  Number of pixels starts with `(input image size / 2^6)` and is doubled on each benchmark.

- `--test`  
  Run tests of all implementations (either tests or benchmark can be executed, not both).  
  Tests are run with a maximum allowed delta of 1.  
  This delta counteracts floating point errors due to differing calculation orders.  
  The result of the default implementation is written to the output file.

- `--sqrt`  
  Test and benchmark all square root implementations.  
  Benchmark result is written to `%s`.  
  No brightness/contrast implementation is executed.

- `-h`, `--help`  
  Print help

## Benchmarking

Benchmarks were performed on the following system:

- Fedora Linux 40 (x86\_64)
- Linux-Kernel 6.12.8
- Intel i7-8565U (8) @ 4.6GHz
- 16GB RAM
- GCC 14.2.1

## Results

Averages times from 5000 runs of each implementation:

![](https://lh7-rt.googleusercontent.com/docsz/AD_4nXdVn462IDPGHJ5ZK23zy8VpcSJT4L5KLR9jvhpI18tT_NskUEElFWx0CgKZoJHnT2MGKQgrfLpujDKYCLdwEV4mBkgVcNEa-RdTZm6aLkV21Dlp0RSNPRQohEJ2dDgQEKudICsz?key=dFBzcnl_LVT2ELuXMID2ipUs)
*Figure 1: Benchmarking results*

### AVX-Optimizations

![](https://lh7-rt.googleusercontent.com/docsz/AD_4nXdC4O5avlGN541hZ8dJjy13SodL093ndPh3OF0CwfGT9kYfqon9pQcVphX3v3-tA2pKqs7qrEwEnogyVIprS7nuJcn_9cxnWJ5AjT4YIfz7l4cHtlmEw99r1TN2Qvv2kjTIpmE4?key=dFBzcnl_LVT2ELuXMID2ipUs)
*Figure 2: Benchmarking comparison AVX*
