/*
*
* Holy diver - an epic adventure at object-oriented world way beneath the surface!
* Template code for implementing the rich features to be specified later on.
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include <fstream>
#include <vector>

using namespace std;

/****************************************************/
// declaring functions:
/****************************************************/
void start_splash_screen(void);
void startup_routines(void);
void quit_routines(void);
void load_level(string); // a routine to load a level map from a file
int read_input(char*);
void update_state(char);  // assuming only one input char (key press) at most at a time ("turn-based" execution flow)
void render_screen(void);
void find_player_start_position(void); // find initial player position marked with 'P'
void move_player(int dx, int dy); // handle player movement
void reload_map(void); // reload the map from file


/****************************************************/
// global variables:
/****************************************************/
char** map;// pointer pointer equals to array of arrays = 2-dimensional array of chars
// above is virtually identical, as a variable, compared to for example:
//	    char map[MAXSTR][MAXLEN] = {{0}}; // declare a static 2-dim array of chars, initialize to zero
// However pointer to pointer has not allocated memory yet attached to it, this is done dynamically when actual size known

const int MAX_HEALTH = 100;
const int MAX_OXYGEN = 100;
const int INIT_LIVES = 3;
const int OXYGEN_PER_MOVE = 2; // oxygen consumed per move

int map_width = 0;
int map_height = 0;
string current_level_file = ""; // store current level filename for reloading

typedef struct Player {
	int health;
	int oxygen;
	int lives;
	int x;  // player x coordinate
	int y;  // player y coordinate
} Player; // named as "Player", a struct containing relevant player data is declared

Player player_data = { MAX_HEALTH, MAX_OXYGEN, INIT_LIVES, 0, 0 }; // initialize player data

/****************************************************************
 *
 * MAIN
 * main function contains merely function calls to various routines and the main game loop
 *
 ****************************************************/
int main(void)
{
	start_splash_screen();
	startup_routines();
	render_screen(); // Show initial map before first input
	char input;

	// IMPORTANT NOTE: do not exit program without cleanup: freeing allocated dynamic memory etc
	while (true) // infinite loop, should end with "break" in case of game over or user quitting etc.
	{
		input = '\0'; // make sure input resetted each cycle
		if (0 > read_input(&input)) break; // exit loop in case input reader returns negative (e.g. user selected "quit")
		update_state(input);
		render_screen();
	}

	quit_routines(); // cleanup, bye-bye messages, game save and whatnot
	return 0;
}

/****************************************************************
 *
 * FUNCTION load_level
 *
 * Open a map file and load level map from it.
 * First weekly home assignment is to be implemented mostly here.
 *
 * **************************************************************/
void load_level(string filepath)
{
	// Free existing map if it exists
	if (map != NULL)
	{
		for (int y = 0; y < map_height; y++)
		{
			free(map[y]);
		}
		free(map);
		map = NULL;
	}

	ifstream file(filepath.c_str());

	if (!file.is_open())
	{
		cout << "ERROR: Could not open map file!" << endl;
		exit(1);
	}

	vector<string> lines;
	string line;

	// Read all lines first
	while (getline(file, line))
	{
		if (!line.empty())
			lines.push_back(line);
	}

	file.close();

	map_height = lines.size();
	map_width = lines[0].length();

	// Allocate rows
	map = (char**)malloc(map_height * sizeof(char*));

	// Allocate columns
	for (int y = 0; y < map_height; y++)
	{
		map[y] = (char*)malloc(map_width * sizeof(char));
	}

	// Copy data into map
	for (int y = 0; y < map_height; y++)
	{
		for (int x = 0; x < map_width; x++)
		{
			map[y][x] = lines[y][x];
		}
	}

	// Store the current level filename for reload functionality
	current_level_file = filepath;

	// Find and set player starting position
	find_player_start_position();
}

/****************************************************************
 *
 * FUNCTION find_player_start_position
 *
 * Find the 'P' symbol on the map and set player coordinates
 *
 * **************************************************************/
void find_player_start_position(void)
{
	for (int y = 0; y < map_height; y++)
	{
		for (int x = 0; x < map_width; x++)
		{
			if (map[y][x] == 'P')
			{
				player_data.x = x;
				player_data.y = y;
				// Replace 'P' with 'o' (free space) on the map
				map[y][x] = 'o';
				return;
			}
		}
	}
}

/****************************************************************
 *
 * FUNCTION move_player
 *
 * Attempt to move player by dx, dy offset
 * Check if movement is valid (within bounds and not into obstacle)
 *
 * **************************************************************/
void move_player(int dx, int dy)
{
	int new_x = player_data.x + dx;
	int new_y = player_data.y + dy;

	// Check bounds
	if (new_x < 0 || new_x >= map_width || new_y < 0 || new_y >= map_height)
	{
		cout << "Cannot move outside the map!" << endl;
		return;
	}

	// Check if destination is an obstacle
	char destination = map[new_y][new_x];
	if (destination == 'x')
	{
		cout << "Cannot move into obstacle!" << endl;
		return;
	}

	// Movement is valid
	player_data.x = new_x;
	player_data.y = new_y;

	// Consume oxygen
	player_data.oxygen -= OXYGEN_PER_MOVE;
	if (player_data.oxygen < 0)
	{
		player_data.oxygen = 0;
	}

	// Check for enemies (M) - for now, just allow movement
	if (destination == 'M')
	{
		// Future: handle enemy encounter
	}
}

/****************************************************************
 *
 * FUNCTION reload_map
 *
 * Reload the current level from file and reset player data
 *
 * **************************************************************/
void reload_map(void)
{
	// Reset player stats
	player_data.health = MAX_HEALTH;
	player_data.oxygen = MAX_OXYGEN;
	player_data.lives = INIT_LIVES;

	// Reload the level
	load_level(current_level_file);

	cout << "Map reloaded! Game state reset." << endl;
}

/****************************************************************
 *
 * FUNCTION read_input
 *
 * read input from user
 *
 * **************************************************************/
int read_input(char* input)
{
	cout << ">>>";	// simple prompt
	try {
		cin >> *input;
	}
	catch (...) {
		return -1; // failure
	}
	cout << endl;  //new line to reset output/input
	cin.ignore();  //clear cin to avoid messing up following inputs
	if (*input == 'q') return -2; // quitting game...
	return 0; // default return, no errors
}

/****************************************************************
 *
 * FUNCTION update_state
 *
 * update game state (player, enemies, artefacts, inventories, health, whatnot)
 * this is a collective entry point to all updates - feel free to divide these many tasks into separate subroutines
 *
 * **************************************************************/
void update_state(char input)
{
	// Handle movement commands (WASD or arrow keys simulation)
	switch (input)
	{
	case 'w':  // up
	case 'W':
		move_player(0, -1);
		break;
	case 's':  // down
	case 'S':
		move_player(0, 1);
		break;
	case 'a':  // left
	case 'A':
		move_player(-1, 0);
		break;
	case 'd':  // right
	case 'D':
		move_player(1, 0);
		break;
	case 'r':  // reload map
	case 'R':
		reload_map();
		break;
	default:
		// Unknown command
		break;
	}
}

/****************************************************************
 *
 * FUNCTION render_screen
 *
 * this function prints out all visuals of the game (typically after each time game state updated)
 *
 * **************************************************************/
void render_screen(void)
{
	system("cls"); // Windows
	// system("clear"); // Linux / macOS

	// Display game stats
	cout << "=== HOLY DIVER ===" << endl;
	cout << "Health: " << player_data.health << "/" << MAX_HEALTH << " | ";
	cout << "Oxygen: " << player_data.oxygen << "/" << MAX_OXYGEN << " | ";
	cout << "Lives: " << player_data.lives << endl;
	cout << "==================" << endl << endl;

	// Render the map with player position
	for (int y = 0; y < map_height; y++)
	{
		for (int x = 0; x < map_width; x++)
		{
			// Draw player at their current position
			if (x == player_data.x && y == player_data.y)
			{
				cout << 'P';
			}
			else
			{
				cout << map[y][x];
			}
		}
		cout << endl;
	}

	cout << endl;
	cout << "Controls: W=up, S=down, A=left, D=right, R=reload, Q=quit" << endl;
}

/****************************************************************
 *
 * FUNCTION start_splash_screen
 *
 * outputs any data or info at program start
 *
 * **************************************************************/
void start_splash_screen(void)
{
	/* this function to display any title information at startup, may include instructions or fancy ASCII-graphics */
	cout << endl << "WELCOME to epic Holy Diver v0.02" << endl;
	cout << "Use WASD to move, R to reload, Q to quit" << endl;
	cout << "Avoid obstacles (x), conserve oxygen!" << endl << endl;
	cin.ignore();
}

/****************************************************************
 *
 * FUNCTION startup_routines
 *
 * Function performs any tasks necessary to set up a game
 * May contain game load dialogue, new player/game dialogue, level loading, random inits, whatever else
 *
 * At first week, this could be suitable place to load level map.
 *
 * **************************************************************/
void startup_routines(void)
{
	load_level("level.txt"); // filename of your map
}

/****************************************************************
 *
 * FUNCTION quit_routines
 *
 * function performs any routines necessary at program shut-down, such as freeing memory or storing data files
 *
 * **************************************************************/
void quit_routines(void)
{
	// Free map memory
	if (map != NULL)
	{
		for (int y = 0; y < map_height; y++)
		{
			free(map[y]);
		}
		free(map);
	}

	cout << endl << "BYE! Welcome back soon." << endl;
}
