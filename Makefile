all: menu

menu: menu.cpp
	g++ menu.cpp -Wall -lX11 -lGL -lGLU -lm ./libggfonts.a

clean:
	rm -f menu a.out


