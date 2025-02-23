#include "benchmark.h"

#include <stdio.h>
#include <time.h>
#include <math.h>

#include "brightness_contrast.h"
#include "math_utils.h"

#define IT_PER_IMPL 7

const char* benchmark_csv_out_file = "benchmark.csv";

const uint8_t benchmark_iterations_per_implementation = IT_PER_IMPL;

struct bench_result
{
  BCImplementation impl;
  size_t pixels;
  double times[2]; // [0] .. total | [1] .. avg
};

static int benchmark_write_csv(struct bench_result results[BCImplMax][IT_PER_IMPL], const char* prog_name);
static void benchmark_sqrt_internal(const char* name, const size_t runs, float (*sqrt_func)(float));

static void benchmark_implementation_internal(const BCInput *input, const size_t width, const size_t height,
                                              const uint8_t *source_image, uint8_t *result_image,
                                              double *time_elapsed, double *time_avg)
{
  struct timespec time_start;
  struct timespec time_end;

  clock_gettime(CLOCK_MONOTONIC, &time_start);

  for (uint32_t i = 0; i < input->benchmark_runs; ++i)
  {
    bc_implementation[input->impl].impl(source_image, width, height,
                                        input->coeffs[0], input->coeffs[1], input->coeffs[2],
                                        input->brightness, input->contrast, result_image);
  }

  clock_gettime(CLOCK_MONOTONIC, &time_end);

  *time_elapsed = (double) (time_end.tv_sec - time_start.tv_sec) +
                  1e-9 * (double) (time_end.tv_nsec - time_start.tv_nsec);
  *time_avg = *time_elapsed / input->benchmark_runs;
}

void benchmark_implementation(const BCInput *input, const size_t width, const size_t height,
                              const uint8_t *source_image, uint8_t *result_image, const char* prog_name)
{
  double time_elapsed, time_avg;

  printf("%s: Benchmarking %s over %u runs...\n", prog_name, bc_implementation[input->impl].name, input->benchmark_runs);

  benchmark_implementation_internal(input, width, height, source_image, result_image, &time_elapsed, &time_avg);

  printf("========== Benchmark Results ==========\n");
  printf("Number of runs      : %d\n", input->benchmark_runs);
  printf("Implementation used : %s\n", bc_implementation[input->impl].name);
  printf("Input size          : %lux%lu = %lu pixels\n", width, height, width * height);
  printf("Total time elapsed  : %.6f seconds\n", time_elapsed);
  printf("Average time per run: %.6f seconds\n", time_avg);
}

int benchmark_implementations_write_csv(BCInput *input, const size_t width, const size_t height,
                                        const uint8_t *source_image, uint8_t *result_image, const char* prog_name)
{
  struct bench_result results[BCImplMax][IT_PER_IMPL];

  printf("%s: Running benchmark of all implementations on %u different image sizes, %d runs each\n",
          prog_name, benchmark_iterations_per_implementation, input->benchmark_runs);

  const float start_width = (float) width / powf(2.0f, benchmark_iterations_per_implementation - 1);
  const float start_height = (float) height / powf(2.0f, benchmark_iterations_per_implementation - 1);
  for (int impl = 0; impl < BCImplMax; ++impl)
  {
    input->impl = impl;
    printf("Benchmarking %s...\n", bc_implementation[impl].name);

    float curr_width = start_width;
    float curr_height = start_height;
    for (uint8_t i = 0; i < benchmark_iterations_per_implementation; ++i)
    {
      benchmark_implementation_internal(input, (size_t) curr_width, (size_t) curr_height, source_image, result_image,
        &results[impl][i].times[0], &results[impl][i].times[1]);

      results[impl][i].pixels = (size_t) curr_width * (size_t) curr_height;

      curr_width *= 2;
      curr_height *= 2;
    }
  }

  if (benchmark_write_csv(results, prog_name))
    return -1;

  printf("Successfully benchmarked %d runs of all implementations. Results stored in %s.\n",
          input->benchmark_runs, benchmark_csv_out_file);
  return 0;
}

static int benchmark_write_csv(struct bench_result results[BCImplMax][IT_PER_IMPL], const char* prog_name)
{
  int ret = 0;
  FILE* csv = fopen(benchmark_csv_out_file, "w+");
  if (!csv)
  {
    fprintf(stderr, "%s: Couldn't open %s\n", prog_name, benchmark_csv_out_file);
    return -1;
  }

  fprintf(csv, "Implementation,Pixels,Total,Average\n");

  for (int impl = 0; impl < BCImplMax; ++impl)
  {
    for (uint8_t it = 0; it < benchmark_iterations_per_implementation; ++it)
    {
      if (fprintf(csv, "%s,%lu,%f,%f\n",
           bc_implementation[impl].name, results[impl][it].pixels, results[impl][it].times[0], results[impl][it].times[1]) < 0)
      {
        fprintf(stderr, "%s: Error writing CSV\n", prog_name);
        ret = -1;
        goto END;
      }
    }
  }

END:
  fclose(csv);
  return ret;
}

void benchmark_sqrt(const char* prog_name, const size_t runs)
{
  printf("%s: Benchmarking sqrt implementations over %lu runs...\n", prog_name, runs);

  benchmark_sqrt_internal("sqrt_heron", runs, &sqrt_heron);
  benchmark_sqrt_internal("sqrt_ieee", runs, &sqrt_ieee);
  benchmark_sqrt_internal("sqrtf", runs, &sqrtf);
}

static void benchmark_sqrt_internal(const char* name, const size_t runs, float (*sqrt_func)(float))
{
  struct timespec time_start;
  struct timespec time_end;
  const float num = 1337.420f;

  clock_gettime(CLOCK_MONOTONIC, &time_start);

  for (size_t i = 0; i < runs; ++i)
    sqrt_func(num);

  clock_gettime(CLOCK_MONOTONIC, &time_end);

  const double time_elapsed = (double) (time_end.tv_sec - time_start.tv_sec) +
                              1e-9 * (double) (time_end.tv_nsec - time_start.tv_nsec);
  const double time_avg = time_elapsed / (double) runs;

  printf("%s: total=%f, avg=%f\n", name, time_elapsed, time_avg);
}
