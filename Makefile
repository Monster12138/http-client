all:http-client
http-client:http-client.c
	gcc $^ -o $@
clean:
	rm -rf http-client
