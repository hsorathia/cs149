cc = gcc

unique : 
	   $(cc) -pthread -Wall uc.c -o pepe.out

debug : 
	$(cc) -pthread -Wall --std=c11 uc.c -o pepe.out -g

clean :
	   @echo "removing the output file"
	   rm *.out
