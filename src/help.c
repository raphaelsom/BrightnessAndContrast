#include "help.h"

#include <stdio.h>

#include "benchmark.h"
#include "brightness_contrast.h"

void print_help()
{
  printf(
    "Help Message\n"
    "Positional arguments:\n"
      "\t<input_file>\n"
                "\t\tThe image to be processed (.ppm (P6), 24bpp)\n"
    "Required arguments:\n"
      "\t-o <output_file>\n"
                "\t\tOutput file\n"
      "\t--brightness <brightness_value>\n"
                "\t\tBrightness shift amount in [-255, 255] (integer)\n"
      "\t--contrast <contrast_value>\n"
                "\t\tContrast shift amount in [-255.0, 255.0] (floating point value)\n"
    "Optional arguments:\n"
      "\t--coeffs <a,b,c>\n"
                "\t\tCoefficients of the grayscale conversion, where a, b and c are floating point values\n"
      "\t-V <version>\n"
                "\t\tImplementation to be used. Available implementations:\n"
        "\t\t0 .. Assembly SIMD (default)\n"
        "\t\t1 .. C SIMD\n"
        "\t\t2 .. Assembly SISD\n"
        "\t\t3 .. C SISD\n"
        "\t\t4 .. C SISD Multithreaded\n"
        "\t\t5 .. C SISD using Heron's method to approximate square roots\n"
        "\t\t6 .. C SISD using square root approximation making use of the IEEE-754 representation\n"
      "\t-B[<runs>]\n"
                "\t\tPerform benchmark test. If <runs> is given: measure average over <runs> runs. Default: %u runs\n"
      "\t--csv\tBenchmark all implementations (impl. given via -V is ignored) and write result to CSV file\n"
                "\t\tEach implementation is benchmarked %u times with an increasing image size, each time over a given number of runs\n"
                "\t\t(can be specified via -B, else the default setting is used).\n"
      "\t\tThe result of %s is written to the output file.\n"
      "\t\tNumber of pixels starts with (input image size / 2^6) and is doubled on each benchmark.\n"
      "\t--test\tRun tests of all implementations (either tests or benchmark can be executed, not both).\n"
               "\t\tTests are run with a maximum allowed delta of 1.\n"
               "\t\tThe idea of this delta is to counteract the floating point errors due to possible different order of calculations in different implementations.\n"
               "\t\tThe result of the default implementation is written to the output file.\n"
      "\t--sqrt\tTest and benchmark all square root implementations. Benchmark result is written to %s.\n"
               "\t\tNo brightness/contrast-implementation is executed.\n"
      "\t-h, --help\n"
                "\t\tPrint help\n",
      bc_default_benchmark_runs,
      benchmark_iterations_per_implementation,
      bc_implementation[BCImplMax - 1].name,
      benchmark_csv_out_file
    );
}

void print_usage_err()
{
  fprintf(stderr, "Usage: ./BrightnessAndContrast.out <input_file> -o <output_file> "
                  "--brightness <brightness_value> --contrast <contrast_value> "
                  "[-V <implementation>] "
                  "[--coeffs <a,b,c>] "
                  "[-B[<runs>]] "
                  "[--csv] "
                  "[--test] "
                  "[--sqrt] "
                  "[-h/--help]\n");
}
