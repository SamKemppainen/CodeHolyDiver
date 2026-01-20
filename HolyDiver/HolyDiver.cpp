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

typedef struct Player {
	int health;
	int oxygen;
	int lives;
} Player; // named as "Player", a struct containing relevant player data is declared

Player player_data = { MAX_HEALTH, MAX_OXYGEN, INIT_LIVES }; // initialize player data

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

	int height = lines.size();
	int width = lines[0].length();

	// Allocate rows
	map = (char**)malloc(height * sizeof(char*));

	// Allocate columns
	for (int y = 0; y < height; y++)
	{
		map[y] = (char*)malloc(width * sizeof(char));
	}

	// Copy data into map
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			map[y][x] = lines[y][x];
		}
	}
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

	for (int y = 0; y < 20; y++)
	{
		for (int x = 0; x < 20; x++)
		{
			cout << map[y][x];
		}
		cout << endl;
	}
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
	cout << endl << "WELCOME to epic Holy Diver v0.01" << endl;
	cout << "Enter commands and enjoy! (press q to quit at all times)" << endl << endl;
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
	for (int y = 0; y < 20; y++)
	{
		free(map[y]);
	}
	free(map);

	cout << endl << "BYE! Welcome back soon." << endl;
}