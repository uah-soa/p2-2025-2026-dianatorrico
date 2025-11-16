#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./sim_paging.h"

//creamos ahora la simulacion como si estuviese haciendo FIFO -----------------------------------------------------------


// Usamos next de frt[] para encadenar. Guardamos cabeza/cola aquí.
static int fifo_head = -1;  // frame al frente (más antiguo)
static int fifo_tail = -1;  // frame al final  (más reciente)

// Encola un frame al final
static void fifo_enqueue(ssystem *S, int frame) {
  S->frt[frame].next = -1;        // por higiene, -1 = fin
  if (fifo_tail == -1) {          // cola vacía
    fifo_head = fifo_tail = frame;
  } else {                        // hay cola, enlazar al final
    S->frt[fifo_tail].next = frame;
    fifo_tail = frame;
  }
}

// Desencola el frame del frente; -1 si vacío
static int fifo_dequeue(ssystem *S) {
  if (fifo_head == -1) return -1; // vacío
  int f = fifo_head;
  fifo_head = S->frt[fifo_head].next;
  if (fifo_head == -1) fifo_tail = -1; // quedó vacía
  S->frt[f].next = -1;                 // higiene
  return f;
}


// Inicializar
void init_tables(ssystem* S) {
  int i;

  // Tabla de páginas a 0
  memset(S->pgt, 0, sizeof(spage) * S->numpags);

  // No usamos LRU aquí
  S->lru = -1;
  S->clock = 0;

  // Lista circular de libres
  for (i = 0; i < S->numframes - 1; i++) {
    S->frt[i].page = -1;
    S->frt[i].next = i + 1;
  }
  S->frt[i].page = -1;    // i == numframes-1
  S->frt[i].next = 0;     // cierre circular
  S->listfree    = i;     // apunta al último

  // Cola FIFO de ocupados vacía
  S->listoccupied = -1;   // no la usamos directamente
  fifo_head = fifo_tail = -1;
}

// MMU: traducción V->F
unsigned sim_mmu(ssystem* S, unsigned virtual_addr, char op) {
  unsigned physical_addr;
  int page, frame, offset;

  page   = virtual_addr / S->pagsz;     // num de pagina 
  offset = virtual_addr % S->pagsz;     // desplazamiento dentro de la pagina 

  //comprobamos que esta dentro del rango de la memoria virtual
  if (page < 0 || page >= S->numpags) {
    S->numillegalrefs++;
    return ~0U;
  }

  // consultamos la tabla de paginas virtuales
  if (!S->pgt[page].present) {
    handle_page_fault(S, virtual_addr);
  }

  //calculamos la direccion fisica 
  frame = S->pgt[page].frame;
  physical_addr = frame * S->pagsz + offset;

  reference_page(S, page, op);

  if (S->detailed) {
    printf("\t %c %u == P %d (F %d) + %d\n",
           op, virtual_addr, page, frame, offset);
  }

  return physical_addr;
}


void reference_page(ssystem* S, int page, char op) {
  if (op == 'R') {
    S->numrefsread++;
  } else if (op == 'W') {
    S->pgt[page].modified = 1;
    S->numrefswrite++;
  }
}

// fallo estilo FIFO
void handle_page_fault(ssystem* S, unsigned virtual_addr) {
  int page, frame, last;

  S->numpagefaults++;
  page = virtual_addr / S->pagsz;

  if (S->detailed) {
    printf("PAGE_FAULT in P %d\n", page);
  }

  // A) Hay frames libres en la lista circular
  if (S->listfree != -1) {
    last  = S->listfree;
    frame = S->frt[last].next;  // cabeza real

    if (frame == last) {
      // solo quedaba uno
      S->listfree = -1;
    } else {
      // saltamos la cabeza
      S->frt[last].next = S->frt[frame].next;
    }

    // Ocupar ese frame con 'page'
    if (S->detailed) printf("@ Taking free F%d\n", frame);
    // Vincula page<->frame, marca present, modified=0, etc.
    occupy_free_frame(S, frame, page);

    // En FIFO, todo frame recién ocupado va al FINAL de la cola
    fifo_enqueue(S, frame);
    if (S->detailed) printf("@ FIFO enqueue F%d\n", frame);
  }
  // B) No hay libres: víctima = frame más antiguo (frente de la cola)
  else {
    int victim_frame = fifo_dequeue(S);     // saca el frame más antiguo
    int victim_page  = S->frt[victim_frame].page;

    if (S->detailed) {
      printf("FIFO victim F%d (P%d)\n", victim_frame, victim_page);
    }

    //reemplazo sobre la página víctima
    replace_page(S, victim_page, page);

    // El frame sigue ocupado (ahora con 'page'): lo mandamos al FINAL
    fifo_enqueue(S, victim_frame);
    if (S->detailed) printf("FIFO enqueue F%d\n", victim_frame);
  }
}

int choose_page_to_be_replaced(ssystem* S) { return 0; }

void occupy_free_frame(ssystem* S, int frame, int page) {
  if (S->detailed) printf("Storing P%d in F%d\n", page, frame);

  // actualizamos la tabla de paginas 
  S->pgt[page].present  = 1;               //pagina presente en memoria 
  S->pgt[page].frame    = frame;           // donde 
  S->pgt[page].modified = 0;        

  // actualizamos la tabla de frames 
  S->frt[frame].page = page;
  
}
// shame as random 
void replace_page(ssystem* S, int victim, int newpage) {
  int frame = S->pgt[victim].frame;

  if (S->pgt[victim].modified) {
    if (S->detailed)
      printf("Writing modified P%d back (to disc) to replace it\n", victim);
    S->numpgwriteback++;
  }

  if (S->detailed)
    printf("Replacing victim P%d with P%d in F%d\n", victim, newpage, frame);

  S->pgt[victim].present = 0;

  S->pgt[newpage].present  = 1;
  S->pgt[newpage].frame    = frame;
  S->pgt[newpage].modified = 0;

  S->frt[frame].page = newpage;
}

// show results 
void print_page_table(ssystem* S) {
  int p;
  printf("%10s %10s %10s   %s\n", "PAGE", "Present", "Frame", "Modified");
  for (p = 0; p < S->numpags; p++)
    if (S->pgt[p].present)
      printf("%8d   %6d     %8d   %6d\n",
             p, S->pgt[p].present, S->pgt[p].frame, S->pgt[p].modified);
    else
      printf("%8d   %6d     %8s   %6s\n", p, S->pgt[p].present, "-", "-");
}

void print_frames_table(ssystem* S) {
  int p, f;
  printf("%10s %10s %10s   %s\n", "FRAME", "Page", "Present", "Modified");
  for (f = 0; f < S->numframes; f++) {
    p = S->frt[f].page;
    if (p == -1)
      printf("%8d   %8s   %6s     %6s\n", f, "-", "-", "-");
    else if (S->pgt[p].present)
      printf("%8d   %8d   %6d     %6d\n", f, p, S->pgt[p].present, S->pgt[p].modified);
    else
      printf("%8d   %8d   %6d     %6s   ERROR!\n", f, p, S->pgt[p].present, "-");
  }
}

void print_replacement_report(ssystem* S) {
  printf("FIFO replacement: victim = oldest frame in queue\n");
}