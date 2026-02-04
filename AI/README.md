# Holy Diver - Homework 2 Implementation

## Changes Made

### 1. Player Position Tracking
- Added `x` and `y` coordinates to the `Player` struct
- Created `find_player_start_position()` function that:
  - Searches the map for the 'P' symbol
  - Sets the player's initial coordinates
  - Replaces 'P' with 'o' (free space) on the underlying map

### 2. Player Movement
- Implemented `move_player(int dx, int dy)` function that:
  - Validates movement (checks bounds and obstacles)
  - Prevents movement into 'x' (obstacle) tiles
  - Allows movement onto 'o' (free space) and 'M' (enemy) tiles
  - Updates player position if movement is valid

- Updated `update_state(char input)` to handle movement commands:
  - **W** or **w**: Move up
  - **S** or **s**: Move down
  - **A** or **a**: Move left
  - **D** or **d**: Move right
  - **R** or **r**: Reload map
  - **Q** or **q**: Quit game

### 3. Oxygen Consumption
- Added `OXYGEN_PER_MOVE` constant (set to 2)
- Player loses 2 oxygen points with each successful move
- Oxygen level is displayed in the game stats after each render

### 4. Map Reload Functionality
- Implemented `reload_map()` function that:
  - Resets player health, oxygen, and lives to initial values
  - Reloads the level from the original file
  - Finds the new player starting position

- Added `current_level_file` global variable to store the current level filename
- Pressing 'R' triggers the reload

### 5. Enhanced Rendering
- Updated `render_screen()` to:
  - Display game stats (Health, Oxygen, Lives) at the top
  - Draw the player 'P' at their current position on the map
  - Show control instructions at the bottom

### 6. Dynamic Map Size
- Added `map_width` and `map_height` global variables
- Map dimensions are now determined from the loaded file
- Fixed hardcoded loop bounds in `render_screen()`

### 7. Memory Management
- Updated `load_level()` to free existing map memory before loading new map
- Updated `quit_routines()` to use dynamic map dimensions
- Added null checks for safer memory handling

## How to Compile and Run

### On Windows (with MinGW or similar):
```bash
g++ -o holydiver holydiver.cpp
holydiver.exe
```

### On Linux/macOS:
```bash
g++ -o holydiver holydiver.cpp
./holydiver
```

**Note:** If using Linux/macOS, change line 311 in the code:
```cpp
// system("cls"); // Windows
system("clear"); // Linux / macOS
```

## Controls
- **W**: Move up
- **S**: Move down
- **A**: Move left
- **D**: Move right
- **R**: Reload map (reset game)
- **Q**: Quit game

## Map Symbols
- **P**: Player (shown at current position)
- **o**: Free space (walkable)
- **x**: Obstacle (not walkable)
- **M**: Enemy (walkable, no interaction yet)

## Gameplay Notes
- Each move consumes 2 oxygen points
- Player starts with 100 oxygen
- Movement is prevented when trying to move into obstacles
- Reloading the map resets all player stats and position
- The game displays current health, oxygen, and lives after each action

## Sample level.txt Format
The included `level.txt` is a 20x20 grid with:
- Walls around the perimeter (x)
- Free spaces to walk (o)
- Several enemies scattered around (M)
- Player start position marked with P (gets replaced with 'o' after loading)

## Future Enhancements (for later assignments)
- Enemy interaction when stepping on 'M' tiles
- Item collection
- Combat system
- Multiple levels
- Save/load game state
- Game over when oxygen reaches 0
