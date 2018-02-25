/*
 * Name: uarray2b.c
 *
 * By: Jared Lieberman and Jordan Stone
 *
 * Date: 10/3/16
 *
 * Purpose:  This source file holds the implementation of the UArray2b_T 
 *        ADT. The client's data is represetned by a two dimensional 
 *        array of blocks. Inside each element (block) of the array is 
 *        a one dimensional array of type UArray_T. 
 */
#include <stdio.h>
#include <stdlib.h>
#include "assert.h"
#include <math.h>
#include <uarray.h>
#include "uarray2.h"
#include "uarray2b.h"

#define T UArray2b_T

struct T {
        int width, height;
        int blocksize, size;
        UArray2_T blocks; 
                          
};

/*
 * Name: UArray2b_new
 * Purpose: This function will create a blocked two dimensional array of 
 * type UArray2_T.  The function returns the object to the client. 
 * The bytes in the array are initialized to zero. The blocksize is the 
 * length of the one dimensional array with a length of blocksize * 
 * blocksize.
 */
T UArray2b_new(int width, int height, int size, int blocksize)
{
        assert(width > 0 && height > 0);
        assert(size > 0);

        UArray2b_T array2b = malloc(sizeof(*array2b));
        assert(array2b != NULL);

        array2b->width     = width;
        array2b->height    = height;
        array2b->size      = size;
        array2b->blocksize = blocksize;
        
        /* width & height of uarray2 must be rounded up to a whole number */
        float fwidth       = array2b->width;
        float fblocksize   = array2b->blocksize;
        float fheight      = array2b->height;

        int uarray2_width  = ceil(fwidth/fblocksize);
        int uarray2_height = ceil(fheight/fblocksize);

        array2b->blocks  = UArray2_new(uarray2_width, uarray2_height, 
                (array2b->size) * (array2b->blocksize * array2b->blocksize));

        int uarray_len = (array2b->blocksize * array2b->blocksize);
        for (int row = 0; row < UArray2_height(array2b->blocks); row++) {
                for (int col = 0; col < UArray2_width(array2b-> blocks); 
                        col++) {

                        UArray_T uarray = UArray_new(uarray_len, 
                                array2b->size);
                        *(UArray_T*)UArray2_at(array2b->blocks, col, row) = 
                                uarray;
                }
        }

        return array2b;
}

/*
 * Name: UArray2b_new_64k_block
 * Purpose: his function will perform similarly to UArray2b_new.
 * This function will create a blocked two dimensional array of 
 * type UArray2_T.  Each block in the UArray_2T two 
 * dimensional array contains 64 bytes of memory. The width and height 
 * arguments determine the dimensions of this array. The size argument 
 * determines the number of bytes in each cell in a block. This function 
 * defaults the blocksize for the client. If a single cell does not fit 
 * in the 64KB block, the blocksize will be 1.
 */             
T UArray2b_new_64K_block(int width, int height, int size)
{
        assert(width > 0 && height > 0);
        assert(size > 0);

        UArray2b_T array2b = malloc(sizeof(*array2b));
        assert(array2b != NULL);

        array2b->width = width;
        array2b->height = height;
        array2b->size = size;
        
        int total_bytes = 64 * 1024;
        double dtotal_bytes = total_bytes/size;
        
        if (size >= total_bytes) {
                array2b->blocksize = 1; 
        } else {
                array2b->blocksize = sqrt(dtotal_bytes);
        }

        /* width & height of uarray2 must be rounded up to a whole number */
        float fwidth     = array2b->width;
        float fblocksize = array2b->blocksize;
        float fheight    = array2b->height;

        int uarray2_width  = ceil(fwidth/fblocksize);
        int uarray2_height = ceil(fheight/fblocksize);

        array2b->blocks  = UArray2_new(uarray2_width, uarray2_height, 
                (array2b->size) * (array2b->blocksize * array2b->blocksize));
      
        int uarray_len = (array2b->blocksize * array2b->blocksize);
        for (int row = 0; row < UArray2_height(array2b->blocks); row++) {
                for (int col = 0; col < UArray2_width(array2b->blocks); 
                        col++) {

                        UArray_T uarray = UArray_new(uarray_len, 
                                array2b->size);
                        *(UArray_T*)UArray2_at(array2b->blocks, col, row) = 
                                uarray;
                }
        }

        return array2b;
}       

/*
 * Name: UArray2b_free
 * Purpose: This function deallocates memory for both the two dimensional 
 * array and the one dimensional array and deletes the object. The client 
 * must pass in the pointer to the object.
 */
void  UArray2b_free(T *array2b)
{
        assert(array2b != NULL && *array2b != NULL);

        for (int row = 0; row < UArray2_height((*array2b)->blocks); row++) {
                for (int col = 0; col < UArray2_width((*array2b)-> blocks); 
                        col++) {

                        void *temp = UArray2_at((*array2b)->blocks, col, 
                                row);
                        UArray_T uarray = *(UArray_T*) temp;
                        UArray_free(&(uarray));
                }
        }
        UArray2_free(&((*array2b)->blocks));

        free(*array2b);
}

/*
 * Name: UArray2b_width
 * Purpose: This function takes the UArray2b_T array as an argument and 
 * returns the number of columns in the array. 
 */
int UArray2b_width(T array2b)
{
        assert(array2b != NULL);
        return (array2b->width);
}

/*
 * Name: UArray2b_width
 * Purpose: This function takes the UArray2b_T array as an argument and 
 * returns the number of rows in the array. 
 */
int UArray2b_height(T array2b)
{
        assert(array2b != NULL);
        return(array2b->height);
}

/*
 * Name: UArray2b_size
 * Purpose: This function returns the number of bytes required by a single 
 * cell in a block.
 */
int UArray2b_size(T array2b)
{
        assert(array2b != NULL);
        return(array2b->size);
}

/*
 * Name: UArray2b_size
 * Purpose: This function returns the size of one side of a block.
 */
int UArray2b_blocksize(T array2b)
{
        assert(array2b != NULL);
        return(array2b->blocksize);
}

/*
 * Name; UArray2b_at
 * Purpose: This function returns a pointer to a single cell inside the two 
 * dimensional array. The client can access the element by casting the 
 * pointer that the function returns and then dereferencing the pointer. 
 * This column and row arguments determine which cell to access.
 */                                                                     
void *UArray2b_at(T array2b, int column, int row)
{
        assert(array2b != NULL);

        if (column >= array2b->width || row >= array2b->height) {
                fprintf(stderr, "Out of bounds access not permitted\n" );
                exit(EXIT_FAILURE);
        }

        assert(row >= 0 && row < array2b->height);

        int block_col = column / array2b->blocksize;
        int block_row = row / array2b->blocksize;
        int uarray_index = array2b->blocksize * (row % array2b->blocksize) 
                + (column % array2b->blocksize);

        void *uarray = UArray2_at(array2b->blocks, block_col, block_row);
        
        return UArray_at( *(UArray_T*) uarray, uarray_index); 
}

/*
 * Name: UArray2b_map
 * Purpose: The client calls this function by passing in the two dimensional 
 * array, an apply function that the client implements, and the closure that 
 * the client provides. This function maps over all of the cells in one 
 * block before moving to another block. While visiting each element of the
 * array, it calls and apply function which is applied to the element. 
 */
void  UArray2b_map(T array2b, void apply(int col, int row, T array2b, 
        void *elem, void *cl), void *cl)
{
        assert(array2b != NULL);

        for (int r = 0; r < UArray2_height(array2b->blocks); r++) {
                for (int c = 0; c < UArray2_width(array2b->blocks); c++) {
                        void *temp = UArray2_at(array2b->blocks, c, r);
                        UArray_T uarray = *(UArray_T *) temp;

                        for (int i = 0; i < UArray_length(uarray); i++) {

                                int cell_col = (i % array2b->blocksize) 
                                        + (array2b->blocksize * c);
                                                
                                int cell_row = (i / array2b->blocksize) 
                                        + (array2b->blocksize * r);

                                if (cell_col < UArray2b_width(array2b) && 
                                        cell_row < 
                                        UArray2b_height(array2b)) {
                                        
                                        apply(cell_col, cell_row, array2b, 
                                                UArray_at(uarray, i), cl);
                                }
                        }
                }
        }
}
