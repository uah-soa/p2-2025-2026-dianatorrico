/*
    sim_pag_main.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim_paging.h"

// Structure holding data of the parameters passed through
// the command line (algorithm to be used etc.)

typedef struct
{
    int pagsz, numframes;
    const char * algorithm, * initialstate;
    int numelem;
    char detailed;
}
sparameters;

// Function that parses the parameters received through the
// command line:

int parse_command (int, char*[], sparameters*);

// Main function

int main (int argc, char * argv[])
{
    sparameters P;      // Parameters received in the command line
    char command[100];  // Command for executing gen_trace
    FILE * pipe;        // Communication channel with gen_trace
    int ok;             // Flag
    char op;            // Elementary operation ('R'ead, 'W'ri..)
    unsigned u;         // Number of the read/written element
    unsigned numpags;   // Total number of pages
    unsigned totelem;   // Total num. of elements (double in MER)
    ssystem S;          // State of the whole simulated system

    memset (&S, 0, sizeof(S));  // Reset system

    if (parse_command(argc,argv,&P)<0)  // Put parameters in P
        return -1;

    printf ("# Parameters:  %s %i %i %s %s %i %c\n",
            argv[0], P.pagsz, P.numframes,
            P.algorithm, P.initialstate, P.numelem,
            P.detailed?'D':'N');

    // Prepare command for invoking gen_trace
    // (sprintf "prints" in a string)
    sprintf (command, "./gen_trace %s %s %u",
                      P.algorithm, P.initialstate, P.numelem);

    printf ("# Executing command:  %s\n", command);

    // Invoke gen_trace and open a pipe to read
    // its standard output ("r" stands for read)
    pipe = popen (command, "r");

    if (!pipe)
    {
        perror ("ERROR while starting gen_trace");
        return -1;
    }

    // Read total # of elements to be sorted
    ok = fscanf (pipe," T %u", &totelem) == 1;

    if (ok)
    {
        // Calculate total number of pages
        numpags = (totelem+P.pagsz-1) / P.pagsz; 

        S.pgt = (spage*) malloc (numpags*sizeof(spage));
        S.frt = (sframe*) malloc (P.numframes*sizeof(sframe));

        if (!S.pgt || !S.frt)
        {
            fprintf (stderr,
                     "ERROR: not enough "
                            "dynamic memory\n");
            ok = 0;
        }
    }

    if (ok)
    {
        S.pagsz = P.pagsz;
        S.numpags = numpags;
        S.numframes = P.numframes;
        S.detailed = P.detailed;

        init_tables (&S);
    }

    while (ok)
    {
        // Ignore spaces and read one character
        if (fscanf(pipe," %c",&op)!=1)
        {
            ok = 0;
            break;
        }

        if (op=='R' || op=='W')              // If R/W,
        {                                    // take element
            if (fscanf(pipe,"%u",&u)!=1)     // number and
                ok = 0;                      // annotate
            else
                sim_mmu (&S, u, op);  // Simulate memory access
        }
        else if (op=='S')        // 'S'orted -> end
            break;               // 'C'omparison -> go on
        else if (op!='C')        // 'O'ut of order (or
            ok = 0;              // something else) -> error
    }

    if (ok)
        print_report (&S);

    // Wait until gen_trace ends and close
    if (pclose(pipe)==-1)
        ok = 0;

    // Free dynamic memory
    free (S.pgt);
    free (S.frt);

    return ok ? 0 : -1;
}

// Function that shows the results

void print_report (ssystem * S)
{
    printf ("\n---------- GENERAL REPORT ----------\n\n");

    printf ("Read references:          %d\n", S->numrefsread);
    printf ("Write references:         %d\n", S->numrefswrite);
    printf ("Page faults:              %d\n", S->numpagefaults);
    printf ("Page dumps to disc:       %d\n", S->numpgwriteback);

    if (S->numillegalrefs)
        printf ("\nWARNING: %d REFERENCES OUT OF RANGE\n",
                S->numillegalrefs);
                         
    printf ("\n---------- PAGES TABLE ---------\n\n");

    print_page_table (S);

    printf ("\n---------- FRAMES TABLE ----------\n\n");

    print_frames_table (S);

    printf ("\n--------- REPLACEMENT REPORT ---------\n\n");

    print_replacement_report (S);

    printf ("\n-------------------------------------\n\n");
    printf ("PAGE FAULTS: --->> %d <<---\n\n",
            S->numpagefaults);
}

// Function that parses the parameters received through the
// command line:

#define VALID_ALGORITHMS "BUB/INS/SEL/HEA/COM/MER/QUI/QRP"
#define VALID_INIT_ORD "ASC/DES/RAN"

int parse_command (int argc, char * argv[], sparameters * p)
{
    int ok;

    // Default parameters
    p->pagsz = 16;
    p->numframes = 32;
    p->algorithm = "MER";
    p->initialstate = "RAN";
    p->numelem = 1000;
    p->detailed = 0;

    if (argc>7)
    {
        fprintf (stderr,
                 "\n    ERROR: too many parameters");
        ok = 0;
    }
    else
    {
        ok = 1;

        if (argc>1 && (sscanf(argv[1],"%d",&p->pagsz)!=1 ||
                       p->pagsz<1))
        {
            fprintf (stderr,
                     "\n    ERROR: wrong page size");
            ok = 0;
        }

        if (argc>2 && (sscanf(argv[2],"%d",&p->numframes)!=1 ||
                       p->numframes<1))
        {
            fprintf (stderr,
                     "\n    ERROR: wrong number of frames");
            ok = 0;
        }

        if (argc>3)
            p->algorithm = argv[3];

        if (strlen(p->algorithm)!=3 ||
            strchr(p->algorithm,'/') ||
            !strstr(VALID_ALGORITHMS,p->algorithm))
        {
            fprintf (stderr,
                     "\n    ERROR: wrong algorithm");
            ok = 0;
        }

        if (argc>4)
            p->initialstate = argv[4];

        if (strlen(p->initialstate)!=3 ||
            strchr(p->initialstate,'/') ||
            !strstr(VALID_INIT_ORD,p->initialstate))
        {
            fprintf (stderr,
                     "\n    ERROR: wrong initial state");
            ok = 0;
        }

        if (argc>5 && (sscanf(argv[5],"%d",&p->numelem)!=1 ||
                       p->numelem<2))
        {
            fprintf (stderr,
                     "\n    ERROR: wrong number of "
                                  "elements");
            ok = 0;
        }

        if (argc>6)
        {
            if (strcmp(argv[6],"N") && strcmp(argv[6],"D"))
            {
                fprintf (stderr,
                         "\n    ERROR: wrong mode");
                ok = 0;
            }

            p->detailed = !strcmp(argv[6],"D");
        }
    }

    if (ok)
        return 0;

    fprintf (stderr,
             "\n\n    USAGE:\n\t%s pagesize numframes alg "
                          "initord numelem mode\n\n", argv[0]);

    fprintf (stderr,
             "\tpagesize: # of elements that fit in a page\n"
             "\tnumframes: # of page frames (physical mem.)\n"
             "\talg: sorting algorithm (%s)\n"
             "\tinitord: initial state of the array (%s)\n"
             "\tnumelem: # of elements to be sorted\n"
             "\tmode: normal(N) or detailed(D)\n"
             "\n",
             VALID_ALGORITHMS, VALID_INIT_ORD);

    fprintf (stderr,
             "    EXAMPLES:\n"
             "\t%s 16 32 MER RAN 1000\n"
             "\t%s 1 3 HEA DES 4 D\n"
             "\n",
             argv[0], argv[0]);

    return -1;
}

