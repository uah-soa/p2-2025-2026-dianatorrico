/*
    Copyright 2023 The Operating Systems Group
    sort.h
 */

#ifndef SORT_H_
#define SORT_H_

#include <stdio.h>

// Type of the data handled by our algorithms:

typedef double thing;

// Type of function that will compare these data:

typedef int function_lesser_than(void *, thing, thing);

// Types of function that the sorting algorithms should use
// in order to access the data of the array:

typedef thing function_read(void *, unsigned pos);
typedef void function_write(void *, unsigned pos, thing value);

// Type of functions that will implement our algorithms:

typedef unsigned function_sort(void *, unsigned size,
                               function_lesser_than *plesserthan,
                               function_read *pread, function_write *pwrite);

// Declaration of the different sorting functions:

function_sort bubble_sort, insertion_sort, selection_sort, heap_sort, comb_sort,
    merge_sort, quick_sort, quick_sort_pa;

#endif  // SORT_H_
