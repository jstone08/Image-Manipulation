/*
 * Name: ppmtrans.c
 *
 * By: Jared Lieberman and Jordan Stone
 *
 * Date: 10/5/16
 *
 * Purpose: This source file will read in ppm images and perform  
 * transformations on the inputted images. 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "assert.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "pnm.h"
#include "cputiming.h"

#define U unsigned

#define SET_METHODS(METHODS, MAP, WHAT) do {                    \
        methods = (METHODS);                                    \
        assert(methods != NULL);                                \
        map = methods->MAP;                                     \
        if (map == NULL) {                                      \
                fprintf(stderr, "%s does not support "          \
                                WHAT "mapping\n",               \
                                argv[0]);                       \
                exit(1);                                        \
        }                                                       \
} while (0)

int process_commandline(int argc, char **argv, char **time_file_name,
        A2Methods_T methods, A2Methods_mapfun *map, int *rotation, 
        int *transpose, int *flip); 
void process_transformation(Pnm_ppm image, A2Methods_mapfun *map, 
        char **time_file_name, int rotation, int flip, int transpose);

double perform_rotation(A2Methods_mapfun *map, Pnm_ppm image, Pnm_ppm 
        new_image, int rotation, int flip, int transpose, CPUTime_T timer);
Pnm_ppm initialize_new_image_dimensions( Pnm_ppm image, Pnm_ppm new_image, 
        U width, U height);

void transpose_image(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl);
void horizontal(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl);
void vertical(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl);

void rotate_0(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl);
void rotate_90(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl);
void rotate_180(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl);
void rotate_270(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl);


static void
usage(const char *progname)
{
        fprintf(stderr, "Usage: %s [-rotate <angle>] "
                        "[-{row,col,block}-major] [filename]\n",
                        progname);
        exit(1);
}

int main(int argc, char *argv[]) 
{
        char *time_file_name = NULL;
        int rotation = 0, transpose = 0, flip = 0;  
        /* 1 = horizontal, 2 = vertical, 0 = no flip */

        FILE *inputfp;

        A2Methods_T methods = uarray2_methods_plain; 
        assert(methods);

        A2Methods_mapfun *map = methods->map_default; 
        assert(map);
        
        int i = process_commandline(argc, argv, &time_file_name, methods, 
                map, &rotation, &transpose, &flip);

        if (i == argc) {
                inputfp = stdin;
        } else if (i == argc - 1) {
                inputfp = fopen(argv[i], "rb");
                if (inputfp == NULL) {
                        fprintf(stderr, "%s: %s %s %s\n", 
                                argv[0], "Could not open file",
                                argv[1], "for reading");
                        exit(EXIT_FAILURE);     
                }
        }

        Pnm_ppm image = Pnm_ppmread(inputfp, methods);
     
        fclose(inputfp);

        process_transformation(image, map, &time_file_name, rotation, flip, 
                transpose);

        return EXIT_SUCCESS;
}

/*
 * Name: process_commandline
 *
 * Purpose: This function will interpret command line arguments which will
 * determine the settings the program will use with the methods suite.
 */
int process_commandline(int argc, char **argv, char **time_file_name,
        A2Methods_T methods, A2Methods_mapfun *map, int *rotation, 
        int *transpose, int *flip)
{
        int i;
        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-row-major") == 0) {
                        SET_METHODS(uarray2_methods_plain, map_row_major, 
                                "row-major");
                } else if (strcmp(argv[i], "-col-major") == 0) {
                        SET_METHODS(uarray2_methods_plain, map_col_major, 
                                "column-major");
                } else if (strcmp(argv[i], "-block-major") == 0) {
                        SET_METHODS(uarray2_methods_blocked, map_block_major,
                                    "block-major");
                } else if (strcmp(argv[i], "-transpose") == 0) {
                        *transpose = 1;
                } else if (strcmp(argv[i], "-flip") == 0) {
                        if (!(i + 1 < argc)) {
                                usage(argv[0]);
                        }
                        if (strcmp(argv[++i], "horizontal") == 0) {
                                *flip = 1;
                        } else if (strcmp(argv[i], "vertical") == 0) {
                                *flip = 2;
                        }
                } else if (strcmp(argv[i], "-rotate") == 0) {
                        if (!(i + 1 < argc)) {    
                                usage(argv[0]);
                        }
                        char *endptr;
                        *rotation = strtol(argv[++i], &endptr, 10);
                        if (!(*rotation == 0 || *rotation == 90 ||
                            *rotation == 180 || *rotation == 270)) {
                                fprintf(stderr, "Rotation must be 0, ");
                                fprintf(stderr, "90 180 or 270\n");
                                usage(argv[0]);
                        }
                        if (!(*endptr == '\0')) {    
                                usage(argv[0]);
                        }
                }
                else if (strcmp(argv[i], "-time") == 0) {
                        *time_file_name = argv[++i];
                } else if (*argv[i] == '-') {
                        fprintf(stderr, "%s: unknown option '%s'\n", 
                                argv[0], argv[i]);
                } else if (argc - i > 1) {
                        fprintf(stderr, "Too many arguments\n");
                        usage(argv[0]);
                } else {
                        break;
                }
        }
        return i;
}

/*
 * Name: process_transformation
 *
 * Purpose: This function sets up the format of the new output image and 
 * calls helper functions to help execute smaller tasks involved with 
 * transforming the image.
 */
void process_transformation(Pnm_ppm image, A2Methods_mapfun *map, 
        char **time_file_name, int rotation, int flip, int transpose)
{
        CPUTime_T timer;
        double time;
        timer = CPUTime_New();

        Pnm_ppm new_image = malloc(sizeof(*new_image));
       
        time = perform_rotation(map, image, new_image, rotation, flip, 
                transpose, timer);

        double num_pixels = new_image->width * new_image->height;   
        double time_per_pix = time/num_pixels;

        if (*time_file_name != NULL) { 
                FILE *time_data = fopen(*time_file_name, "a"); 
                if (time_data == NULL) {
                        fprintf(stderr, "timing file could not be opened\n");
                        fclose(time_data);
                        exit(EXIT_FAILURE);
                }
                fprintf(time_data, "Time taken to run file: %f\n", time);
                fprintf(time_data, "Time taken per pixel: %f\n", 
                        time_per_pix);
                fclose(time_data);
        }
        Pnm_ppmwrite(stdout, new_image);
        Pnm_ppmfree(&image);
        Pnm_ppmfree(&new_image);

        CPUTime_Free(&timer);
}

/*
 * Name: perform_rotation
 *
 * Purpose: This function will handle the timing and call the map function
 * for the specified image transformation
 */
double perform_rotation(A2Methods_mapfun *map, Pnm_ppm image, Pnm_ppm 
        new_image, int rotation, int flip, int transpose, CPUTime_T timer) 
{
        if (transpose == 1 || rotation == 90 || rotation == 270) {
                new_image = initialize_new_image_dimensions(image, new_image,
                        image->height, image->width);
                CPUTime_Start(timer);
                if (transpose == 1) {
                        map(image->pixels, transpose_image, new_image);
                } else if (rotation == 90) {
                        map(image->pixels, rotate_90, new_image);
                } else {
                        map(image->pixels, rotate_270, new_image);
                }
        } else if (flip == 1 || flip == 2 || rotation == 0 || 
                rotation == 180) {
                
                new_image = initialize_new_image_dimensions(image, new_image,
                        image->width, image->height);
                CPUTime_Start(timer);
                if (flip == 1) {
                        map(image->pixels, horizontal, new_image);
                } else if (flip == 2) {
                        map(image->pixels, vertical, new_image);
                } else if (rotation == 0) {
                        map(image->pixels, rotate_0, new_image);
                } else {
                        map(image->pixels, rotate_180, new_image);
                }
        }

        return CPUTime_Stop(timer);
}

/*
 * Name: initialize_new_image_dimensions
 *
 * Purpose: This function will create a new Pnm_ppm which will help store
 * the new image data for the transformed image.
 */
Pnm_ppm initialize_new_image_dimensions(Pnm_ppm image, Pnm_ppm new_image, 
        U width, U height) 
{
        new_image->width       = width;
        new_image->height      = height;
        new_image->denominator = image->denominator; 

        new_image->pixels = image->methods->new(new_image->width, 
                new_image->height, sizeof(struct Pnm_rgb));
        new_image->methods = image->methods;

        return new_image;
}

/*
 * Name: transpose_image
 *
 * Purpose: This function is an apply function that is called when the 
 * user wants to transpose the image. The function will be applied to 
 * every pixel in the image.
 */
void transpose_image(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl)
{
        (void) array2;
        Pnm_ppm new_image = (Pnm_ppm) cl;
        Pnm_rgb source = (Pnm_rgb) elem;
        
        int new_col = row;
        int new_row = col;
        
        Pnm_rgb dest = (Pnm_rgb) new_image->methods->at(new_image->pixels, 
                new_col, new_row);

        *dest = *source;
}

/*
 * Name: horizontal
 *
 * Purpose: This function is an apply function that is called when the 
 * user wants to horizontally flip the image. The function will be applied
 * to every pixel in the image.
 */
void horizontal(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl)
{
        (void) array2;
        Pnm_ppm new_image = (Pnm_ppm) cl;
        Pnm_rgb source = (Pnm_rgb) elem;
        
        int new_col = new_image->width - (col + 1);
        int new_row = row;
        
        Pnm_rgb dest = (Pnm_rgb) new_image->methods->at(new_image->pixels, 
                new_col, new_row);

        *dest = *source;
}

/*
 * Name: vertical
 *
 * Purpose: This function is an apply function that is called when the 
 * user wants to vertically flip the image. The function will be applied to 
 * every pixel in the image.
 */
void vertical(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl)
{
        (void) array2;
        Pnm_ppm new_image = (Pnm_ppm) cl;
        Pnm_rgb source = (Pnm_rgb) elem;
        
        int new_col = col;        
        int new_row = new_image->height - (row + 1);
        
        Pnm_rgb dest = (Pnm_rgb) new_image->methods->at(new_image->pixels, 
                new_col, new_row);

        *dest = *source;
}

/*
 * Name: rotate_0
 *
 * Purpose: This function is an apply function that is called when the 
 * user wants to rotate the image 0 degrees. The function will be applied to 
 * every pixel in the image.
 */
void rotate_0(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl)
{
        (void) array2;
        Pnm_ppm new_image = (Pnm_ppm) cl;
        Pnm_rgb source = (Pnm_rgb) elem;
        
        Pnm_rgb dest = (Pnm_rgb) new_image->methods->at(new_image->pixels, 
                col, row);

        *dest = *source;
}

/*
 * Name: rotate_90
 *
 * Purpose: This function is an apply function that is called when the 
 * user wants to rotate the image 90 degrees. The function will be applied 
 * to every pixel in the image.
 */
void rotate_90(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl)
{ 
        (void) array2;
        Pnm_ppm new_image = (Pnm_ppm) cl;
        Pnm_rgb source = (Pnm_rgb) elem;

        int new_col = (int) (new_image->width) - row - 1;
        int new_row = col;
        
        Pnm_rgb dest = (Pnm_rgb) new_image->methods->at(new_image->pixels, 
                new_col, new_row);

        *dest = *source;
}

/*
 * Name: rotate_180
 *
 * Purpose: This function is an apply function that is called when the 
 * user wants to rotate the image 180 degrees. The function will be applied 
 * to every pixel in the image.
 */
void rotate_180(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl)
{ 
        (void) array2;
        Pnm_ppm new_image = (Pnm_ppm) cl;
        Pnm_rgb source = (Pnm_rgb) elem;

        U new_col = new_image->width - (U)col - 1;
        U new_row = new_image->height - (U)row - 1;
        
        Pnm_rgb dest = (Pnm_rgb) new_image->methods->at(new_image->pixels, 
                new_col, new_row);

        *dest = *source;
}

/*
 * Name: rotate_270
 *
 * Purpose: This function is an apply function that is called when the 
 * user wants to rotate the image 270 degrees. The function will be applied
 * to every pixel in the image.
 */
void rotate_270(int col, int row, A2Methods_UArray2 array2, 
        A2Methods_Object *elem, void *cl)
{
        (void) array2;
        Pnm_ppm new_image = (Pnm_ppm) cl;
        Pnm_rgb source = (Pnm_rgb) elem;
         
        int new_col = row;
        int new_row = (int) (new_image->height) - col - 1;
      
        Pnm_rgb dest = (Pnm_rgb) new_image->methods->at(new_image->pixels, 
                new_col, new_row);

        *dest = *source;
}
