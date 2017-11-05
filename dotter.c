#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define _GNU_SOURCE
#include "stb/stb_image_write.h"
#include "stb/stb_image.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
// Used for passing the bottom left and top right corners of the box that the circle was drawn in
typedef struct box_t {
   int x1, y1, x2, y2;
} box_t;

int square(int a) { return a * a; }
// Gets a pointer to the given pixel in the given image
uint8_t* getpixel(uint8_t* image, int x, int y, int width) {
   return image + (((y * width) + x) * 3);
}
// Helper function for the distance function, gives the color distance between the two passed pixels
// that is, √(r² + g² + b²) where r, g and b are the differences in the red, green and blue components of the two pixels
double subtract(uint8_t* a, uint8_t* b) {
   return sqrt(square(a[0] - b[0]) + square(a[1] - b[1]) + square(a[2] - b[2]));
}
// Computes the average euclidian color distance between the two given images
// For optimization reasons, it only looks at the pixels within in the given box
double dist(uint8_t* a, uint8_t* b, box_t box, int width) {
   double res = 0;
   // Add up the differences in each pixel
   for (int x = box.x1; x < box.x2; x++) {
      for (int y = box.y1; y < box.y2; y++) {
         res += subtract(getpixel(a, x, y, width), getpixel(b, x, y, width));
      }
   }
   // divide by the area (width*height) of the box
   return res / ((box.x2 - box.x1) * (box.y2 - box.y1));
}
// Ensures that the box does not exceed the borders of the image, and if it does, it moves it inside
void correct(box_t* box, int width, int height, int radius) {
   if (box->x1 < 0) {
      box->x2 += -box->x1;
      box->x1 += -box->x1;
   }
   if (box->y1 < 0) {
      box->y2 += -box->y1;
      box->y1 += -box->y1;
   }
   if (box->y2 > height) {
      box->y2 -= radius * 2;
      box->y1 -= radius * 2;
   }
   if (box->x2 > width) {
      box->x2 -= radius * 2;
      box->x1 -= radius * 2;
   }
}
// Draws a circle of a given color inside the given box
// This could be made slightly more efficient by calculating the start and width
// of each scanline and doing each scanline in one copy operation, instead of checking every single pixel,
// but that would require more algebra than I want to do
void draw(uint8_t* image, box_t box, uint8_t* color, int width, int radius) {
   // Center coordinates of the circle
   int cx = box.x1 + ((box.x2 - box.x1)/2);
   int cy = box.y1 + ((box.y2 - box.y1)/2);
   // For each pixel
   for (int x = box.x1; x < box.x2; x++) {
      for (int y = box.y1; y < box.y2; y++) {
         // If it's inside the circle, copy the color to that pixel
         if (sqrt(square(x - cx) + square(y - cy)) <= radius) {
            uint8_t* pixel = getpixel(image, x, y, width);
            memcpy(pixel, color, 3);
         }
      }
   }
}
// Picks a random pixel from the input image and returns the color at that pixel
uint8_t* random_color(uint8_t* image, int width, int height) {
   int x = rand() % width;
   int y = rand() % height;
   return getpixel(image, x, y, width);
}
// Copies the pixels within a given box from the source image to the destination image
uint8_t* copy(uint8_t* source, uint8_t* dest, int width, box_t box) {
   for (int y = box.y1; y < box.y2; y++) {
      // memcpy takes its arguments in dest, source order.
      memcpy(getpixel(dest, box.x1, y, width), getpixel(source, box.x1, y, width), 3 * (box.x2 - box.x1));
   }
}
// entry point
int main(int argc, char** argv) {
   // Defaults
   bool make_slices = false;
   srand(time(NULL));
   char* source_filename = "input.png";
   char* output_filename = "output.png";
   int radius = 5;
   int iterations = 100;
   // Override defaults with command line arguments, if specified
   if (argc > 1) {
      source_filename = argv[1];
   }
   if (argc > 2) {
      output_filename = argv[2];
   }
   if (argc > 3) {
      iterations = atoi(argv[3]);
      assert(iterations != 0);
   }
   if (argc > 4) {
      radius = atoi(argv[4]);
      assert(radius != 0);
   }
   if (argc > 5) {
      make_slices = true;
   }
   // helper variables
   time_t start = time(NULL);
   time_t now = 0;
   int width, height, n;
   int kept = 0;
   int slice = 0;
   // Read in the source image
   uint8_t* source = stbi_load(source_filename, &width, &height, &n, 3);
   if (!source) {
      fprintf(stderr, "Unable to open/read image, are you sure it's a png?\n");
   }
   assert(source);
   // Create our two images
   uint8_t* img1 = malloc(width * height * 3);
   uint8_t* img2 = malloc(width * height * 3);
   for (int i = 0; i < iterations; i++) {
      // Pick a random location do draw the circle at
      int x = rand() % width;
      int y = rand() % height;
      box_t box = (box_t) {.x1 = x - radius, .y1 = y - radius, .x2 = x + radius, .y2 = y + radius};
      // Make sure the circle is inside the boundries of the image
      correct(&box, width, height, radius);
      // Draw the dot in img1
      draw(img1, box, random_color(source, width, height), width, radius);
      // Take the color distance between source and img1, and source and img2
      double newdist = dist(source, img1, box, width);
      double olddist = dist(source, img2, box, width);
      // If img1 is better, copy img1 to img2 (only copies the changed pixels)
      // otherwise, copy img2 back to img1 (only copies the changed pixels)
      if (newdist < olddist) {
         kept++;
         copy(img1, img2, width, box);
      } else {
         copy(img2, img1, width, box);
      }
      // Every 10000 iterations (max 1 per second), print a status update
      // Also every 10000 iterations, if we're making slices, output a slice
      if (i % 10000 == 1) {
         if (make_slices) {
            char* name;
            slice++;
            asprintf(&name, "out-%4d.png", slice);
            assert(name);
            // change the padding spaces to zeroes in the filename
            for (char* c = name; *c; c++) { if (*c == ' ') *c = '0'; }
            // save the slice
            stbi_write_png(name, width, height, 3, img2, width * 3);
         }
         if (time(NULL) != now) {
            // Estimate time remaining
            now = time(NULL);
            time_t elapsed = now - start;
            double percent = ((double) i) / iterations;
            double total = elapsed / percent;
            double eta = total - elapsed;
            printf("%3.3f%% complete, eta = %3.3f seconds\n", 100 * percent, eta);
         }
      }
   }
   // Free the source image
   stbi_image_free(source);
   // Write the output
   stbi_write_png(output_filename, width, height, 3, img2, width * 3);
   // Free img1 and img2
   free(img1);
   free(img2);
   // Print a final message and exit
   printf("Written to %s. Took %d seconds, kept %d cirlces and discarded %d circles\n", output_filename, time(NULL) - start, kept, iterations - kept);
}
