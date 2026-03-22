/*
 * Holy Diver - a turn-based underwater adventure game.
 *
 * Weekly progress:
 *   Week 1: Load map from file, display it.
 *   Week 2: Player movement, oxygen consumption, map reload ('r').
 *   Week 3: Second map, enemy chase AI, death screen, lives/respawn system.
 *   Week 4: Fog of war, battery/flashlight system, enemy polymorphism
 *            (MovingEnemy / StationaryEnemy), World class as movement authority.
 *   Week 5: Treasure collectibles ($) - collect all to advance to next level.
 *
 * Controls:
 *   Move:       w=up  s=down  a=left  d=right
 *   Illuminate: i=up  k=down  j=left  l=right   (costs 5% battery)
 *   Other:      r=reload  n=next level  q=quit
 *
 * Map tile legend:
 *   x = wall/obstacle      o = open floor
 *   P = player start       M = moving enemy
 *   S = stationary enemy   $ = treasure (collect all to advance)
 */






/* MY THOUGHTS
*Well, I think I should address the elephant in the room and say that yes AI was used very heavily with this task.
*The progress I used was that I first made a primitive version on my own (now deleted). 
*Then with some understanding of the main points I used Ai to help me build a proper version of the game.
*I then spend my time analyzing and asking about the methods used, the "whys" and "hows" of it.
*



*/

#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <string>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include "classes.h"

using namespace std;

// ---------------------------------------------------------------
// DESIGN NOTE - Global gameOver() function
//
// The assignment spec says Player's destructor must call a global
// gameOver() function to signal the main loop to exit.
// This requires gameOver() to be a free (non-member) function so
// the Player class (defined in classes.h) can call it via forward
// declaration. The GameManager class below wraps everything else,
// but this one global function is kept to satisfy the spec.
//
// In a larger project you would use an event system, a callback,
// or a singleton GameManager to avoid this global entirely.
// ---------------------------------------------------------------
bool g_game_over = false;   // the one global: set by gameOver(), read by GameManager::run()

// gameOver() - sets the flag that stops the main game loop.
// Called by Player's destructor and by win/death logic inside GameManager.
void gameOver() {
    g_game_over = true;
}


/*==========================================================
 * CLASS: GameManager
 *
 * The backbone of the game. Owns the Player and World objects,
 * runs the main game loop, and handles all input and state updates.
 *
 * TEACHING POINT - GameManager / single backbone class:
 *   The assignment notes that "a well-designed OOP program rarely
 *   contains functions separate from classes or global variables"
 *   and suggests having one central class with a run() method.
 *   GameManager does exactly this: main() creates one GameManager
 *   and calls run(). All game logic lives inside the class as methods
 *   instead of loose global functions.
 *
 *   Benefits:
 *   - All game state (player, world, level index, etc.) is owned by
 *     one object, not scattered across globals.
 *   - Methods can access each other and member data without passing
 *     large argument lists or using globals.
 *   - The class can be instantiated, reset, or extended cleanly.
 *==========================================================*/
class GameManager {
public:
    // ---- Game balance constants ----
    // Declared as static const because they belong to the game concept,
    // not to any particular instance. static means shared across all
    // instances; const means they cannot be changed at runtime.
    static const int OXYGEN_PER_MOVE = 2;   // oxygen lost per turn (even on failed moves)
    static const int OXYGEN_DAMAGE = 5;   // health lost per turn when oxygen hits 0
    static const int INIT_LIVES = 3;   // starting lives

private:
    // ---- Owned game objects ----
    // Player and World are member objects (not pointers) - they are
    // created when GameManager is created and destroyed with it.
    Player player;   // the human player's state
    World  world;    // the game world: map, enemies, items

    // ---- Level management ----
    vector<string> mapPlaylist;     // file paths for each level, in order
    int            currentLevel;    // index into mapPlaylist (0-based)

public:
    // ---------------------------------------------------------------
    // Constructor
    // Initialises the level counter. Player and World have their own
    // constructors that run automatically here.
    // ---------------------------------------------------------------
    GameManager() : currentLevel(0) {
        // Link World to Player so World can read player position and apply damage.
        // World holds a non-owning pointer (does not delete it).
        world.player = &player;
    }

    // Destructor - Player and World destructors run automatically.
    // Player's destructor will call gameOver(), which is fine.
    ~GameManager() {}

    // Copying a GameManager makes no sense - there is only one game.
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // ---------------------------------------------------------------
    // run()
    // The single public entry point. Sets up the game and runs the
    // input/update/render loop until the game ends.
    // main() calls this and nothing else.
    // ---------------------------------------------------------------
    void run() {
        splashScreen();
        if (!startup()) return;     // startup() returns false if map loading fails

        char input;
        while (!g_game_over) {
            input = '\0';
            if (readInput(&input) < 0) break;   // 'q' or read error
            updateState(input);
            if (!g_game_over) renderScreen();   // redraw after every non-illuminate action
        }

        shutdown();
    }

private:
    // ---------------------------------------------------------------
    // startup()
    // Seeds RNG, asks for map file path(s), loads the first level.
    // Returns false if the first map cannot be loaded (fatal error).
    // ---------------------------------------------------------------
    bool startup() {
        srand((unsigned)time(nullptr));  // seed the random number generator

        // Ask the user for one or two map file paths
        cout << "Enter path for Level 1 map file: ";
        string lvl1; getline(cin, lvl1);
        mapPlaylist.push_back(lvl1);

        cout << "Enter path for Level 2 map file (press Enter to skip): ";
        string lvl2; getline(cin, lvl2);
        if (!lvl2.empty()) mapPlaylist.push_back(lvl2);

        currentLevel = 0;

        // Attempt to load the first level
        if (world.loadFromFile(mapPlaylist[0]) != 0) {
            cout << "Failed to load map. Exiting.\n";
            return false;
        }

        renderScreen();
        return true;
    }

    // ---------------------------------------------------------------
    // shutdown()
    // Cleanup and goodbye message before the program exits.
    // ---------------------------------------------------------------
    void shutdown() {
        world.freeMap();
        printSlow("\nBYE! Welcome back soon.\n", 40);
    }

    // ---------------------------------------------------------------
    // readInput(input)
    // Reads one character from the keyboard.
    // Returns -2 if the player pressed 'q' (quit).
    // Returns -1 on a read error.
    // Returns  0 on success (character stored in *input).
    // ---------------------------------------------------------------
    int readInput(char* input) {
        cout << ">>> Move: w/a/s/d  |  Illuminate: i/j/k/l  |  r=reload  n=next  q=quit : ";
        try { cin >> *input; }
        catch (...) { return -1; }
        cout << "\n";
        cin.ignore();
        if (*input == 'q') return -2;
        return 0;
    }

    // ---------------------------------------------------------------
    // updateState(input)
    // Dispatches a single key to the correct handler.
    // Order of checks: system keys -> illuminate keys -> movement keys.
    // ---------------------------------------------------------------
    void updateState(char input) {
        if (g_game_over) return;

        // ---- System commands ----
        if (input == 'r') {
            // Reload the current map from disk.
            // Save and restore lives so the reload is a level reset,
            // not a full game reset.
            cout << "Reloading map...\n";
            int savedLives = player.lives;
            world.loadFromFile(world.mapFilePath);
            player.lives = savedLives;
            return;
        }
        if (input == 'n') { nextLevel(); return; }

        // ---- Illuminate (flashlight) commands: i/j/k/l ----
        // Illuminating reveals tiles without moving or ticking enemies.
        // After the reveal we redraw immediately, then pause for Enter, this was a dumb fix for a bug of sorts.
        // so the player can study the newly lit area before their next move.
        if (input == 'i') { world.illuminateTile(0, -1); renderScreen(); waitEnter(); return; }  // up
        if (input == 'k') { world.illuminateTile(0, 1); renderScreen(); waitEnter(); return; }  // down
        if (input == 'j') { world.illuminateTile(-1, 0); renderScreen(); waitEnter(); return; }  // left
        if (input == 'l') { world.illuminateTile(1, 0); renderScreen(); waitEnter(); return; }  // right

        // ---- Movement commands: w/a/s/d ----
        // Translate the key into a direction vector (dx, dy).
        // dx = column change: negative = left, positive = right
        // dy = row change:    negative = up,   positive = down
        int dx = 0, dy = 0;
        if (input == 'w') dy = -1;
        else if (input == 's') dy = 1;
        else if (input == 'a') dx = -1;
        else if (input == 'd') dx = 1;
        else return;   // unrecognised key - ignore silently

        // Ask World to validate and execute the move.
        // World returns false if the move is blocked (wall, enemy, boundary).
        // Oxygen is spent regardless - attempting a blocked move still uses a turn.
        world.tryMovePlayer(dx, dy);

        // Reduce oxygen by the per-move cost (even if the move was blocked)
        player.reduceOxygen(OXYGEN_PER_MOVE);

        // If oxygen has run out, start draining health each turn
        // This is the "advanced" version: depleting oxygen doesn't instantly kill;
        // it starts a health drain, giving the player a brief window to refill.
        if (!player.hasOxygen()) {
            printSlow("*** OUT OF OXYGEN! Taking damage! ***\n", 40);
            player.takeDamage(OXYGEN_DAMAGE);
            if (!player.isAlive()) { handleDeath(); return; }
        }

        // An enemy ambush inside tryMovePlayer (dark tile) may have killed the player
        if (!player.isAlive()) { handleDeath(); return; }

        // Activate any enemies that are now inside the player's visible area
        world.updateEnemyActivation();

        // Tick all active enemies one step; returns true if one reached the player
        bool killedByEnemy = world.updateEnemies();
        if (killedByEnemy) { handleDeath(); return; }

        // Check the treasure win condition after every successful move.
        // allTreasuresFound() only returns true when the level HAS treasures,
        // so levels without '$' tiles are not affected.
        if (world.allTreasuresFound()) {
            printSlow("\n*** ALL TREASURES COLLECTED! The way forward opens... ***\n", 40);
            nextLevel();
        }
    }

    // ---------------------------------------------------------------
    // renderScreen()
    // Draws the map (with fog of war) and the player's stat bar.
    // ---------------------------------------------------------------
    void renderScreen() {
        if (g_game_over) return;
        if (!world.map) { cout << "(no map loaded)\n"; return; }

        cout << "\n";
        cout << "  [Level " << (currentLevel + 1)
            << "/" << mapPlaylist.size() << "]\n";

        // World::render() handles the fog-of-war filtering and the
        // animated scan-line effect on first draw after a load.
        world.render();

        // Status bar: show all resource stats and the treasure counter
        pair<int, int> tc = world.treasureCount();
        int tFound = tc.first, tTotal = tc.second;

        cout << "\n";
        cout << "  Oxygen: " << player.oxygen << "%"
            << "  |  Health: " << player.health
            << "  |  Battery: " << player.battery << "%"
            << "  |  Lives: " << player.lives;

        // Only show the treasure counter on levels that actually have treasure
        if (tTotal > 0)
            cout << "  |  Treasure: " << tFound << "/" << tTotal;

        cout << "\n\n";
    }

    // ---------------------------------------------------------------
    // handleDeath()
    // Called whenever the player's health reaches 0.
    // Loses one life and either respawns or ends the game.
    // ---------------------------------------------------------------
    void handleDeath() {
        printSlow("\n*** YOU DIED! ***\n", 60);

        // Clear the player's tile on the map before reloading
        world.setTile(player.x, player.y, 'o');

        bool survived = player.loseLife();

        if (!survived) {
            // No lives left - show the game over screen and stop
            deathScreen();
            gameOver();
            return;
        }

        cout << "Lives remaining: " << player.lives << "  -- Respawning...\n";

        // Reload the level to reset enemy positions.
        // Save the life count so the reload doesn't reset it.
        int savedLives = player.lives;
        world.loadFromFile(world.mapFilePath);
        player.lives = savedLives;

        renderScreen();
    }

    // ---------------------------------------------------------------
    // nextLevel()
    // Advances to the next map in the playlist.
    // If the playlist is finished, shows the win screen and exits.
    // ---------------------------------------------------------------
    void nextLevel() {
        currentLevel++;

        if (currentLevel >= (int)mapPlaylist.size()) {
            // All levels complete - player wins
            printSlow("\n", 15);
            printSlow("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", 15);
            printSlow("  ~                                                  ~\n", 15);
            printSlow("  ~         Y O U   W I N !                         ~\n", 50);
            printSlow("  ~                                                  ~\n", 15);
            printSlow("  ~   You have explored every depth of the sea.      ~\n", 30);
            printSlow("  ~   The treasure is yours, brave diver!            ~\n", 30);
            printSlow("  ~                                                  ~\n", 15);
            printSlow("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n", 15);
            gameOver();
            return;
        }

        printSlow("\n--- Descending deeper... Level "
            + to_string(currentLevel + 1) + " ---\n", 40);

        // Reset player stats for the new level (lives carry over)
        player.health = 100;
        player.oxygen = 100;
        player.battery = 100;

        if (world.loadFromFile(mapPlaylist[currentLevel]) != 0) {
            cout << "Failed to load next level. Exiting.\n";
            gameOver();
        }
    }

    // ---------------------------------------------------------------
    // deathScreen()
    // Printed when the player runs out of lives entirely.
    // Uses printSlow() for a dramatic effect.
    // ---------------------------------------------------------------
    void deathScreen() {
        printSlow("\n");
        printSlow("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", 15);
        printSlow("  ~                                                  ~\n", 15);
        printSlow("  ~           G A M E   O V E R                     ~\n", 50);
        printSlow("  ~                                                  ~\n", 15);
        printSlow("  ~   Your light flickered out in the deep dark.     ~\n", 30);
        printSlow("  ~       The sea keeps all its secrets.             ~\n", 30);
        printSlow("  ~                                                  ~\n", 15);
        printSlow("  ~             No lives remaining.                  ~\n", 30);
        printSlow("  ~                                                  ~\n", 15);
        printSlow("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n", 15);
    }

    // ---------------------------------------------------------------
    // splashScreen()
    // Title card and control reference printed at startup.
    // ---------------------------------------------------------------
    void splashScreen() {
        printSlow("\n");
        printSlow("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", 15);
        printSlow("  ~                                                  ~\n", 15);
        printSlow("  ~    W E L C O M E   T O   H O L Y   D I V E R   ~\n", 50);
        printSlow("  ~                     v0.05                        ~\n", 25);
        printSlow("  ~                                                  ~\n", 15);
        printSlow("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n", 15);
        printSlow("  Move:        w=Up  s=Down  a=Left  d=Right\n", 20);
        printSlow("  Illuminate:  i=Up  k=Down  j=Left  l=Right  (costs 5% battery)\n", 20);
        printSlow("  Other:       r=Reload  n=Next level  q=Quit\n\n", 20);
        printSlow("  The map is DARK. Illuminate tiles before moving into them.\n", 25);
        printSlow("  Moving into darkness may reveal hidden enemies - painfully.\n", 25);
        printSlow("  M = moving enemy (chases you once activated)\n", 25);
        printSlow("  S = stationary enemy (hurts when you walk into it)\n", 25);
        printSlow("  $ = treasure  (collect ALL to advance to the next level)\n", 25);
        printSlow("  You have " + to_string(INIT_LIVES) + " lives. Good luck.\n\n", 40);
    }

    // ---------------------------------------------------------------
    // printSlow(text, ms)
    // Prints a string one character at a time with a delay between each.
    // ms = milliseconds between characters (lower = faster).
    // Used on screens where a slow, dramatic reveal fits the mood.
    // ---------------------------------------------------------------
    void printSlow(const string& text, int ms = 25) {
        for (char c : text) {
            cout << c << flush;
            this_thread::sleep_for(chrono::milliseconds(ms));
        }
    }

    // ---------------------------------------------------------------
    // waitEnter()
    // Pauses and waits for the player to press Enter.
    // Used after flashlight illumination so the player can read the
    // newly revealed tiles before the prompt reappears.
    // ---------------------------------------------------------------
    void waitEnter() {
        cout << "  (Press Enter to continue...)\n";
        cin.get();
    }
};


/*==========================================================
 * main()
 *
 * Entry point. Creates one GameManager and calls run().
 * That's it - all logic is inside GameManager.
 *
 * TEACHING POINT - Minimal main():
 *   The assignment suggests main() should just instantiate one
 *   class and call one method. A thin main() is easy to read
 *   and makes it obvious where the program starts.
 *==========================================================*/
int main() {
    GameManager game;
    game.run();
    return 0;
}