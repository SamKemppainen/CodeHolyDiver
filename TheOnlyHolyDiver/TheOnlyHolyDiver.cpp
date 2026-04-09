/*
 * Holy Diver - a turn-based underwater adventure game, very thrilling...
 *
 * Controls:
 *   Move:       w=up  s=down  a=left  d=right
 *   Illuminate: i=up  k=down  j=left  l=right   (costs 5% battery)
 *   Other:      r=reload  n=next level  q=quit
 *
 * Map tile symbols:
 *   x = wall/obstacle      o = open floor
 *   P = player start       M = moving enemy
 *   S = stationary enemy   $ = treasure (collect all to advance)
 *   B = battery pack       O = oxygen canister    G = gold coin
 */

 /* MY THOUGHTS
 *Well, I think I should address the elephant in the room and say that yes AI was used very heavily with this task.
 *The progress I used was that I first made a very primitive version on my own (now deleted) to get some experience.
 *Then with some understanding of the main points I used Ai and google to help me build a proper version of the game.
 *I then spend my time analyzing and asking about the methods used in the code as well as their applications in the C++ language, the "whys" and "hows" of it.
 * This is mostly working learning method but still, it is hard not to fall into the trap where AI carries the project so much that you end up learning nothing...
 * You really need to prompt it with "dont give me full codes, but help me learn"
 * Sigh, but such is the world we live in
 *
 * Well, through out the code there are snippets of Ai comments left in.
 * These are intented to help me understand the code I have made and hopefully get the main points across.
 * I wish to be informed if any of these comments are missleading or encourage a bad working method.
 * Or if i have at somepoint missed the point of the function and created incorrect methods in this system.
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

bool g_game_over = false;

void gameOver() {
    g_game_over = true;
}


/*==========================================================
 * CLASS: GameManager
 *==========================================================*/
class GameManager {
public:
    static const int OXYGEN_PER_MOVE = 2;
    static const int OXYGEN_DAMAGE = 5;
    static const int INIT_LIVES = 3;

private:
    Player player;
    World  world;

    vector<string> mapPlaylist;
    int            currentLevel;

public:
    GameManager() : currentLevel(0) {
        world.player = &player;
    }

    ~GameManager() {}

    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    void run() {
        splashScreen();
        if (!startup()) return;

        char input;
        while (!g_game_over) {
            input = '\0';
            if (readInput(&input) < 0) break;
            updateState(input);
            if (!g_game_over) renderScreen();
        }

        shutdown();
    }

private:
    bool startup() {
        srand((unsigned)time(nullptr));

        cout << "Enter path for Level 1 map file: ";
        string lvl1; getline(cin, lvl1);
        mapPlaylist.push_back(lvl1);

        cout << "Enter path for Level 2 map file (press Enter to skip): ";
        string lvl2; getline(cin, lvl2);
        if (!lvl2.empty()) mapPlaylist.push_back(lvl2);

        currentLevel = 0;

        if (world.loadFromFile(mapPlaylist[0]) != 0) {
            cout << "Failed to load map. Exiting.\n";
            return false;
        }

        renderScreen();
        return true;
    }

    void shutdown() {
        world.freeMap();
        printSlow("\nBYE! Welcome back soon.\n", 40);
    }

    int readInput(char* input) {
        cout << ">>> Move: w/a/s/d  |  Illuminate: i/j/k/l  |  r=reload  n=next  q=quit : ";
        try { cin >> *input; }
        catch (...) { return -1; }
        cout << "\n";
        cin.ignore();
        if (*input == 'q') return -2;
        return 0;
    }

    void updateState(char input) {
        if (g_game_over) return;

        if (input == 'r') {
            cout << "Reloading map...\n";
            int savedLives = player.lives;
            int savedScore = player.score;
            world.loadFromFile(world.mapFilePath);
            player.lives = savedLives;
            player.score = savedScore;
            return;
        }
        if (input == 'n') { nextLevel(); return; }

        if (input == 'i') { world.illuminateTile(0, -1); renderScreen(); waitEnter(); return; }
        if (input == 'k') { world.illuminateTile(0, 1); renderScreen(); waitEnter(); return; }
        if (input == 'j') { world.illuminateTile(-1, 0); renderScreen(); waitEnter(); return; }
        if (input == 'l') { world.illuminateTile(1, 0); renderScreen(); waitEnter(); return; }

        int dx = 0, dy = 0;
        if (input == 'w') dy = -1;
        else if (input == 's') dy = 1;
        else if (input == 'a') dx = -1;
        else if (input == 'd') dx = 1;
        else return;

        world.tryMovePlayer(dx, dy);

        player.reduceOxygen(OXYGEN_PER_MOVE);

        if (!player.hasOxygen()) {
            printSlow("*** OUT OF OXYGEN! Taking damage! ***\n", 40);
            player.takeDamage(OXYGEN_DAMAGE);
            if (!player.isAlive()) { handleDeath(); return; }
        }

        if (!player.isAlive()) { handleDeath(); return; }

        world.updateEnemyActivation();

        bool killedByEnemy = world.updateEnemies();
        if (killedByEnemy) { handleDeath(); return; }

        if (world.allTreasuresFound()) {
            printSlow("\n*** ALL TREASURES COLLECTED! The way forward opens... ***\n", 40);
            nextLevel();
        }
    }

    // ---------------------------------------------------------------
    // renderScreen()
    // Shows map plus all player stats including score and a summary
    // of how many collectible items remain on the level.
    // ---------------------------------------------------------------
    void renderScreen() {
        if (g_game_over) return;
        if (!world.map) { cout << "(no map loaded)\n"; return; }

        cout << "\n";
        cout << "  [Level " << (currentLevel + 1)
            << "/" << mapPlaylist.size() << "]\n";

        world.render();

        pair<int, int> tc = world.treasureCount();
        int tFound = tc.first, tTotal = tc.second;

        // Count how many collectible items are still on the map
        int itemsLeft = 0;
        for (CollectibleItem* c : world.collectibles)
            if (!c->collected) itemsLeft++;

        cout << "\n";
        cout << "  Oxygen: " << player.oxygen << "%"
            << "  |  Health: " << player.health
            << "  |  Battery: " << player.battery << "%"
            << "  |  Lives: " << player.lives
            << "  |  Score: " << player.score;

        if (tTotal > 0)
            cout << "  |  Treasure: " << tFound << "/" << tTotal;

        if (!world.collectibles.empty())
            cout << "  |  Items left: " << itemsLeft;

        cout << "\n";

        // Legend so the player knows what B/O/G mean on the map
        cout << "  [B=battery  O=oxygen  G=gold coin  $=treasure]\n\n";
    }

    void handleDeath() {
        printSlow("\n*** YOU DIED! ***\n", 60);

        world.setTile(player.x, player.y, 'o');

        bool survived = player.loseLife();

        if (!survived) {
            deathScreen();
            gameOver();
            return;
        }

        cout << "Lives remaining: " << player.lives << "  -- Respawning...\n";

        int savedLives = player.lives;
        int savedScore = player.score;
        world.loadFromFile(world.mapFilePath);
        player.lives = savedLives;
        player.score = savedScore;

        renderScreen();
    }

    void nextLevel() {
        currentLevel++;

        if (currentLevel >= (int)mapPlaylist.size()) {
            printSlow("\n", 15);
            printSlow("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", 15);
            printSlow("  ~                                                  ~\n", 15);
            printSlow("  ~         Y O U   W I N !                         ~\n", 50);
            printSlow("  ~                                                  ~\n", 15);
            printSlow("  ~   You have explored every depth of the sea.      ~\n", 30);
            printSlow("  ~   The treasure is yours, brave diver!            ~\n", 30);
            printSlow("  ~                                                  ~\n", 15);
            printSlow("  ~   Final score: " + to_string(player.score)
                + string(32 - to_string(player.score).size(), ' ') + "~\n", 30);
            printSlow("  ~                                                  ~\n", 15);
            printSlow("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n", 15);
            gameOver();
            return;
        }

        printSlow("\n--- Descending deeper... Level "
            + to_string(currentLevel + 1) + " ---\n", 40);

        player.health = 100;
        player.oxygen = 100;
        player.battery = 100;
        // score and lives carry over between levels

        if (world.loadFromFile(mapPlaylist[currentLevel]) != 0) {
            cout << "Failed to load next level. Exiting.\n";
            gameOver();
        }
    }

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
        printSlow("  ~   Final score: " + to_string(player.score)
            + string(32 - to_string(player.score).size(), ' ') + "~\n", 30);
        printSlow("  ~                                                  ~\n", 15);
        printSlow("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n", 15);
    }

    void splashScreen() {
        printSlow("\n");
        printSlow("  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", 15);
        printSlow("  ~                                                  ~\n", 15);
        printSlow("  ~    W E L C O M E   T O   H O L Y   D I V E R   ~\n", 50);
        printSlow("  ~                     v0.06                        ~\n", 25);
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
        printSlow("  B = battery pack  (recharges your flashlight)\n", 25);
        printSlow("  O = oxygen canister  (refills your oxygen supply)\n", 25);
        printSlow("  G = gold coin  (worth 100 points)\n", 25);
        printSlow("  You have " + to_string(INIT_LIVES) + " lives. Good luck.\n\n", 40);
    }

    void printSlow(const string& text, int ms = 25) {
        for (char c : text) {
            cout << c << flush;
            this_thread::sleep_for(chrono::milliseconds(ms));
        }
    }

    void waitEnter() {
        cout << "  (Press Enter to continue...)\n";
        cin.get();
    }
};


int main() {
    GameManager game;
    game.run();
    return 0;
}