#include "input_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "benchmark.h"

#define INVALID_PARAM_MSG "%s: Parameter for option '-%c' invalid: %s\n"
#define INVALID_PARAM_MSG_LONG "%s: Parameter for option '--%s' invalid: %s\n"

#define OPT_LONG_OFFSET ('z' + 1)
#define OPT_COEFFS      (OPT_LONG_OFFSET)
#define OPT_BRIGHTNESS  (OPT_LONG_OFFSET + 1)
#define OPT_CONTRAST    (OPT_LONG_OFFSET + 2)
#define OPT_CSV         (OPT_LONG_OFFSET + 3)
#define OPT_TEST        (OPT_LONG_OFFSET + 4)
#define OPT_SQRT        (OPT_LONG_OFFSET + 5)

void print_help();

void print_usage_err();
int parse_coefficients(const char* exec_name, char* str, BCInput* input);

static struct option options[] =
{
  {"coeffs",     required_argument, NULL, OPT_COEFFS},
  {"brightness", required_argument, NULL, OPT_BRIGHTNESS},
  {"contrast",   required_argument, NULL, OPT_CONTRAST},
  {"csv",        no_argument,       NULL, OPT_CSV},
  {"test",       no_argument,       NULL, OPT_TEST},
  {"sqrt",       no_argument,       NULL, OPT_SQRT},
  {"help",       no_argument,       NULL, 'h'},
  {0, 0, 0, 0}
};

int parse_input(const int argc, char **argv, BCInput *input)
{
  bool brightness_set = false;
  bool contrast_set = false;
  bool benchmark_runs_set = false;

  while (true)
  {
    int options_idx;
    const int c = getopt_long(argc, argv, ":V:B::o:h", options, &options_idx);

    if (c == -1) // all arguments parsed
      break;

    switch (c)
    {
      case OPT_COEFFS:
      {
        if (parse_coefficients(argv[0], optarg, input))
        {
          print_usage_err();
          return 1;
        }

        const float coeff_sum = input->coeffs[0] + input->coeffs[1] + input->coeffs[2];
        const int inf = isinf(coeff_sum);
        if (inf)
        {
          fprintf(stderr, "%s: Sum of coefficients %s allowed value.\n", argv[0], (inf > 0 ? "exceeds maximum" : "less than minimum"));
          print_usage_err();
          return 1;
        }

        break;
      }

      case OPT_BRIGHTNESS:
      {
        if (parse_int16(optarg, &input->brightness))
        {
          fprintf(stderr, INVALID_PARAM_MSG_LONG, argv[0], options[OPT_BRIGHTNESS - OPT_LONG_OFFSET].name, optarg);
          print_usage_err();
          return 1;
        }

        if (input->brightness < -255 || input->brightness > 255)
        {
          fprintf(stderr, "%s: Brightness out of range\n", argv[0]);
          print_usage_err();
          return 1;
        }

        brightness_set = true;
        break;
      }

      case OPT_CONTRAST:
      {
        if (parse_float(optarg, &input->contrast))
        {
          fprintf(stderr, INVALID_PARAM_MSG_LONG, argv[0], options[OPT_CONTRAST - OPT_LONG_OFFSET].name, optarg);
          print_usage_err();
          return 1;
        }

        if (input->contrast < -255.0f || input->contrast > 255.0f)
        {
          fprintf(stderr, "%s: Contrast out of range\n", argv[0]);
          print_usage_err();
          return 1;
        }

        contrast_set = true;
        break;
      }

      case OPT_CSV:
      {
        input->benchmark_csv = true;
        break;
      }

      case OPT_TEST:
      {
        input->run_tests = true;
        input->test_delta = bc_default_test_delta;

        break;
      }

      case OPT_SQRT:
      {
        input->run_sqrt_tests_benchmark = true;
        return 0;
      }

      case 'V':
      {
        uint16_t version;

        if (parse_uint16(optarg, &version) || version >= BCImplMax)
        {
          fprintf(stderr, INVALID_PARAM_MSG, argv[0], c, optarg);
          print_usage_err();
          return 1;
        }

        input->impl = (BCImplVersion) version;

        break;
      }

      case 'B':
      {
        if (!optarg)
        {
          input->benchmark_runs = bc_default_benchmark_runs;
          break;
        }

        if (parse_uint32(optarg, &input->benchmark_runs))
        {
          fprintf(stderr, INVALID_PARAM_MSG, argv[0], c, optarg);
          print_usage_err();
          return 1;
        }

        benchmark_runs_set = true;
        break;
      }

      case 'o':
      {
        const size_t len = strlen(optarg) + 1;
        input->output_file = malloc(len);
        if (!input->output_file)
        {
          fprintf(stderr, "%s: Out of memory\n", argv[0]);
          return -1;
        }

        strncpy(input->output_file, optarg, len);
        break;
      }

      case 'h':
      {
        print_help();
        input->help_printed = true;
        return 0;
      }

      case ':':
      {
        if (optopt >= OPT_LONG_OFFSET) // long option
        {
          fprintf(stderr, "%s: option '--%s' is missing an argument\n", argv[0],
                  options[optopt - OPT_LONG_OFFSET].name); // calculate index from optopt because opt_index is not set in case of error
        }
        else // short option
        {
          fprintf(stderr, "%s: option '-%c' is missing an argument\n", argv[0], (char) optopt);
        }

        print_usage_err();
        return 1;
      }

      case '?':
      default:
      {
        fprintf(stderr, "%s: Unknown option '-%c'\n", argv[0], optopt);
        print_usage_err();
        return 1;
      }
    }
  }

  if (optind == argc - 1)
  {
    const size_t len = strlen(argv[optind]) + 1;
    input->input_file = malloc(len);
    if (!input->input_file)
    {
      fprintf(stderr, "%s: Out of memory\n", argv[0]);
      return -1;
    }

    strncpy(input->input_file, argv[optind], len);
  }
  else
  {
    fprintf(stderr, "%s: Expected exactly one positional argument\n", argv[0]);
    print_usage_err();
    return 1;
  }

  if (!input->output_file)
  {
    fprintf(stderr, "%s: Output file not specified\n", argv[0]);
    print_usage_err();
    return 1;
  }

  if (!brightness_set)
  {
    fprintf(stderr, "%s: Brightness not specified\n", argv[0]);
    print_usage_err();
    return 1;
  }

  if (!contrast_set)
  {
    fprintf(stderr, "%s: Contrast not specified\n", argv[0]);
    print_usage_err();
    return 1;
  }

  if (input->benchmark_csv && !benchmark_runs_set)
    input->benchmark_runs = bc_default_benchmark_runs;

  if (input->benchmark_runs > 0 && input->run_tests)
  {
    fprintf(stderr, "%s: Invalid combination of arguments - Tests and benchmark together cannot be executed.\n", argv[0]);
    print_usage_err();
    return 1;
  }

  return 0;
}

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

int parse_coefficients(const char* exec_name, char* str, BCInput* input)
{
  int coeffIdx = 0;

  char* str_coeff = strtok(str, ",");
  while (str_coeff)
  {
    if (parse_float(str_coeff, &input->coeffs[coeffIdx++]))
    {
      fprintf(stderr, "%s: Coefficient could not be converted: %s\n", exec_name, str_coeff);
      return 1;
    }

    str_coeff = strtok(NULL, ",");
  }

  if (coeffIdx != 3)
  {
    fprintf(stderr, "%s: Option '--%s' expects exactly three coeffs\n", exec_name, options[OPT_COEFFS - OPT_LONG_OFFSET].name);
    return 1;
  }

  return 0;
}

int parse_uint8(const char* str, uint8_t* param)
{
  errno = 0;

  char* endptr;
  unsigned long value = strtoul(str, &endptr, 10);

  if (endptr == str || *endptr != '\0' || errno == ERANGE || errno == EINVAL || value > UINT8_MAX)
    return 1;

  *param = (uint8_t) value;
  return 0;
}

int parse_uint16(const char* str, uint16_t* param)
{
  errno = 0;

  char* endptr;
  unsigned long value = strtoul(str, &endptr, 10);

  if (endptr == str || *endptr != '\0' || errno == ERANGE || errno == EINVAL || value > UINT16_MAX)
    return 1;

  *param = (uint16_t) value;
  return 0;
}

int parse_int16(const char* str, int16_t* param)
{
  errno = 0;

  char* endptr;
  long value = strtol(str, &endptr, 10);

  if (endptr == str || *endptr != '\0' || errno == ERANGE || errno == EINVAL || value > INT16_MAX || value < INT16_MIN)
    return 1;

  *param = (int16_t) value;
  return 0;
}

int parse_uint32(const char* str, uint32_t* param)
{
  errno = 0;

  char* endptr;
  unsigned long value = strtoul(str, &endptr, 10);

  if (endptr == str || *endptr != '\0' || errno == ERANGE || errno == EINVAL || value > UINT32_MAX)
    return 1;

  *param = (uint32_t) value;
  return 0;
}

int parse_float(const char* str, float* param)
{
  errno = 0;

  char* end_ptr;
  float value = strtof(str, &end_ptr);

  if (end_ptr == str || *end_ptr != '\0' || errno == ERANGE)
    return 1;

  *param = value;
  return 0;
}

int parse_size_t(const char* str, size_t* param)
{
  errno = 0;

  char* endptr;
  unsigned long value = strtoul(str, &endptr, 10);

  if (endptr == str || *endptr != '\0' || errno == ERANGE || errno == EINVAL)
    return 1;
  *param = value;
  return 0;
}
