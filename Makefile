all:
	g++ src/main.cpp src/InitShader.cpp -o app -I$(PWD)/include -lGL -lglfw -lGLEW

run:
	./app
