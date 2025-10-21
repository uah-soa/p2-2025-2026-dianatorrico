/*
    gen_trace.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sort.h"

// Functions that prepare the data according to
// different criteria:

typedef void function_prepare_data (thing A[], unsigned size);

function_prepare_data ascending_order,
                      descending_order,
                      random_order;

// Functions that the sorting algorithms should use in order
// to access the data of the array:

thing read (void *, unsigned pos);
void write (void *, unsigned pos, thing value);

// Functions that the sorting algorithms should use in order
// to compare values of the array:

int lesser_than (void *, thing a, thing b);
int lesser_than_back_to_front (void *, thing a, thing b);

// The 'read' and 'write' functions receive, as their first
// parameter, a pointer to a structure of this type:

typedef struct
{
    thing * pdata;            // Array with data to be sorted
    unsigned nreads;          // Read operations counter
    unsigned nwrites;         // Write operations counter
    unsigned ncomparisons;    // Comparisons counter
    FILE * pf;                // Operations log
}
scontrol;

// Structure holding data of the parameters passed through
// the command line (algorithm to be used etc.)

typedef struct
{
    function_prepare_data * pprepare;
    function_sort * psort;
    int size;
}
sparameters;

// Function that parses the parameters received through the
// command line:

int parse_command (int, char *[], sparameters *);

// Main function

int main (int argc, char * argv[])
{
    thing * A;         // Dynamic array with data to sort
    scontrol C;        // Struct controlling access to array
    sparameters P;     // Parameters
    unsigned totalsz;  // Total # of elements (2*size in MER)
    unsigned u;

    if (parse_command(argc,argv,&P)<0)
        return -1;

    totalsz = P.psort==merge_sort ? P.size*2 : P.size;
    A = (thing*) malloc (totalsz*sizeof(thing));

    if (!A)
    {
        fprintf (stderr, "ERROR: not enough "
                         "dynamic memory.\n");
        return -2;
    }

    C.pdata = A;

    // Generate data in specified initial state
    P.pprepare (A, P.size);

    // Reset counters
    C.nreads = C.nwrites = C.ncomparisons = 0;
    C.pf = stdout;

    // Show total size
    printf (" T%u\n", totalsz);

    // Sort data with specified algorithm
    P.psort (&C,
             P.size,
             lesser_than,
             read,
             write);

    C.pf = NULL;

    for (u=0; u<P.size-1; u++)
        if (lesser_than(&C,A[u+1],A[u]))
            break;

    printf (" %s\n", u<P.size-1?"Out of order :-(":"Sorted ;-)");
    free (A);
    return 0;
}

// Functions that the sorting algorithms should use in order
// to access the data of the array:

thing read (void * p, unsigned pos)
{
    scontrol * pc = (scontrol*) p;

    pc->nreads ++;

    if (pc->pf)
    {
        fprintf (pc->pf, " R%u", pos);

        if (((pc->nreads+
              pc->nwrites+
              pc->ncomparisons) & 7) == 0)
            fputc ('\n', pc->pf);
    }

    return pc->pdata[pos];
}

void write (void * p, unsigned pos, thing value)
{
    scontrol * pc = (scontrol*) p;

    pc->nwrites ++;

    if (pc->pf)
    {
        fprintf (pc->pf, " W%u", pos);

        if (((pc->nreads+
              pc->nwrites+
              pc->ncomparisons) & 7) == 0)
            fputc ('\n', pc->pf);
    }

    pc->pdata[pos] = value;
}

// Functions that the sorting algorithms should use in order
// to compare values of the array:

int lesser_than (void * p, thing a, thing b)
{
    scontrol * pc = (scontrol*) p;

    pc->ncomparisons ++;

    if (pc->pf)
    {
        fprintf (pc->pf, " C");

        if (((pc->nreads+
              pc->nwrites+
              pc->ncomparisons) & 7) == 0)
            fputc ('\n', pc->pf);
    }

    return a < b;
}

int lesser_than_back_to_front (void * p, thing a, thing b)
{
    scontrol * pc = (scontrol*) p;

    pc->ncomparisons ++;

    if (pc->pf)
    {
        fprintf (pc->pf, " C");

        if (((pc->nreads+
              pc->nwrites+
              pc->ncomparisons) & 7) == 0)
            fputc ('\n', pc->pf);
    }

    return a > b;
}

// Functions that prepare the data according to
// different criteria:

void ascending_order (thing A[], unsigned size)
{
    unsigned u;

    for (u=0; u<size; u++)
        A[u] = u;
}

void descending_order (thing A[], unsigned size)
{
    unsigned u;

    for (u=0; u<size; u++)
        A[u] = size-u-1;
}

void random_order (thing A[], unsigned size)
{
    unsigned u, n;
    thing tmp;

    srand (0);

    for (u=0; u<5; u++)
        rand ();

    ascending_order (A, size);

    for (u=0; u<size-1; u++)
    {
        n = 1 + u + (unsigned)(rand() * (size-u-1.0) / RAND_MAX);

        if (n>size-1)
            n = size-1;

        if (n!=u)
        {
            tmp = A[n];
            A[n] = A[u];
            A[u] = tmp;
        }
    }
}

// Function that parses the parameters received through the
// command line:

int parse_command (int argc, char * argv[],
                   sparameters * pPar)
{
    unsigned u;

    struct
    {
        function_prepare_data * pfun;
        const char * name;
    }
    G[] = { { ascending_order, "ASC" },
            { descending_order, "DES" },
            { random_order, "RAN" },
            { NULL, NULL } };

    struct
    {
        function_sort * pfun;
        const char * name;
    }
    S[] = { { bubble_sort, "BUB" },
            { insertion_sort, "INS" },
            { selection_sort, "SEL" },
            { heap_sort, "HEA" },
            { comb_sort, "COM" },
            { merge_sort, "MER" },
            { quick_sort, "QUI" },
            { quick_sort_pa, "QRP" },
            { NULL, NULL } }	;

    // Default parameters:
    pPar->pprepare = random_order;
    pPar->psort = merge_sort;
    pPar->size = 4;

    if (argc>1)
    {
        for (u=0; S[u].pfun; u++)
            if (!strcmp(argv[1],S[u].name))
                break;

        if (S[u].pfun)
            pPar->psort = S[u].pfun;
        else
        {
            fprintf (stderr, "ERROR: Unknown sorting "
                             "algorithm \"%s\"\n", argv[1]);
            return -1;
        }
    }

    if (argc>2)
    {
        for (u=0; G[u].pfun; u++)
            if (!strcmp(argv[2],G[u].name))
                break;

        if (G[u].pfun)
            pPar->pprepare = G[u].pfun;
        else
        {
            fprintf (stderr, "ERROR: Unknown initial "
                             "state \"%s\"\n", argv[2]);
            return -1;
        }
    }

    if (argc>3)
    {
        u = sscanf (argv[3], "%d", &pPar->size);

        if (u!=1 || pPar->size<2 || pPar->size>10000)
        {
            fprintf (stderr, "ERROR: Wrong size (must be "
                             "a number ranging from 2 "
                             "to 10000\n");
            return -1;
        }
    }

    return 0;
}


