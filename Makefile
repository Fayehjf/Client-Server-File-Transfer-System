all: client serverM serverA serverR serverD

client: client.c
	gcc client.c -o client

serverM: serverM.c
	gcc serverM.c -o serverM

serverA: serverA.c
	gcc serverA.c -o serverA

serverR: serverR.c
	gcc serverR.c -o serverR

serverD: serverD.c
	gcc serverD.c -o serverD

clean:
	rm -f client serverM serverA serverR serverD

