#include <stdio.h>
#include <stdlib.h>

#include "benchmark.h"
#include "brightness_contrast.h"
#include "brightness_contrast_test.h"
#include "image_io.h"
#include "input_parser.h"
#include "sqrt_test.h"

int main(const int argc, char **argv)
{
  int ret = EXIT_SUCCESS;
  size_t width;
  size_t height;
  uint8_t* source_image = NULL;
  uint8_t* result_image = NULL;
  BCInput input;

  bc_init_input(&input);
  if ((ret = parse_input(argc, argv, &input)) || input.help_printed)
  {
    // in case ret == 1, help/usage was already printed

    if (ret == -1)
      fprintf(stderr, "Failed to parse command line parameters\n");

    goto CLEANUP;
  }

  if (input.run_sqrt_tests_benchmark)
  {
    test_sqrt_heron(argv[0]);
    benchmark_sqrt(argv[0], 150000000); // hardcoded amount of runs, sorry
    goto CLEANUP;
  }
  
  if ((ret = read_source_image(input.input_file, &source_image, &width, &height, argv[0])))
  {
    if (ret == 1)
      fprintf(stderr, "Invalid input image\n");
    else
      fprintf(stderr, "Failed to read input image\n");

    goto CLEANUP;
  }

  if ((ret = alloc_image_pointer(&result_image, width, height, 1)))
    goto CLEANUP;

  if (input.benchmark_csv)
    benchmark_implementations_write_csv(&input, width, height, source_image, result_image, argv[0]);
  else if (input.benchmark_runs > 0)
    benchmark_implementation(&input, width, height, source_image, result_image, argv[0]);
  else if (input.run_tests)
    bc_test_implementations(&input, width, height, source_image, result_image, argv[0]);
  else
  {
    bc_implementation[input.impl].impl(source_image, width, height,
                                       input.coeffs[0], input.coeffs[1], input.coeffs[2],
                                       input.brightness, input.contrast, result_image);
    printf("%s: Conversion and brightness/contrast adjustment using %s successful.\n", argv[0], bc_implementation[input.impl].name);
  }

  if ((ret = write_to_res_img(input.output_file, result_image, width, height, argv[0])))
    goto CLEANUP;

CLEANUP:
  bc_destroy_input(&input);
  free(source_image);
  free(result_image);

  if (ret != 0)
    ret = EXIT_FAILURE;

  return ret;
}
