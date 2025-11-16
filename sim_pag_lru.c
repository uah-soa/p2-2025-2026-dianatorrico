// politica de remplazo LRU: las recently used 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./sim_paging.h"

//inicializacion de tablas 
void init_tables(ssystem* S) {
  int i;

  // Limpiar la tabla de páginas: present=0, frame=0, modified=0, timestamp=0
  memset(S->pgt, 0, sizeof(spage) * S->numpags);

  // Reloj global 
  S->clock = 0;   // se incrementa en cada referencia
  S->lru   = -1;  // nos basamos en timestamps

  // Construimos la lista CIRCULAR de frames libres:
  for (i = 0; i < S->numframes - 1; i++) {
    S->frt[i].page = -1;                    // libre
    S->frt[i].next = i + 1;                 // encadenamos el siguiente libre
  }
  // último elemento (i == numframes-1)
  S->frt[i].page = -1;
  S->frt[i].next = 0;      
  S->listfree    = i;      

  // Lista de ocupados no es necesaria para LRU por timestamp
  S->listoccupied = -1;
}

unsigned sim_mmu(ssystem* S, unsigned virtual_addr, char op) {
  unsigned physical_addr;
  int page, frame, offset;

  page   = virtual_addr / S->pagsz;
  offset = virtual_addr % S->pagsz;

  //validamos el rango
  if (page < 0 || page >= S->numpags) {
    S->numillegalrefs++;     // acceso fuera de rango
    return ~0U;              // dirección física inválida (sentinela)
  }

  //fallo de pagina 
  if (!S->pgt[page].present) {
    handle_page_fault(S, virtual_addr);
  }

  frame = S->pgt[page].frame;
  physical_addr = frame * S->pagsz + offset;

  // actualizamos el timestamp
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
    S->pgt[page].modified = 1;  // al escribir, la página queda modificada
    S->numrefswrite++;
  }
  //avanzamos el reloj global
  S->clock++;
  S->pgt[page].timestamp = S->clock;
}

void handle_page_fault(ssystem* S, unsigned virtual_addr) {
  int page, frame, last;

  S->numpagefaults++;
  page = virtual_addr / S->pagsz;

  if (S->detailed) {
    printf("PAGE_FAULT in P %d\n", page);
  }

  // A) Quedan frames libres en la lista circular
  if (S->listfree != -1) {
    // 'last' apunta al "último" del círculo
    last  = S->listfree;
    // el que tomamos es el siguiente del último
    frame = S->frt[last].next;

    if (frame == last) {
      //ya no hay libres
      S->listfree = -1;
    } else {
      // "Saltamos" la cabeza: último->next = cabeza->next
      S->frt[last].next = S->frt[frame].next;
    }

    // Vincular la página con el frame libre
    occupy_free_frame(S, frame, page);
  }
  // No hay libres -> reemplazo LRU
  else {
    int victim = choose_page_to_be_replaced(S); // elige la menos usada recientemente
    replace_page(S, victim, page);
  }
}

// eleccion de la pagina a reemplazar, la ultima menos utilizada 