all: gen_trace count_ops calculate_ws sim_pag_random sim_pag_fifo

# Add progressively to all: sim_pag_random sim_pag_lru sim_pag_fifo sim_pag_fifo2ch

gen_trace: gen_trace.o sort.o sort.h
	gcc -g -Wall -o gen_trace gen_trace.o sort.o

gen_trace.o: gen_trace.c sort.h
	gcc -g -Wall -c -o gen_trace.o gen_trace.c

sort.o: sort.c sort.h
	gcc -g -Wall -c -o sort.o sort.c

count_ops: count_ops.c
	gcc -g -Wall -o count_ops count_ops.c

calculate_ws: calculate_ws.c
	gcc -g -Wall -o calculate_ws calculate_ws.c

sim_pag_random: sim_pag_random.o sim_pag_main.o
	gcc -g -Wall -o sim_pag_random sim_pag_random.o sim_pag_main.o

sim_pag_random.o: sim_pag_random.c sim_paging.h
	gcc -g -Wall -c -o sim_pag_random.o sim_pag_random.c

sim_pag_lru: sim_pag_lru.o sim_pag_main.o
	gcc -g -Wall -o sim_pag_lru sim_pag_lru.o sim_pag_main.o

sim_pag_lru.o: sim_pag_lru.c sim_paging.h
	gcc -g -Wall -c -o sim_pag_lru.o sim_pag_lru.c

sim_pag_fifo: sim_pag_fifo.o sim_pag_main.o
	gcc -g -Wall -o sim_pag_fifo sim_pag_fifo.o sim_pag_main.o

sim_pag_fifo.o: sim_pag_fifo.c sim_paging.h
	gcc -g -Wall -c -o sim_pag_fifo.o sim_pag_fifo.c

sim_pag_fifo2ch: sim_pag_fifo2ch.o sim_pag_main.o
	gcc -g -Wall -o sim_pag_fifo2ch sim_pag_fifo2ch.o sim_pag_main.o

sim_pag_fifo2ch.o: sim_pag_fifo2ch.c sim_paging.h
	gcc -g -Wall -c -o sim_pag_fifo2ch.o sim_pag_fifo2ch.c

sim_pag_main.o: sim_pag_main.c sim_paging.h
	gcc -g -Wall -c -o sim_pag_main.o sim_pag_main.c

clean:
	rm -f gen_trace.o sort.o gen_trace
	rm -f count_ops
	rm -f calculate_ws
	rm -f sim_pag_main.o
	rm -f sim_pag_random.o sim_pag_random
	rm -f sim_pag_lru.o sim_pag_lru
	rm -f sim_pag_fifo.o sim_pag_fifo
	rm -f sim_pag_fifo2ch.o sim_pag_fifo2ch
	rm -f *.plist

