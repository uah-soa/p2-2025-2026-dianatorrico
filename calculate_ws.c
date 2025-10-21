/*
    calculate_ws.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure holding data of the parameters passed through
// the command line (algorithm to be used etc.)

typedef struct
{
    int pagesz, interval;
    const char * algorithm, * initialorder;
    int numelem;
}
sparameters;

// Function that parses the parameters received through the
// command line:

int parse_command (int, char*[], sparameters*);

// Structure that maintains the set of referenced pages:

#define NUM_BYTES(BITS) ((BITS+7)>>3)
#define SET_BIT(P,NBIT) ((P)[(NBIT)>>3] |= 1<<((NBIT)&7))
#define GET_BIT(P,NBIT) ((P)[(NBIT)>>3] & (1<<((NBIT)&7)))

typedef struct
{
    char * prefbits;      // Reference bits of the pages
    int numbytes;         // Size in bytes
    unsigned numpages;    // # of pages (and ref. bits)
    unsigned numrefs;     // # of references in current interval
    unsigned totalrefs;   // Total # of references
    unsigned numillegal;  // # of illegal references
}
spgstate;

// Functions that manipulate the referenced pages set

int reserve_bits (spgstate *, int numpages);
void free_bits (spgstate *);

void annotate_reference (const sparameters *,
                         spgstate *,
                         unsigned element);

void dump_num_refs (spgstate *);
void print_header (void);

// Main function

int main (int argc, char * argv[])
{
    sparameters P;      // Parameters received in the command line
    char command[100];  // Command for executing gen_trace
    FILE * pipe;        // Communication channel with gen_traza
    int ok;             // Flag
    char op;            // Elementary operation ('R'ead, 'W'ri..)
    unsigned u;         // Number of the read/written element
    spgstate S;         // State of the pages (referenced/not)
    unsigned numpags;   // Total number of pages
    unsigned totelem;   // Total num. of elements (double in MER)

    S.prefbits = NULL;

    if (parse_command(argc,argv,&P)<0)  // Put parameters in P
        return -1;

    printf ("# Parameters:  %s %i %i %s %s %i\n",
            argv[0], P.pagesz, P.interval,
            P.algorithm, P.initialorder, P.numelem);

    // Prepare command for invoking gen_trace
    // (sprintf "prints" in a string)
    sprintf (command, "./gen_trace %s %s %u",
                      P.algorithm, P.initialorder, P.numelem);

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
        numpags = (totelem+P.pagesz-1) / P.pagesz; 

        // Reserve space for the reference bits
        if (reserve_bits(&S,numpags)<0)
        {
            fprintf (stderr,
                     "ERROR: not enough "
                            "dynamic memory\n");
            ok = 0;
        }
    }

    if (ok)
        print_header ();

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
                annotate_reference (&P, &S, u);
        }
        else if (op=='S')        // 'S'orted -> end
            break;               // 'C'omparison -> go on
        else if (op!='C')        // 'O'ut of order (or
            ok = 0;              // something else) -> error
    }

    if (ok)
    {
        dump_num_refs (&S);

        if (S.numillegal)
            printf ("WARNING: There were %u references to "
                             "nonexistent pages\n", S.numillegal);
    }

    // Wait until gen_trace ends and close
    if (pclose(pipe)==-1)
        ok = 0;

    free_bits (&S);

    return ok ? 0 : -1;
}

// Functions that manipulate the referenced pages set

int reserve_bits (spgstate * pS, int numpages)
{
    pS->numpages = numpages;
    pS->numbytes = NUM_BYTES (numpages);
    pS->numrefs = pS->totalrefs = pS->numillegal = 0;
    pS->prefbits = (char*) malloc (pS->numbytes);

    if (pS->prefbits)
    {
        memset (pS->prefbits, 0, pS->numbytes);
        return 0;
    }

    return -1;
}

void free_bits (spgstate * pS)
{
    free (pS->prefbits);
    pS->prefbits = NULL;
}

void annotate_reference (const sparameters * pPar,
                         spgstate * pS,
                         unsigned element)
{
    unsigned page;

    page = element / pPar->pagesz;

    if (page < pS->numpages)
    {
        SET_BIT (pS->prefbits, page);

        if (++pS->numrefs >= pPar->interval)
            dump_num_refs (pS);
    }
    else
        pS->numillegal ++;
}

void print_header (void)
{
    printf ("#\n#%18s %15s %15s %15s\n#\n",
            "Position", "Interval", "Pages", "Pages/op.");
}

void dump_num_refs (spgstate * pS)
{
    unsigned u, refs;

    if (!pS->numrefs)
        return;

    for (u=refs=0; u<pS->numpages; u++)
        if (GET_BIT(pS->prefbits, u))
            refs ++;

    printf (" %15u %15u %15u %15f\n",
            pS->totalrefs, pS->numrefs,
            refs, refs/(float)pS->numrefs);

    memset (pS->prefbits, 0, pS->numbytes);
    pS->totalrefs += pS->numrefs;
    pS->numrefs = 0;
}

// Function that parses the parameters received through the
// command line:

#define VALID_ALGORITHMS "BUB/INS/SEL/HEA/COM/MER/QUI/QRP"
#define VALID_INITIAL_ORD "ASC/DES/RAN"

int parse_command (int argc, char * argv[], sparameters * p)
{
    int ok;

    // Default parameters
    p->pagesz = 16;
    p->interval = 2000;
    p->algorithm = "MER";
    p->initialorder = "RAN";
    p->numelem = 1000;

    if (argc>6)
        ok = 0;
    else
    {
        ok = 1;

        if (argc>1 && (sscanf(argv[1],"%d",&p->pagesz)!=1 ||
                       p->pagesz<1))
        {
            fprintf (stderr,
                     "\n    ERROR: wrong page size\n");
            ok = 0;
        }

        if (argc>2 && (sscanf(argv[2],"%d",&p->interval)!=1 ||
                       p->interval<2))
        {
            fprintf (stderr,
                     "\n    ERROR: wrong interval\n");
            ok = 0;
        }

        if (argc>3)
            p->algorithm = argv[3];

        if (strlen(p->algorithm)!=3 ||
            strchr(p->algorithm,'/') ||
            !strstr(VALID_ALGORITHMS,p->algorithm))
        {
            fprintf (stderr,
                     "\n    ERROR: wrong algorithm\n");
            ok = 0;
        }

        if (argc>4)
            p->initialorder = argv[4];

        if (strlen(p->initialorder)!=3 ||
            strchr(p->initialorder,'/') ||
            !strstr(VALID_INITIAL_ORD,p->initialorder))
        {
            fprintf (stderr,
                     "\n    ERROR: wrong initial order\n");
            ok = 0;
        }

        if (argc>5 && (sscanf(argv[5],"%d",&p->numelem)!=1 ||
                       p->numelem<2))
        {
            fprintf (stderr,
                     "\n    ERROR: wrong number of "
                                  "elements\n");
            ok = 0;
        }
    }

    if (ok)
        return 0;

    fprintf (stderr,
             "\n    USAGE:\n\t%s pagesz interval algorithm "
                        "initialorder numelem\n\n", argv[0]);

    fprintf (stderr,
             "\tpagesz: nº de elementos que caben "
                       "en una página\n"
             "\tinterval: # of operations per interval\n"
             "\talgorithm: sorting algorithm (%s)\n"
             "\tinitialorder: initial order of the array (%s)\n"
             "\tnumelem: # of elements to be sorted\n"
             "\n",
             VALID_ALGORITHMS, VALID_INITIAL_ORD);

    fprintf (stderr,
             "    EXAMPLE:\n"
             "\t%s 16 2000 MER RAN 1000\n"
             "\n",
             argv[0]);

    return -1;
}

