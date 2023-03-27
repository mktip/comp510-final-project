all:
	g++ src/*.cpp -o app -I$(LD_LIBRARY_PATH) -L/nix/store/8jny6z8z1hbi67njcabxcbpnrbwsz2hf-glew-2.2.0-dev/include -L$(PWD)/include -I$(PWD)/include -lGL -lglfw -lGLEW

run:
	./app
