/*
    count_ops.c
*/

#include <stdio.h>
#include <stdlib.h>

#define NUM_ALG 8
#define NUM_INI 3
#define NUM_SZS 3

int main ()
{
    // Initial states of the array: ASCending order,
    // DEScending order and RANdom order (or rather disorder)
    const char * initial[NUM_INI] = { "ASC", "DES", "RAN" };

    // Array sizes with wich to experiment
    unsigned sizes[NUM_SZS] = { 10, 100, 1000 };

    // Sorting algorithms: bubble, insertion, selection,
    // heapsort, combsort, mergesort, quicksort, and
    // quicksort with random pivot
    const char * algorithms[NUM_ALG] = { "BUB", "INS", "SEL",
                                         "HEA", "COM", "MER",
                                         "QUI", "QRP" };

    char command[100]; // Command for executing gen_trace
    FILE * pipe;       // Channel for communicating with gen_trace
    int a, i, t, ok;   // Array indexes and flag
    char op;           // Elementary operation ('R'ead, 'W'rite...)
    unsigned u;        // Number of read/written element
    unsigned sz;       // Size of the array to sort

    unsigned reads, writes, comparisons;            // Counters
    unsigned results[NUM_ALG][NUM_INI][NUM_SZS];    // Tables

    // Carry out experiments and fill results tables

    for (t=0; t<NUM_SZS; t++)
        for (a=0; a<NUM_ALG; a++)
            for (i=0; i<NUM_INI; i++)
            {
                sz = sizes[t];
                reads = writes = comparisons = 0;

                // Make command to invoke gen_trace
                // (sprintf "prints" in a string)
                sprintf (command, "./gen_trace %s %s %u",
                                  algorithms[a], initial[i], sz);

                printf ("Executing command: %s\n", command);

                // Invoke gen_trace and open pipe for reading
                // its standard output ("r" stands for read)
                pipe = popen (command, "r");

                if (!pipe)
                {
                    perror ("ERROR starting gen_trace");
                    return -1;
                }

                // Read (and ignore) size
                ok = fscanf (pipe," T %u", &u) == 1;

                while (ok)
                {
                    // Ignore spaces and read one character
                    if (fscanf(pipe," %c",&op)!=1)
                    {
                        ok = 0;
                        break;
                    }

                    if (op=='R' || op=='W')  // If it's a read
                    {                        // or a write,
                        if (op=='R')         // count it and read
                            reads ++;        // the number of the
                        else                 // referenced
                            writes ++;       // element (u)

                        if (fscanf(pipe,"%u",&u)!=1)
                            ok = 0;
                    }
                    else if (op=='C')        // 'C'omparison
                        comparisons ++;
                    else if (op=='S')        // 'S'orted (end)
                        break;
                    else                     // 'O'ut of order (or
                        ok = 0;              // sth. else) -> error
                }

                // Wait until gen_trace ends and close
                if (pclose(pipe)==-1)
                    ok = 0;

                // Store number of operations in the table
                // (0 if an error occurred)
                results[a][i][t] = ok ? reads +
                                        writes +
                                        comparisons : 0;
            }

    // Print tables

    for (i=0; i<NUM_INI; i++)
    {
        printf ("\n\nInitial state: %s\n", initial[i]);
        printf ("===================\nSize");

        for (a=0; a<NUM_ALG; a++)
            printf ("%8s", algorithms[a]);

        printf ("\n\n");

        for (t=0; t<NUM_SZS; t++)
        {
            printf ("%6u", sizes[t]);

            for (a=0; a<NUM_ALG; a++)
                if (results[a][i][t]<1000000)
                    printf (" %7u", results[a][i][t]);
                else
                    printf (" %7.1e", (float)results[a][i][t]);

            printf ("\n");
        }
    }

    printf ("\n");

    return 0;
}

