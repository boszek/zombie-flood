## Zombie flood is a mix of fluid simuation and zombie game. It might remind you THey are Billions

## Rules
There is a center fortress that is the last hold and all zombies from the surroundings are trying to get to it, to flood it. 

## Technology
Game uses C++, OpenGL and SDL2. It is multi platform, but for now focuses on MacOS.

## Implementation plan
### First iteration - simulation of fluid/zombies, non interactive, we simply observe how zombies interact with 
1. have application skeleton with SDL2 and OpenGL, running in a window, displays a 2D map from bitmap, allows scrolling and zooming the map
1. Generate a random map bitmap, reminding an abandoned city after apocalypse. Another bitmap represents obstacles, which will be the the input for fluid simulation
1. Implement simple fluid simulation - particles are simple boxes representing fluid/zombies, interacting with obstacles and each other, the goal of each particle is to get to the center fortress
1. Zombies, fluid spawns at random positions on the edges of the map
1. When fluid reaches center it disappears (like in a sink) and we count how many zombies/particles reached it

### Second iteration - Interactivity and Visuals
1. **Interactive Controls**:
    - Add control bars to modify simulation speed and zombie size.
    - Add a pause button.
    - Enable map scrolling via mouse drag (left button).
2. **Visual Polish**:
    - Use `assets/zombie1.png` for zombie sprite.
    - Rotate sprites to face their movement direction.
