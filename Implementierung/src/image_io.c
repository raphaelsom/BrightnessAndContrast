#include "image_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "input_parser.h"

int check_input_format(FILE* input_file);
int read_whitespaces(FILE* input_file);
int read_until_next_whitespace(FILE* input_file, char* str, size_t n);
int read_size_t(FILE* input_file, size_t* param);
int check_max_c_val(FILE* input_file);
int read_pixels_and_write_to_src(FILE* input_file, size_t width, size_t height, uint8_t** source_image);
int read_comment(FILE* input_file);

static const char* prog_name;

int write_to_res_img(const char* output_file_name, const uint8_t* res_image, const size_t width, const size_t height,
                     const char* program_name)
{
  prog_name = program_name;

  FILE* output_file = fopen(output_file_name, "w+");
  if (!output_file)
  {
    fprintf(stderr, "%s: Failed to open output file: %s\n", prog_name, output_file_name);
    return 1;
  }

  int ret = 0;

  if (fprintf(output_file, "P5\n%lu %lu\n255\n", width, height) < 0)
  {
    fprintf(stderr, "%s: Failed to write to output file: %s\n", prog_name, output_file_name);
    ret = -1;
    goto END;
  }

  const size_t length = fwrite(res_image, 1, width * height, output_file);
  if (length != width * height)
  {
    fprintf(stderr, "%s: Failed to write to output file \n", prog_name);
    ret = -1;
  }

END:
  fclose(output_file);
  return ret;
}

int read_source_image(const char* input_file_name, uint8_t** source_image, size_t* width, size_t* height,
                      const char* program_name)
{
  prog_name = program_name;

  FILE* input_file = fopen(input_file_name, "r");

  if (!input_file)
  {
    fprintf(stderr, "%s: Error opening input file: %s\n", prog_name, input_file_name);
    return -1;
  }

  /*
   * PPM-Format-Specification
   * wanted format: ppm
   * magic number -> P6
   * Whitespace (blanks, TABs, CRs, LFs)
   * width (ascii in dec)
   * Whitespace
   * height (ascii in dec)
   * Whitespace
   * max color val (ascii in dec, 0 < maxCval < 65536)
   * single whitespace
   * raster of [height] rows, each containing [width] pixels
   *  order: left to right
   *  each pixel:
   *    r->g->b
   *    1 byte, if maxCval < 256
   *    2 byte, else
   *    MSB first
  */

  //global vars needed
  int ret;

  //check if first lines are comments
  if ((ret = read_whitespaces(input_file)))
    goto END;

  //Check for right input format
  if ((ret = check_input_format(input_file)))
    goto END;

  //check for whitespaces
  if ((ret = read_whitespaces(input_file)))
    goto END;

  //check for and read correct width
  if ((ret = read_size_t(input_file, width)))
    goto END;

  //check for whitespaces
  if ((ret = read_whitespaces(input_file)))
    goto END;

  //check for and read correct height
  if ((ret = read_size_t(input_file, height)))
    goto END;

  //check for whitespaces
  if ((ret = read_whitespaces(input_file)))
    goto END;

  //check for max color value
  if ((ret = check_max_c_val(input_file)))
    goto END;

  // no more whitespaces allowed after max. color value; single whitespace is checked already

  if (*width == 0 || *height == 0)
  {
    fprintf(stderr, "%s: Zero width/height not allowed\n", prog_name);
    return 1;
  }

  //alloc source pointer
  if ((ret = alloc_image_pointer(source_image, *width, *height, 3)))
    goto END;

  //process pixels
  if ((ret = read_pixels_and_write_to_src(input_file, *width, *height, source_image)))
    goto END;

END:
  fclose(input_file);
  return ret;
}

int check_input_format(FILE* input_file)
{
  int ret = 0;

  // read 3 chars to ensure it is "P6" and not e.g. "P6123"
  char buf[4]; // 3 chars + \0
  if ((ret = read_until_next_whitespace(input_file, buf, sizeof(buf))) || strcmp(buf, "P6") != 0)
  {
    fprintf(stderr, "%s: Invalid magic number\n", prog_name);
    return ret;
  }

  return 0;
}

int read_whitespaces(FILE* input_file)
{
  char c = ' ';

  while (isspace(c))
  {
    const size_t length = fread(&c, 1, 1, input_file);
    if (length != 1)
    {
      fprintf(stderr, "%s: Unexpected EOF\n", prog_name);
      return 1;
    }

    if (c == '#')
    {
      if (read_comment(input_file))
        return 1;

      c = ' '; // set to whitespace to continue reading until non-whitespace is found
    }
  }

  fseek(input_file, -1, SEEK_CUR);
  return 0;
}

int read_until_next_whitespace(FILE* input_file, char* str, size_t n)
{
  if (n <= 1) // str needs to have at least 2 bytes to be able to read a char into it and add \0
  {
    fprintf(stderr, "%s: Invalid use of read_until_next_whitespace", prog_name);
    return -1;
  }

  size_t idx = 0;
  while (true)
  {
    char c;
    size_t length = fread(&c, 1, 1, input_file);
    if (length != 1)
    {
      fprintf(stderr, "%s: Unexpected EOF\n", prog_name);
      return 1;
    }

    if (isspace(c))
      break;

    if (idx == (n - 1)) // no more space in string - idx (n - 1) needed for '\0'
    {
      fprintf(stderr, "%s: Value exceeds expected size\n", prog_name);
      return 1;
    }

    if (c == '#')
    {
      if (read_comment(input_file))
        return 1;

      break;
    }

    str[idx++] = c;
  }

  str[idx] = '\0';
  return 0;
}

int read_size_t(FILE* input_file, size_t* param)
{
  int ret = 0;

  char str[21]; // max size_t = 20 digits + \0
  if ((ret = read_until_next_whitespace(input_file, str, sizeof(str))))
    return ret;

  return parse_size_t(str, param);
}

int check_max_c_val(FILE* input_file)
{
  int ret = 0;

  char str[4]; // "255" + \0
  if ((ret = read_until_next_whitespace(input_file, str, sizeof(str))))
    return ret;

  if (strcmp(str, "255") != 0)
  {
    fprintf(stderr, "%s: Invalid max. color value - expected 255\n", prog_name);
    return 1;
  }

  return 0;
}

int alloc_image_pointer(uint8_t** image, const size_t width, const size_t height, unsigned int bytes_per_pixel)
{
  size_t size = width * height * bytes_per_pixel;
  if (size / bytes_per_pixel / width / height != 1)
  {
    fprintf(stderr, "%s: Image too large\n", prog_name);
    return 1;
  }

  *image = malloc(size);
  if (!*image)
  {
    fprintf(stderr, "%s: Not enough memory\n", prog_name);
    return -1;
  }
  return 0;
}

int read_pixels_and_write_to_src(FILE* input_file, size_t width, size_t height, uint8_t** source_image)
{
  size_t length = fread((char*)(*source_image), 3, width * height, input_file);

  char eof = 0;
  size_t unused = fread(&eof, 1, 1, input_file);
  (void) unused;

  if (length != width * height || !feof(input_file))
  {
    fprintf(stderr, "%s: Pixel count didn't match width * height\n", prog_name);
    return 1;
  }

  return 0;
}

int read_comment(FILE* input_file)
{
  char c = 0;
  while (c != '\n')
  {
    if (fread(&c, 1, 1, input_file) != 1)
    {
      fprintf(stderr, "%s: unexpected EOF\n", prog_name);
      return 1;
    }
  }

  return 0;
}