/*
    sort.c
*/

#include <stdlib.h>
#include "sort.h"

// Sorting by the bubble method
//     Stable:                yes
//     Max complexity:        O(N*N)
//     Average complexity:    O(N*N)
//     Min complexity:        O(N)
//     Other considerations:  It's the worst of all the usually
//                            taught methods. Some even propose not
//                            to toeach it ever again, and use the
//                            insertion sort method instead.
//                            The number of write operations can
//                            be reduced by chaining the swap
//                            operations, but it's not worth it
//                            (it will be worse than insertion
//                            anyway).

unsigned bubble_sort (void * p, unsigned size,
                      function_lesser_than * plesserthan,
                      function_read * pread,
                      function_write * pwrite)
{
    int end;
    unsigned u, iter;
    thing a, b;

    for (end=0, iter=0; size>1 && !end; size--)
    {
        a = pread (p, 0);

        for (end=1, u=0; u+1<size; u++, iter++)
        {
            b = pread (p, u+1);

            if (plesserthan(p,b,a))
            {
                pwrite (p, u, b);
                pwrite (p, u+1, a);
                end = 0;
            }
            else
                a = b;
        }
    }

    return iter;
}

// Sorting by the insertion method
//     Stable:                yes
//     Max complexity:        O(N*N)
//     Average complexity:    O(N*N)
//     Min complexity:        O(N)
//     Other considerations:  As simple as the bubble, in
//                            principle, but can be optimized
//                            to be faster. It will always be
//                            O(N*N) worst case, though.

unsigned insertion_sort (void * p, unsigned size,
                         function_lesser_than * plesserthan,
                         function_read * pread,
                         function_write * pwrite)
{
    unsigned u, v, iter;
    thing a, b, c;

    a = pread (p, 0);

    for (u=1, iter=0; u<size; u++)
    {
        b = pread (p, u);

        if (plesserthan(p,b,a))
        {
            v = u;
            c = a;

            do
            {
                pwrite (p, v, c);
                iter ++;

                if (--v==0)
                    break;

                c = pread (p, v-1);
            }
            while (plesserthan(p,b,c));

            pwrite (p, v, b);
        }
        else
        {
            a = b;
            iter ++;
        }
    }

    return iter;
}

// Sorting by the selection method
//     Stable:                no
//     Max complexity:        O(N*N)
//     Average complexity:    O(N*N)
//     Min complexity:        O(N*N)
//     Other considerations:  Never makes more than 2*N write
//                            operations. Always makes the same
//                            number of operations on the array
//                            (except when it saves write ops.).
//                            The read operations and one half of
//                            the write ops. follow a fixed memory
//                            access pattern. Not so with the
//                            other half of write ops., though.

unsigned selection_sort (void * p, unsigned size,
                         function_lesser_than * plesserthan,
                         function_read * pread,
                         function_write * pwrite)
{
    unsigned u, v, min, iter;
    thing a, b, c;

    for (u=iter=0; u<size-1; u++)
    {
        a = b = pread (p, min=u);

        for (v=u+1; v<size; v++, iter++)
        {
            c = pread (p, v);

            if (plesserthan(p,c,b))
            {
                min = v;
                b = c;
            }
        }

        if (min!=u)
        {
            pwrite (p, u, b);
            pwrite (p, min, a);
        }
    }

    return iter;
}

// Sorting by the heap method
//     Stable:                no
//     Max complexity:        O(N*log N)
//     Average complexity:    O(N*log N)
//     Min complexity:        O(N*log N)
//     Other considerations:  It's relatively slow compared to the
//                            _average_ behaviour of quicksort, and
//                            it is equally slow when the data are
//                            already sorted or nearly sorted. On
//                            the other hand, it's the only method
//                            that, with only O(1) additional
//                            space, has a worst case time
//                            complexity of O(N*log N).

static unsigned sift_in (void * p, unsigned size,
                         unsigned hole, thing a,
                         function_lesser_than * plesserthan,
                         function_read * pread,
                         function_write * pwrite);

unsigned heap_sort (void * p, unsigned size,
                    function_lesser_than * plesserthan,
                    function_read * pread,
                    function_write * pwrite)
{
    unsigned pos, iter;
    thing a;

    if (size<2)
        return 0;

    // First: heap up

    for (pos=size>>1, iter=0; pos; pos--)
        iter += sift_in (p, size, pos, pread(p,pos-1),
                         plesserthan, pread, pwrite);

    // Second: sort while extracting from the heap

    while (size>1)
    {
        a = pread (p, size-1);
        pwrite (p, size-1, pread(p,0));
        size --;
        iter += sift_in (p, size, 1, a,
                         plesserthan, pread, pwrite);
    }

    return iter;
}

static unsigned sift_in (void * p, unsigned size,
                         unsigned hole, thing nuevo,
                         function_lesser_than * plesserthan,
                         function_read * pread,
                         function_write * pwrite)
{
    unsigned u, h, iter;
    thing a, b;

    h = hole;
    iter = 0;

    for (u=h<<1; u<size; u<<=1, iter++)
    {
        a = pread (p, u-1);
        b = pread (p, u);

        if (plesserthan(p,a,b))
        {
            u ++;
            a = b;
        }

        pwrite (p, h-1, a);
        h = u;
    }

    if (u==size)
    {
        pwrite (p, h-1, pread(p, u-1));
        h = u;
    }

    while ((u=h>>1) >= hole)
    {
        a = pread (p, u-1);
        iter ++;

        if (!plesserthan(p,a,nuevo))
            break;

        pwrite (p, h-1, a);
        h = u;
    }

    pwrite (p, h-1, nuevo);

    return iter;
}

// Sorting by the "comb" method
//     Stable:                no
//     Max complexity:        O(N*N) (it might be O(N*log N),
//                                    but it's not proved)
//     Average complexity:    O(N*log N)
//     Min complexity:        O(N*log N)
//     Other considerations:  Fixed memory access pattern
//                            (if it is O(N*log N))

static const unsigned long combs[];

unsigned comb_sort (void * p, unsigned size,
                    function_lesser_than * plesserthan,
                    function_read * pread,
                    function_write * pwrite)
{
    unsigned u, v, n, comb, iter;
    thing a, b;

    iter = n = 0;

    while (size>combs[n])
        n ++;

    while (n>0)
    {
        comb = combs[--n];

        for (u=0, v=comb; v<size; u++, v++, iter++)
        {
            a = pread (p, u);
            b = pread (p, v);

            if (plesserthan(p,b,a))
            {
                pwrite (p, u, b);
                pwrite (p, v, a);
            }
        }

        if (n==0)
            break;

        comb = combs[--n];

        for (u=size-comb-1, v=size-1; v>=comb; u--, v--, iter++)
        {
            a = pread (p, u);
            b = pread (p, v);

            if (plesserthan(p,b,a))
            {
                pwrite (p, u, b);
                pwrite (p, v, a);
            }
        }
    }

    return iter + insertion_sort (p, size, plesserthan,
                                  pread, pwrite);
}

static const unsigned long combs[] =
{
    // This table uses coprime numbers. The ratio between any
    // two consecutive numbers is lesser than the square root
    // of 2.
    // Watch out: the smallest values have been ommited;
    //            the final insertion sort stage is always
    //            necessary ---whether it takes O(N*N) or
    //            just O(N) as usual, is another kettle of
    //            fish.
    5UL, 7UL,
    9UL, // Watch out: 9 is not prime
    11UL, 13UL, 17UL, 23UL, 31UL, 43UL, 59UL, 83UL, 113UL,
    157UL, 211UL, 293UL, 409UL, 577UL, 811UL, 1129UL,
    1583UL, 2237UL, 3163UL, 4463UL, 6311UL, 8923UL, 12619UL,
    17839UL, 25219UL, 35617UL, 50363UL, 71209UL, 100703UL,
    142403UL, 201359UL, 284759UL, 402697UL, 569497UL,
    805381UL, 1138979UL, 1610753UL, 2277941UL, 3221473UL,
    4555843UL, 6442897UL, 9111629UL, 12885751UL, 18223193UL,
    25771469UL, 36446357UL, 51542927UL, 72892669UL,
    103085789UL, 145785317UL, 206171569UL, 291570607UL,
    412343081UL, 583141177UL, 824686151UL, 1166282329UL,
    1649372281UL, 2332564607UL, 3298744483UL,
    ~0UL
};

// Sorting by the method of merging sorted lists
//     Stable:                yes
//     Max complexity:        O(N*log N)
//     Average complexity:    O(N*log N)
//     Min complexity:        O(N*log N)
//     Other considerations:  It needs O(N) additional memory.
//                            One version (natural mergesort) is
//                            O(N) min, but that version makes a
//                            less efficient use of the cache in
//                            the average case.

static unsigned merge_sort_r (void * p, unsigned size,
                              unsigned dest, unsigned temp,
                              function_lesser_than * plesserthan,
                              function_read * pread,
                              function_write * pwrite);

unsigned merge_sort (void * p, unsigned size,
                     function_lesser_than * plesserthan,
                     function_read * pread,
                     function_write * pwrite)
{
    unsigned u;

    for (u=0; u<size; u++)
        pwrite (p, u+size, pread(p,u));

    return size + merge_sort_r (p, size, 0, size, plesserthan, pread, pwrite);
}

static unsigned merge_sort_r (void * p, unsigned size,
                              unsigned dest, unsigned temp,
                              function_lesser_than * plesserthan,
                              function_read * pread,
                              function_write * pwrite)
{
    unsigned u, v, w, left, right, iter;
    thing a, b;

    left = size / 2;
    right = size - left;
    iter = 0;

    if (left>1)
        iter += merge_sort_r (p, left, temp, dest,
                              plesserthan, pread, pwrite);

    if (right>1)
        iter += merge_sort_r (p, right, temp+left, dest+left,
                              plesserthan, pread, pwrite);

    if (left)
        a = pread (p, temp);

    if (right)
        b = pread (p, temp+left);

    for (u=v=w=0; u<left && v<right; w++, iter++)
        if (plesserthan(p,b,a))
        {
            pwrite (p, dest+w, b);

            if (++v<right)
                b = pread (p, temp+left+v);
        }
        else
        {
            pwrite (p, dest+w, a);

            if (++u<left)
                a = pread (p, temp+u);
        }

    if (u<left)
        for (;;)
        {
            iter ++;
            pwrite (p, dest+w++, a);

            if (++u==left)
                return iter;

            a = pread (p, temp+u);
        }

    if (v<right)
        for (;;)
        {
            iter ++;
            pwrite (p, dest+w++, b);

            if (++v==right)
                return iter;

            b = pread (p, temp+left+v);
        }

    return iter;
}

// Sorting by the "quick" method
//     Stable:                no
//     Max complexity:        O(N*N)      <<-- (that's bad)
//     Average complexity:    O(N*log N)
//     Min complexity:        O(N*log N)
//     Other considerations:  It's the most popular. The problem of
//                            its O(N*N) worst case complexity is
//                            solved by switching to heapsort when
//                            things get ugly. It's also usual to
//                            switch to insertionsort when the
//                            number of elements is small (lesser
//                            than 8, for example)

static unsigned quick_sort_r (void * p,
                              unsigned from, unsigned size,
                              unsigned pa,
                              function_lesser_than * plesserthan,
                              function_read * pread,
                              function_write * pwrite);

unsigned quick_sort (void * p, unsigned size,
                     function_lesser_than * plesserthan,
                     function_read * pread,
                     function_write * pwrite)
{
    return quick_sort_r (p, 0, size, 0, plesserthan, pread, pwrite);
}

unsigned quick_sort_pa (void * p, unsigned size,
                        function_lesser_than * plesserthan,
                        function_read * pread,
                        function_write * pwrite)
{
    return quick_sort_r (p, 0, size, 1, plesserthan, pread, pwrite);
}

static unsigned my_random (unsigned from, unsigned size);

static unsigned quick_sort_r (void * p,
                              unsigned from, unsigned size,
                              unsigned pa,
                              function_lesser_than * plesserthan,
                              function_read * pread,
                              function_write * pwrite)
{
    unsigned left, right, hole, iter;
    thing a, pivot;

    for (iter=0; size>1; )
    {
        iter += size;

        if (pa)  // If requested, choose the pivot at random
        {
            hole = my_random (from, size);
            pivot = pread (p, hole);
            pwrite (p, hole, pread(p,from));
            iter ++;
        }
        else                           // Otherwise, choose the
            pivot = pread (p, from);   // first one as pivot

        hole = from;
        left = from + 1;
        right = from + size - 1;

        for (;;)
        {
            do
                a = pread (p, right);
            while (plesserthan(p,pivot,a) && right-->left);

            if (right<left)
                break;

            pwrite (p, hole, a);
            hole = right--;

            do
                a = pread (p, left);
            while (plesserthan(p,a,pivot) && left++<right);

            if (left>right)
                break;

            pwrite (p, hole, a);
            hole = left++;
        }

        pwrite (p, hole, pivot);

        left = hole - from;
        right = from + size - hole - 1;

        if (left>right)
        {
            if (right>1)
                iter += quick_sort_r (p, hole+1, right, pa,
                                      plesserthan, pread, pwrite);
            size = left;
        }
        else
        {
            if (left>1)
                iter += quick_sort_r (p, from, left, pa,
                                      plesserthan, pread, pwrite);
            size = right;
            from = hole + 1;
        }
    }

    return iter;
}

static unsigned my_random (unsigned from, unsigned size)
{
    unsigned n;

    n = from + (unsigned)(rand()/(RAND_MAX+1.0)*size);

    if (n>from+size-1)
        n = from+size-1;
    else if (n<from)
        n = from;

    return n;
}


