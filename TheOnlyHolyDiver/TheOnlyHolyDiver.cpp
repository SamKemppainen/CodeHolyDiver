/*
*
* Holy diver - an epic adventure at object-oriented world way beneath the surface!
* Week 1: Load map from file, display it.
* Week 2: Player movement (1=up,2=down,3=left,4=right), oxygen consumption, map reload.
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include "classes.h"

using namespace std;

/****************************************************/
// declaring functions:
/****************************************************/
void start_splash_screen(void);
void startup_routines(void);
void quit_routines(void);
int load_level(string);
int read_input(char*);
void update_state(char);
void render_screen(void);
void free_map(void);
void gameOver(void);

/****************************************************/
// global variables:
/****************************************************/
char** map;
int map_rows = 0;
int map_cols = 0;
string current_map_path = "";
bool game_over = false; // set to true by gameOver() to break the main loop

const int MAX_HEALTH = 100;
const int MAX_OXYGEN = 100;
const int INIT_LIVES = 3;
const int OXYGEN_PER_MOVE = 2;

Player player_data; // uses Player class from classes.h, defaults: health=100, oxygen=100, lives=3

void gameOver()
{
    game_over = true;
}

/****************************************************/
// MAIN
/****************************************************/
int main(void)
{
    start_splash_screen();
    startup_routines();
    char input;

    while (!game_over)
    {
        input = '\0';
        if (0 > read_input(&input)) break;
        update_state(input);
        render_screen();
    }

    quit_routines();
    return 0;
}

/****************************************************/
// FUNCTION free_map
// Frees dynamically allocated map memory
/****************************************************/
void free_map(void)
{
    if (map != nullptr) {
        for (int i = 0; i < map_rows; i++) {
            free(map[i]);
        }
        free(map);
        map = nullptr;
    }
    map_rows = 0;
    map_cols = 0;
}

/****************************************************/
// FUNCTION load_level
// Opens map file, allocates 2D char array, reads map.
// Returns 0 on success, -1 on failure.
/****************************************************/
int load_level(string filepath)
{
    ifstream file(filepath);
    if (!file.is_open()) {
        cout << "Error: Could not open file: " << filepath << endl;
        return -1;
    }

    // Read all lines into a vector first to know dimensions
    vector<string> lines;
    string line;
    while (getline(file, line)) {
        if (!line.empty())
            lines.push_back(line);
    }
    file.close();

    if (lines.empty()) {
        cout << "Error: Map file is empty." << endl;
        return -1;
    }

    // Free old map if any
    free_map();

    map_rows = lines.size();
    map_cols = lines[0].size();

    // Allocate 2D array
    map = (char**)malloc(map_rows * sizeof(char*));
    if (!map) { cout << "Error: Memory allocation failed." << endl; return -1; }

    for (int i = 0; i < map_rows; i++) {
        map[i] = (char*)malloc((map_cols + 1) * sizeof(char));
        if (!map[i]) { cout << "Error: Memory allocation failed." << endl; return -1; }
        // Copy line into map row
        for (int j = 0; j < map_cols; j++) {
            map[i][j] = (j < (int)lines[i].size()) ? lines[i][j] : 'x';
        }
        map[i][map_cols] = '\0';
    }

    // Find player starting position 'P'
    for (int i = 0; i < map_rows; i++) {
        for (int j = 0; j < map_cols; j++) {
            if (map[i][j] == 'P') {
                player_data.y = i;
                player_data.x = j;
            }
        }
    }

    return 0;
}

/****************************************************/
// FUNCTION read_input
/****************************************************/
int read_input(char* input)
{
    cout << ">>> (1=up 2=down 3=left 4=right | r=reload | q=quit): ";
    try {
        cin >> *input;
    }
    catch (...) {
        return -1;
    }
    cout << endl;
    cin.ignore();
    if (*input == 'q') return -2;
    return 0;
}

/****************************************************/
// FUNCTION update_state
// Handles player movement, oxygen, and map reload
/****************************************************/
void update_state(char input)
{
    if (input == 'r') {
        cout << "Reloading map..." << endl;
        load_level(current_map_path);
        player_data.reset(player_data.x, player_data.y);
        return;
    }

    int new_x = player_data.x;
    int new_y = player_data.y;

    if (input == '1') new_y--;  // up
    else if (input == '2') new_y++;  // down
    else if (input == '3') new_x--;  // left
    else if (input == '4') new_x++;  // right
    else return; // unknown input, do nothing

    // Bounds check
    if (new_y < 0 || new_y >= map_rows || new_x < 0 || new_x >= map_cols) {
        cout << "Can't move there!" << endl;
        return;
    }

    char target = map[new_y][new_x];

    // Allow movement onto 'o', 'P', or 'M' tiles; block 'x'
    if (target == 'x') {
        cout << "Blocked by obstacle!" << endl;
        return;
    }

    // Move player: clear old position, set new
    map[player_data.y][player_data.x] = 'o';
    player_data.x = new_x;
    player_data.y = new_y;
    map[player_data.y][player_data.x] = 'P';

    // Reduce oxygen
    player_data.reduceOxygen(OXYGEN_PER_MOVE);

    if (!player_data.hasOxygen()) {
        cout << "*** OUT OF OXYGEN! ***" << endl;
    }
}

/****************************************************/
// FUNCTION render_screen
/****************************************************/
void render_screen(void)
{
    if (map == nullptr) {
        cout << "(no map loaded)" << endl;
        return;
    }

    cout << endl;
    for (int i = 0; i < map_rows; i++) {
        cout << map[i] << endl;
    }
    cout << endl;
    cout << "Oxygen: " << player_data.oxygen << "% | "
        << "Health: " << player_data.health << " | "
        << "Lives: " << player_data.lives << endl;
    cout << endl;
}

/****************************************************/
// FUNCTION start_splash_screen
/****************************************************/
void start_splash_screen(void)
{
    cout << endl << "WELCOME to epic Holy Diver v0.02" << endl;
    cout << "Controls: 1=Up  2=Down  3=Left  4=Right  r=Reload  q=Quit" << endl << endl;
}

/****************************************************/
// FUNCTION startup_routines
/****************************************************/
void startup_routines(void)
{
    cout << "Enter map file path: ";
    getline(cin, current_map_path);

    if (load_level(current_map_path) != 0) {
        cout << "Failed to load map. Exiting." << endl;
        exit(1);
    }

    render_screen(); // show map right after loading (week 1 requirement)
}

/****************************************************/
// FUNCTION quit_routines
/****************************************************/
void quit_routines(void)
{
    free_map();
    cout << endl << "BYE! Welcome back soon." << endl;
}