output: 
	gcc -g receiver.c -o receiver -lpthread
	gcc -g sender.c -o sender -lpthread
clean:
	rm receiver sender 