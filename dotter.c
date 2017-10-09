#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG 1
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
typedef struct box_t {
   int x1, y1, x2, y2;
} box_t;

int square(int a) { return a * a; }
uint8_t* getpixel(uint8_t* image, int x, int y, int width) {
   return image + (((y * width) + x) * 3);
}
double subtract(uint8_t* a, uint8_t* b) {
   return sqrt(square(a[0] - b[0]) + square(a[1] - b[1]) + square(a[2] - b[2]));
}
double dist(uint8_t* a, uint8_t* b, box_t box, int width) {
   double res = 0;
   for (int x = box.x1; x < box.x2; x++) {
      for (int y = box.y1; y < box.y2; y++) {
         res += subtract(getpixel(a, x, y, width), getpixel(b, x, y, width));
      }
   }
   return res / ((box.x2 - box.x1) * (box.y2 - box.y1));
}
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
void draw(uint8_t* image, box_t box, uint8_t* color, int width, int radius) {
   int cx = box.x1 + ((box.x2 - box.x1)/2);
   int cy = box.y1 + ((box.y2 - box.y1)/2);
   for (int x = box.x1; x < box.x2; x++) {
      for (int y = box.y1; y < box.y2; y++) {
         if (sqrt(square(x - cx) + square(y - cy)) <= radius) {
            uint8_t* pixel = getpixel(image, x, y, width);
            memcpy(pixel, color, 3);
         }
      }
   }
}
uint8_t* random_color(uint8_t* image, int width, int height) {
   int x = rand() % width;
   int y = rand() % height;
   return getpixel(image, x, y, width);
}
uint8_t* copy(uint8_t* source, uint8_t* dest, int width, box_t box) {
   for (int y = box.y1; y < box.y2; y++) {
      memcpy(getpixel(dest, box.x1, y, width), getpixel(source, box.x1, y, width), 3 * (box.x2 - box.x1));
   }
}

int main(int argc, char** argv) {
   srand(time(NULL));
   char* source_filename = "Pictures/2hu.png";
   char* output_filename = "out.png";
   int radius = 5;
   int iterations = 100;
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
   time_t start = time(NULL);
   int width, height, n;
   uint8_t* source = stbi_load(source_filename, &width, &height, &n, 3);
   assert(source);
   uint8_t* img1 = malloc(width * height * 3);
   uint8_t* img2 = malloc(width * height * 3);
   time_t now;
   for (int i = 0; i < iterations; i++) {
      int x = rand() % width;
      int y = rand() % height;
      box_t box = (box_t) {.x1 = x - radius, .y1 = y - radius, .x2 = x + radius, .y2 = y + radius};
      correct(&box, width, height, radius);
      draw(img1, box, random_color(source, width, height), width, radius);
      double newdist = dist(source, img1, box, width);
      double olddist = dist(source, img2, box, width);
      if (newdist < olddist) {
         copy(img1, img2, width, box);
      } else {
         copy(img2, img1, width, box);
      }
      if (i % 10000 == 1 && time(NULL) != now) {
         now = time(NULL);
         time_t elapsed = now - start;
         double percent = ((double) i) / iterations;
         double total = elapsed / percent;
         double eta = total - elapsed;
         printf("%3.3f%% complete, eta = %3.3f seconds\n", 100 * percent, eta);
      }
   }
   stbi_image_free(source);
   stbi_write_png("out.png", width, height, 3, img2, width * 3);
   free(img1);
   free(img2);
}
