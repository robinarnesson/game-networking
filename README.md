# game-networking

A simple game-like networking program that use client-side prediction and entity interpolation. The project is based on Valves description of basic networking for the Source engine, read [here](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking).

Watch demo:
[http://youtu.be/A4RfSNtUu6c](http://youtu.be/A4RfSNtUu6c)

## Dependencies
[SDL 2](https://www.libsdl.org/download-2.0.php), [SDL2_gfx](http://cms.ferzkopp.net/index.php/software/13-sdl-gfx), [SDL\_image 2](https://www.libsdl.org/projects/SDL_image/), [GLM](http://glm.g-truc.net/0.9.6/index.html), [Boost](http://www.boost.org/) (boost serialization and boost asio)

## Usage
### Installation
Install all required libraries and run the following lines in the terminal:
```
git clone git@github.com:robinarnesson/game-networking.git
cd game-networking
make all
```
### Start server
Run in terminal:
```
./server 1024
```
### Start client(s)
Run in terminal:
```
./client localhost 1024
```
### Controls
* Move around with the arrow keys.
* Toggle debug mode with key '1'.
* Toggle prediction and interpolation with key '2'.