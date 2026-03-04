/*
 * Holy Diver - an epic adventure in an object-oriented world beneath the surface!
 *
 * Week 1: Load map from file, display it.
 * Week 2: Player movement, oxygen consumption, map reload.
 * Week 3: Second map, enemy chase AI, death screen, life/respawn system.
 * Week 4: Fog of war, battery/illuminate system, enemy polymorphism
 *         (MovingEnemy / StationaryEnemy), World class owns all game objects.
 *
 * Controls:
 *   Move:       w=up  s=down  a=left  d=right
 *   Illuminate: i=up  k=down  j=left  l=right
 *   Other:      r=reload  n=next level  q=quit
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <string>
#include <iostream>
#include <vector>
#include "classes.h"

using namespace std;

/****************************************************/
// Function declarations
/****************************************************/
void start_splash_screen(void);
void startup_routines(void);
void quit_routines(void);
int  read_input(char*);
void update_state(char);
void render_screen(void);
void gameOver(void);
void death_screen(void);
void handle_player_death(void);
void next_level(void);

/****************************************************/
// Global game state
/****************************************************/
bool game_over = false;

vector<string> map_playlist;
int            current_map_index = 0;

// Constants
const int OXYGEN_PER_MOVE = 2;
const int OXYGEN_DAMAGE = 5;   // health lost per move when oxygen = 0
const int INIT_LIVES = 3;

// The two main game objects.
// Player lives here in main scope; World holds a pointer to it.
Player player_data;
World  world;

/****************************************************/
// gameOver - called by World or death logic to stop the loop
/****************************************************/
void gameOver()
{
    game_over = true;
}

/****************************************************/
// death_screen
/****************************************************/
void death_screen(void)
{
    cout << "\n";
    cout << "  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    cout << "  ~                                                  ~\n";
    cout << "  ~           G A M E   O V E R                     ~\n";
    cout << "  ~                                                  ~\n";
    cout << "  ~   Your light flickered out in the deep dark.     ~\n";
    cout << "  ~       The sea keeps all its secrets.             ~\n";
    cout << "  ~                                                  ~\n";
    cout << "  ~             No lives remaining.                  ~\n";
    cout << "  ~                                                  ~\n";
    cout << "  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n";
}

/****************************************************/
// handle_player_death
// Called whenever player health hits 0.
/****************************************************/
void handle_player_death(void)
{
    cout << "\n*** YOU DIED! ***\n";

    // Remove player marker from map before respawn
    world.setTile(player_data.x, player_data.y, 'o');

    bool survived = player_data.loseLife();

    if (!survived) {
        death_screen();
        gameOver();
        return;
    }

    cout << "Lives remaining: " << player_data.lives << "  -- Respawning...\n";

    // Reload the level to reset enemy positions; keep player's lives count
    int saved_lives = player_data.lives;
    world.loadFromFile(world.mapFilePath);
    player_data.lives = saved_lives;

    render_screen();
}

/****************************************************/
// next_level
/****************************************************/
void next_level(void)
{
    current_map_index++;
    if (current_map_index >= (int)map_playlist.size()) {
        cout << "\n";
        cout << "  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
        cout << "  ~                                                  ~\n";
        cout << "  ~         Y O U   W I N !                         ~\n";
        cout << "  ~                                                  ~\n";
        cout << "  ~   You have explored every depth of the sea.      ~\n";
        cout << "  ~   The treasure is yours, brave diver!            ~\n";
        cout << "  ~                                                  ~\n";
        cout << "  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n";
        gameOver();
        return;
    }

    cout << "\n--- Descending deeper... Level " << (current_map_index + 1) << " ---\n";
    player_data.health = 100;
    player_data.oxygen = 100;
    player_data.battery = 100;

    if (world.loadFromFile(map_playlist[current_map_index]) != 0) {
        cout << "Failed to load next level. Exiting.\n";
        gameOver();
    }
}

/****************************************************/
// read_input
/****************************************************/
int read_input(char* input)
{
    cout << ">>> Move: w/a/s/d  |  Illuminate: i/j/k/l  |  r=reload  n=next  q=quit : ";
    try { cin >> *input; }
    catch (...) { return -1; }
    cout << "\n";
    cin.ignore();
    if (*input == 'q') return -2;
    return 0;
}

/****************************************************/
// update_state
// Dispatches input to movement, illumination, or system commands.
/****************************************************/
void update_state(char input)
{
    if (game_over) return;

    // ---- System commands ----
    if (input == 'r') {
        cout << "Reloading map...\n";
        int saved_lives = player_data.lives;
        world.loadFromFile(world.mapFilePath);
        player_data.lives = saved_lives;
        return;
    }
    if (input == 'n') { next_level(); return; }

    // ---- Illuminate adjacent tile (i/j/k/l = up/left/down/right) ----
    // Illuminating does NOT count as a move and does NOT tick enemies
    if (input == 'i') { world.illuminateTile(0, -1); return; }
    if (input == 'k') { world.illuminateTile(0, 1); return; }
    if (input == 'j') { world.illuminateTile(-1, 0); return; }
    if (input == 'l') { world.illuminateTile(1, 0); return; }

    // ---- Movement (w/a/s/d) ----
    int dx = 0, dy = 0;
    if (input == 'w') dy = -1;
    else if (input == 's') dy = 1;
    else if (input == 'a') dx = -1;
    else if (input == 'd') dx = 1;
    else return; // unknown key - do nothing

    // tryMovePlayer handles fog-of-war encounters and wall blocking
    world.tryMovePlayer(dx, dy);

    // Oxygen is consumed whether or not the move succeeded (per spec:
    // "attempting to move into an obstacle consumes one turn's oxygen")
    player_data.reduceOxygen(OXYGEN_PER_MOVE);

    if (!player_data.hasOxygen()) {
        cout << "*** OUT OF OXYGEN! Taking damage! ***\n";
        player_data.takeDamage(OXYGEN_DAMAGE);
        if (!player_data.isAlive()) {
            handle_player_death();
            return;
        }
    }

    // Check if the player has died from a dark-tile encounter this move
    if (!player_data.isAlive()) {
        handle_player_death();
        return;
    }

    // Activate enemies that are now in the player's line of sight
    world.updateEnemyActivation();

    // Tick all enemies
    bool player_killed = world.updateEnemies();
    if (player_killed) {
        handle_player_death();
    }
}

/****************************************************/
// render_screen
/****************************************************/
void render_screen(void)
{
    if (game_over) return;
    if (!world.map) { cout << "(no map loaded)\n"; return; }

    cout << "\n";
    cout << "  [Level " << (current_map_index + 1)
        << "/" << map_playlist.size() << "]\n";

    world.render();  // World handles fog-of-war masking

    cout << "\n";
    cout << "  Oxygen:  " << player_data.oxygen << "%"
        << "  |  Health: " << player_data.health
        << "  |  Battery: " << player_data.battery << "%"
        << "  |  Lives: " << player_data.lives << "\n";
    cout << "\n";
}

/****************************************************/
// start_splash_screen
/****************************************************/
void start_splash_screen(void)
{
    cout << "\n";
    cout << "  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    cout << "  ~    W E L C O M E   T O   H O L Y   D I V E R   ~\n";
    cout << "  ~                     v0.04                        ~\n";
    cout << "  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n";
    cout << "  Move:        w=Up  s=Down  a=Left  d=Right\n";
    cout << "  Illuminate:  i=Up  k=Down  j=Left  l=Right  (costs 5% battery)\n";
    cout << "  Other:       r=Reload  n=Next level  q=Quit\n\n";
    cout << "  The map is DARK. Illuminate tiles before moving into them.\n";
    cout << "  Moving into darkness may reveal hidden enemies - painfully.\n";
    cout << "  M = moving enemy (chases you once activated)\n";
    cout << "  S = stationary enemy (hurts when you walk into it)\n";
    cout << "  You have " << INIT_LIVES << " lives. Good luck.\n\n";
}

/****************************************************/
// startup_routines
/****************************************************/
void startup_routines(void)
{
    srand((unsigned)time(nullptr));

    // Link World to the player object
    world.player = &player_data;

    cout << "Enter path for Level 1 map file: ";
    string lvl1; getline(cin, lvl1);
    map_playlist.push_back(lvl1);

    cout << "Enter path for Level 2 map file (Enter to skip): ";
    string lvl2; getline(cin, lvl2);
    if (!lvl2.empty()) map_playlist.push_back(lvl2);

    current_map_index = 0;

    if (world.loadFromFile(map_playlist[0]) != 0) {
        cout << "Failed to load map. Exiting.\n";
        exit(1);
    }

    render_screen();
}

/****************************************************/
// quit_routines
/****************************************************/
void quit_routines(void)
{
    world.freeMap();
    cout << "\nBYE! Welcome back soon.\n";
}

/****************************************************/
// MAIN
/****************************************************/
int main(void)
{
    start_splash_screen();
    startup_routines();

    char input;
    while (!game_over) {
        input = '\0';
        if (0 > read_input(&input)) break;
        update_state(input);
        if (!game_over) render_screen();
    }

    quit_routines();
    return 0;
}