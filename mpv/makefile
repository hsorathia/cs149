cc = gcc

most : 
	$(cc) -pthread -Wall --std=c11 pc.c -o pepe.out

debug : 
	$(cc) -pthread -Wall --std=c11 pc.c -o pepe.out -g

valgrind :
	valgrind --leak-check=full --show-leak-kinds=all ./pepe.out 399words 799words 1099words

clean :
	@echo "removing the output file"
	rm *.out
