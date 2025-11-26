all: libmy1.so libmy2.so static dynamic

libmy1.so: lib1.c
	gcc -shared -fPIC lib1.c -o libmy1.so -lm

libmy2.so: lib2.c
	gcc -shared -fPIC lib2.c -o libmy2.so -lm


static: static_part.c lib1.c
	gcc static_part.c lib1.c -o static -static -lm


dynamic: dynamic_part.c
	gcc dynamic_part.c -o dynamic -ldl

clean:
	rm -f *.so static dynamic