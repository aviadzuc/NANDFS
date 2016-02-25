/** @file insertionsort.c
 * Insertion sort for pointer to pointer arrays. Used in scansir()
 */
/* Copyright (c) 2007 the authors listed at the following URL, and/or
 the authors of referenced articles or incorporated external code:
 http://en.literateprograms.org/Quicksort_(C)?action=history&offset=20070511214343
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 Retrieved from: http://en.literateprograms.org/Quicksort_(C)?oldid=10011 */
 
#include <system.h>
#include <src/fs/insertionsort.h>  
  
static int32_t compare_elements_helper(void **base, int32_t element_size, int32_t idx1, int32_t idx2,
                                   int32_t(*comparer)(void **, void**)){
    uint8_t **base_bytes = (uint8_t **)base;
    return comparer((void**)(&(base_bytes[idx1*element_size])), (void**)(&(base_bytes[idx2*element_size])));
}

#define element_less_than(i,j)  (compare_elements_helper(base, element_size, (i), (j), comparer) < 0)
 
static void exchange_elements_helper(void **base, int32_t idx1, int32_t idx2){
    void **temp_ptr = NULL;
    
    *temp_ptr  = base[idx1];
    base[idx1] = base[idx2];
    base[idx2] = *temp_ptr;
//    /* replace byte by byte*/
//    for (i=0; i<element_size; i++)
//    {
//        temp = base_bytes[idx1*element_size + i];
//        base_bytes[idx1*element_size + i] = base_bytes[idx2*element_size + i];
//        base_bytes[idx2*element_size + i] = temp;
//    }
}
 
#define exchange_elements(I,J)  (exchange_elements_helper(base, (I), (J)))

void insertion_sort(void **base, int32_t num_elements, int32_t element_size,
                    int32_t (*comparer)(void **, void **)){
   int32_t i;
   
   for (i=0; i < num_elements; i++)
   {
       int j;
       for (j = i - 1; j >= 0; j--)
       {
           if (element_less_than(j, j + 1)){
           	   break;
           }
           exchange_elements(j, j + 1);
       }
   }
}
