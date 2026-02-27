all: menu

menu: menu.cpp
	g++ menu.cpp -Wall -o menu -lX11 -lGL -lGLU -lm ./libggfonts.a

clean:
	rm -f menu a.out


