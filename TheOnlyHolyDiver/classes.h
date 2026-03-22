#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <thread>
using namespace std;

// gameOver() is defined in holy_diver.cpp as a global function.
// We forward-declare it here so the Player destructor can call it.
// This is one place where a global is unavoidable with the current structure:
// the Player destructor needs to signal the game loop to stop, but the Player
// class itself does not own the loop. A GameManager class would be a cleaner
// solution (the destructor could call a method on the manager instead), but
// a forward declaration keeps things simple for now.
void gameOver();


/*==========================================================
 * CLASS: Player
 *
 * Represents the human player's state in the game world.
 * Owns: health, oxygen, battery, lives, map position,
 *       and the fog-of-war visibility grid.
 *
 * The Player does NOT know about the map layout or enemies -
 * that information belongs to World. Player only knows about
 * its own stats and which tiles it has already seen.
 *
 * TEACHING POINT - Single Responsibility Principle:
 *   Each class should do one job. Player tracks player state.
 *   World tracks the game world. Keeping these separate makes
 *   each class easier to understand, test, and change.
 *==========================================================*/
class Player {
public:
    // --- Core stats (0-100 range) ---
    // Three separate resource bars, each with different drain and replenish rules.
    // Having them separate (not one combined "energy") lets the game create
    // interesting trade-offs: you might have full health but no oxygen.
    int health;     // Falls to 0 = death. Damaged by enemies and oxygen deprivation.
    int oxygen;     // Falls 2% per move. When it hits 0, health starts draining.
    int battery;    // Falls 5% per illuminate command. Used for the flashlight.
    int lives;      // How many times the player can die before game over.

    // --- Position on the map ---
    // x = column (left/right), y = row (up/down).
    // The World class reads these to know where the player is.
    // Enemies read them too (via World) to decide which way to chase.
    int x, y;

    // --- Spawn point ---
    // Saved when the map loads. Used to teleport the player back after death.
    int spawnX, spawnY;

    // --- Fog-of-war visibility grid ---
    // A 2D array of bools that exactly mirrors the map dimensions.
    // visibility[row][col] = true  --> player has seen this tile; draw it normally.
    // visibility[row][col] = false --> tile is dark; draw it as a blank space.
    //
    // TEACHING POINT - Dynamic 2D array:
    //   We allocate this with 'new' because we don't know the map size at
    //   compile time. The map file could be any size. We store the dimensions
    //   (visRows, visCols) so we can safely free and bounds-check the array later.
    bool** visibility;
    int visRows;    // number of rows in the visibility grid (= map height)
    int visCols;    // number of columns (= map width)

    // ---------------------------------------------------------------
    // Constructor
    // Sets all stats to their starting values.
    // The visibility pointer starts as nullptr - it gets allocated later
    // by allocVisibility() once the map dimensions are known.
    // ---------------------------------------------------------------
    Player(int startX = 0, int startY = 0)
        : health(100), oxygen(100), battery(100), lives(3),
        x(startX), y(startY), spawnX(startX), spawnY(startY),
        visibility(nullptr), visRows(0), visCols(0)
    {
        // Member initializer list (the ": health(100), ..." above) is the
        // preferred C++ way to set values in a constructor. It initializes
        // members directly rather than assigning them after construction -
        // slightly more efficient and required for const or reference members.
    }

    // ---------------------------------------------------------------
    // Destructor
    // Called automatically when the Player object goes out of scope or
    // is deleted. Frees the heap memory used by the visibility grid,
    // then signals the game loop to stop via the global gameOver().
    //
    // TEACHING POINT - Destructor responsibility:
    //   Any memory you allocate with 'new' must be freed with 'delete'.
    //   The destructor is the natural place to do this cleanup.
    //   If you forget, the memory leaks - it stays reserved until the
    //   whole program exits, which is a bug in long-running programs.
    // ---------------------------------------------------------------
    ~Player() {
        freeVisibility();   // release the 2D bool array
        // Print a farewell message as required by the assignment spec.
        cout << "The diver's tale ends here. Farewell, brave soul.\n";
        // Tell the main loop we are done. gameOver() sets game_over = true.
        gameOver();
    }

    // ---------------------------------------------------------------
    // Copy constructor and copy assignment - DELETED
    //
    // TEACHING POINT - Deleted copy:
    //   There should only ever be one Player. If C++ were allowed to copy
    //   it (e.g. by accident when passing to a function by value), both
    //   copies would point to the same visibility array. When one is
    //   destroyed it would free the array; the other would then hold a
    //   dangling pointer and crash. Deleting these operations makes the
    //   compiler reject any accidental copy at compile time.
    // ---------------------------------------------------------------
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    // ---------------------------------------------------------------
    // allocVisibility(r, c)
    // Allocates a fresh rows*cols grid of bools, all set to false (dark).
    // Called by World::loadFromFile() every time a new map is loaded.
    //
    // TEACHING POINT - Dynamic 2D array allocation:
    //   A 2D array in C++ is an array of pointers, each pointing to a row.
    //   Step 1: allocate the array of row-pointers:  new bool*[r]
    //   Step 2: for each row, allocate the columns:  new bool[c]()
    //   The () at the end zero-initialises every bool to false.
    // ---------------------------------------------------------------
    void allocVisibility(int r, int c) {
        freeVisibility();           // free any existing grid first
        visRows = r;
        visCols = c;
        visibility = new bool* [r];
        for (int i = 0; i < r; i++)
            visibility[i] = new bool[c]();   // () = value-initialise to false
    }

    // ---------------------------------------------------------------
    // freeVisibility()
    // Releases the memory allocated by allocVisibility().
    // Must be called in the destructor and before allocating a new grid.
    // ---------------------------------------------------------------
    void freeVisibility() {
        if (visibility) {
            // Delete each row individually because each was separately allocated.
            // Deleting only the outer pointer would leave the row arrays leaked.
            for (int i = 0; i < visRows; i++)
                delete[] visibility[i];
            delete[] visibility;
            visibility = nullptr;   // set to nullptr to prevent a dangling pointer
        }
        visRows = visCols = 0;
    }

    // ---------------------------------------------------------------
    // setVisible(cx, cy)
    // Marks a single tile as permanently visible.
    // Once lit, a tile is never dark again (fog-of-war is one-way).
    // ---------------------------------------------------------------
    void setVisible(int cx, int cy) {
        // Bounds check: never write outside the allocated array
        if (visibility && cy >= 0 && cy < visRows && cx >= 0 && cx < visCols)
            visibility[cy][cx] = true;
    }

    // ---------------------------------------------------------------
    // isVisible(cx, cy)
    // Returns true if the tile at (cx, cy) has been seen before.
    // World::render() calls this for every tile to decide what to draw.
    // ---------------------------------------------------------------
    bool isVisible(int cx, int cy) const {
        if (!visibility || cy < 0 || cy >= visRows || cx < 0 || cx >= visCols)
            return false;
        return visibility[cy][cx];
    }

    // ---------------------------------------------------------------
    // revealAround(cx, cy, radius)
    // Illuminates all tiles within a circular area centred on (cx, cy).
    // Called after every successful move so the player always sees a
    // small patch of tiles immediately around their current position.
    //
    // The circle test: dx*dx + dy*dy <= radius*radius is the standard
    // "point inside circle" formula (Pythagoras - comparing squared distances
    // avoids a costly square root calculation).
    // ---------------------------------------------------------------
    void revealAround(int cx, int cy, int radius) {
        for (int dy = -radius; dy <= radius; dy++)
            for (int dx = -radius; dx <= radius; dx++)
                if (dx * dx + dy * dy <= radius * radius)
                    setVisible(cx + dx, cy + dy);
    }

    // ---------------------------------------------------------------
    // illuminate(cx, cy)
    // Spends 5% battery to reveal a single tile.
    // Returns false if the battery is empty.
    // The main flashlight ray logic lives in World::illuminateTile();
    // this helper is used for individual tile reveals where needed.
    // ---------------------------------------------------------------
    bool illuminate(int cx, int cy) {
        if (battery <= 0) return false;
        battery = max(0, battery - 5);
        setVisible(cx, cy);
        return true;
    }

    // ---------------------------------------------------------------
    // Stat helper methods
    // These small methods let World and the game loop modify player
    // stats without directly touching the member variables from outside.
    //
    // TEACHING POINT - Encapsulation:
    //   Adding helper methods instead of writing to members directly
    //   lets you add logic later (e.g. a "shield" that halves damage)
    //   without changing every place in the code that deals damage.
    //   max() and min() clamp values so stats never go out of range.
    // ---------------------------------------------------------------
    void setSpawn(int sx, int sy) { spawnX = sx; spawnY = sy; }
    void reduceOxygen(int amount) { oxygen = max(0, oxygen - amount); }
    void takeDamage(int amount) { health = max(0, health - amount); }
    void heal(int amount) { health = min(100, health + amount); }
    void refillOxygen(int amount) { oxygen = min(100, oxygen + amount); }
    void refillBattery(int amount) { battery = min(100, battery + amount); }

    // Query helpers - marked const because they do not modify any member
    bool isAlive()    const { return health > 0; }
    bool hasOxygen()  const { return oxygen > 0; }
    bool hasBattery() const { return battery > 0; }

    // ---------------------------------------------------------------
    // loseLife()
    // Decrements the life counter and, if lives remain, respawns the
    // player at the saved spawn point with full stats.
    // Returns false when all lives are gone (signals game over to caller).
    // ---------------------------------------------------------------
    bool loseLife() {
        lives--;
        if (lives <= 0) {
            lives = 0;
            return false;   // no lives left - caller should trigger game over
        }
        // Respawn: restore all stats and teleport back to the starting tile
        health = oxygen = battery = 100;
        x = spawnX;
        y = spawnY;
        return true;        // survived - continue playing
    }

    // ---------------------------------------------------------------
    // reset(sx, sy)
    // Full reset used when starting a brand-new game.
    // Restores every stat to 100, resets lives to 3, and places
    // the player at position (sx, sy).
    // ---------------------------------------------------------------
    void reset(int sx, int sy) {
        health = oxygen = battery = 100;
        lives = 3;
        x = sx; y = sy;
        spawnX = sx; spawnY = sy;
    }
};


/*==========================================================
 * BASE CLASS: Enemy
 *
 * Defines the shared interface and data for all enemy types.
 * Stores position, health, damage value, and two state flags:
 *   alive  - false once the enemy is destroyed (World skips dead enemies)
 *   active - false until the player sees the enemy; once active,
 *            the enemy starts acting each turn (chasing or wandering)
 *
 * TEACHING POINT - Abstract base class and polymorphism:
 *   update() is declared as "pure virtual" (= 0). This means:
 *   1. Enemy itself cannot be instantiated - you must use a subclass.
 *   2. Every subclass MUST provide its own update() implementation.
 *   3. A vector<Enemy*> can hold any mix of subclass pointers, and
 *      calling e->update() will automatically use the right version
 *      for each object. This is runtime polymorphism - the correct
 *      function is selected at runtime based on the actual object type,
 *      not the pointer type.
 *==========================================================*/
class Enemy {
public:
    int  x, y;      // grid position; World reads this to draw and check collisions
    int  health;    // enemy HP (reduced when player fights back - future feature)
    int  damage;    // HP deducted from the player on contact
    char symbol;    // character displayed on the map ('M' or 'S')
    bool alive;     // false = defeated, World will skip this enemy
    bool active;    // false = not yet spotted; true = acts each turn

protected:
    // Movement pacing: enemy only moves once every moveCooldown turns.
    // This prevents fast enemies from being impossible to dodge.
    int moveTimer;      // counts turns since the last actual move
    int moveCooldown;   // how many turns must pass between moves

public:
    Enemy(int sx = 0, int sy = 0, char sym = 'M', int dmg = 25, int cooldown = 2)
        : x(sx), y(sy), health(50), damage(dmg), symbol(sym),
        alive(true), active(false), moveTimer(0), moveCooldown(cooldown)
    {
    }

    // Virtual destructor - REQUIRED when using polymorphism with inheritance.
    //
    // TEACHING POINT - Why virtual destructor:
    //   If you delete a MovingEnemy through an Enemy* pointer, C++ needs
    //   to know to run MovingEnemy's destructor, not just Enemy's.
    //   Marking the base destructor 'virtual' ensures the correct subclass
    //   destructor is called. Without it, only the base destructor runs,
    //   which can leave subclass resources unreleased - a subtle but serious bug.
    virtual ~Enemy() {}

    // Pure virtual update() - each subclass defines its own AI behaviour.
    // Returns true if the enemy moved onto the player's tile (= contact damage).
    // Receives player position and map access so the enemy can navigate without
    // storing a pointer directly to Player or World.
    virtual bool update(int playerX, int playerY,
        char** map, int rows, int cols) = 0;

    // giveDamage() - returns this enemy's damage value.
    // Kept as a method so future subclasses could override it (e.g. enraged mode).
    int giveDamage() const { return damage; }

    // takeDamage() - reduces health; sets alive = false when HP reaches zero.
    void takeDamage(int amount) {
        health -= amount;
        if (health <= 0) { health = 0; alive = false; }
    }

    // ---------------------------------------------------------------
    // Copy constructor - DELETED
    //
    // TEACHING POINT - Copy constructor design for Enemy:
    //   The assignment asks us to design copy constructors and note whether
    //   the compiler-generated default is sufficient.
    //   For Enemy, the default shallow copy is NOT safe: if a subclass ever
    //   adds pointer members, both copies would share those pointers and
    //   the destructor would double-free them.
    //   We DELETE copying here to prevent accidental misuse. Enemies are
    //   always managed through pointers in vector<Enemy*>, so copying them
    //   is never actually needed in this program.
    // ---------------------------------------------------------------
    Enemy(const Enemy&) = delete;
    Enemy& operator=(const Enemy&) = delete;

protected:
    // canStep(nx, ny) - shared navigation helper available to all subclasses.
    // Returns true only if the tile at (nx, ny) is open floor ('o') or the
    // player tile ('P'). Walls, map edges, other enemies, and '$' tiles block movement.
    bool canStep(int nx, int ny, char** map, int rows, int cols) const {
        if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) return false;
        char t = map[ny][nx];
        // 'o' = open floor, 'P' = player tile (moving here = contact damage)
        return (t == 'o' || t == 'P');
    }
};


/*==========================================================
 * SUBCLASS: MovingEnemy  (symbol 'M')
 *
 * ACTIVE behaviour (player has seen it):
 *   Chases the player using greedy pathfinding. Moves one tile
 *   per turn along whichever axis has the bigger gap to the player,
 *   falling back to the other axis if the primary direction is blocked.
 *
 * INACTIVE behaviour (not yet spotted):
 *   Wanders randomly with a 25% chance to stay still each turn,
 *   simulating an enemy that is lurking rather than actively patrolling.
 *
 * TEACHING POINT - Subclass and 'override' keyword:
 *   'override' after the method signature tells the compiler to verify
 *   that this really does override a virtual method in the base class.
 *   A typo in the name would be caught at compile time instead of
 *   silently creating a completely new, unrelated method.
 *==========================================================*/
class MovingEnemy : public Enemy {
public:
    MovingEnemy(int sx = 0, int sy = 0, char sym = 'M', int dmg = 25)
        : Enemy(sx, sy, sym, dmg, 2)   // moveCooldown=2: moves every other turn
    {
    }

    bool update(int playerX, int playerY,
        char** map, int rows, int cols) override
    {
        if (!alive) return false;

        // Rate-limit movement: only act once every moveCooldown turns
        moveTimer++;
        if (moveTimer < moveCooldown) return false;
        moveTimer = 0;

        int newX = x, newY = y;   // default position: stay put

        if (active) {
            // --- Greedy chase algorithm ---
            // Calculate the distance to the player on each axis
            int dx = playerX - x;   // positive = player is to the right
            int dy = playerY - y;   // positive = player is below

            // Move along the axis with the larger gap to close distance faster
            bool horizFirst = (abs(dx) >= abs(dy));

            // Primary attempt: step one tile toward the player on the chosen axis
            int tryX = x + (horizFirst ? (dx > 0 ? 1 : -1) : 0);
            int tryY = y + (horizFirst ? 0 : (dy > 0 ? 1 : -1));

            if (canStep(tryX, tryY, map, rows, cols)) {
                newX = tryX; newY = tryY;
            }
            else {
                // Primary direction blocked - try the other axis as a fallback
                int altX = x + (horizFirst ? 0 : (dx > 0 ? 1 : (dx < 0 ? -1 : 0)));
                int altY = y + (horizFirst ? (dy > 0 ? 1 : (dy < 0 ? -1 : 0)) : 0);
                if (canStep(altX, altY, map, rows, cols)) {
                    newX = altX; newY = altY;
                }
                // Both directions blocked: stay in place this turn
            }
        }
        else {
            // --- Random wandering (inactive enemy) ---
            // 25% chance to skip the turn (rand()%4 == 0 is true 1 in 4 times)
            if (rand() % 4 == 0) return false;

            // Pick a random cardinal direction and try to step there
            int dirs[4][2] = { {0,-1}, {0,1}, {-1,0}, {1,0} };  // up/down/left/right
            int d = rand() % 4;
            int tx = x + dirs[d][0];
            int ty = y + dirs[d][1];
            if (canStep(tx, ty, map, rows, cols)) { newX = tx; newY = ty; }
        }

        // Did the enemy walk onto the player's tile?
        bool hitPlayer = (newX == playerX && newY == playerY);

        if (!hitPlayer) {
            // Update the map grid: erase old position, draw new position
            map[y][x] = 'o';      // old tile becomes open floor
            map[newY][newX] = symbol;   // new tile shows the enemy symbol
        }
        x = newX; y = newY;     // update stored position
        return hitPlayer;       // caller (World) applies damage if true
    }
};


/*==========================================================
 * SUBCLASS: StationaryEnemy  (symbol 'S')
 *
 * Never moves. Acts like a hazard tile - walking into it hurts.
 * Contact damage is handled by World::tryMovePlayer() when the
 * player attempts to enter the tile, so update() here is a no-op.
 *
 * TEACHING POINT - Minimal override:
 *   update() must exist because it is pure virtual in Enemy,
 *   but for a stationary enemy it intentionally does nothing.
 *   This is valid: the interface contract is satisfied, the
 *   behaviour is deliberately empty. The constructor passes a
 *   huge cooldown (999) as extra insurance that nothing moves.
 *==========================================================*/
class StationaryEnemy : public Enemy {
public:
    StationaryEnemy(int sx = 0, int sy = 0, char sym = 'S', int dmg = 15)
        : Enemy(sx, sy, sym, dmg, 999)  // cooldown=999: will never actually move
    {
    }

    // Stationary enemies never act; update is intentionally empty
    bool update(int, int, char**, int, int) override { return false; }
};


/*==========================================================
 * CLASS: Item
 *
 * A collectible object placed on the map.
 * When the player steps onto an item tile, applyTo() is called,
 * which restores the relevant player stat and marks the item collected.
 *
 * Currently TREASURE is used for the win condition.
 * OXYGEN, HEALTH, BATTERY, KEY are defined ready for future expansion.
 *==========================================================*/
class Item {
public:
    // Enum defines all possible item types as named constants.
    // Using an enum (instead of raw integers like 0, 1, 2...) makes the
    // code self-documenting and prevents accidentally passing an invalid number.
    enum Type { TREASURE, OXYGEN, HEALTH, BATTERY, KEY };

    int  x, y;          // position on the map grid
    Type type;          // what kind of item this is
    int  value;         // how much stat to restore (0 for TREASURE - score only)
    bool collected;     // true once the player has picked it up
    char symbol;        // character shown on the map ('$')

    // Constructor - initialises all fields via member initializer list
    Item(int sx = 0, int sy = 0, Type t = TREASURE, int val = 10, char sym = '$')
        : x(sx), y(sy), type(t), value(val), collected(false), symbol(sym)
    {
    }

    ~Item() {}

    // ---------------------------------------------------------------
    // Copy constructor - explicitly defined as required by the assignment.
    //
    // TEACHING POINT - Copy constructor for Item:
    //   The assignment asks us to design copy constructors and note whether
    //   the compiler-generated default is sufficient.
    //   For Item, the default would actually work fine: all members are
    //   simple value types (int, bool, char, enum) with no heap memory or
    //   pointers. The compiler would copy them correctly on its own.
    //   We write it explicitly here to demonstrate understanding:
    //   a copy constructor takes a const reference to the same type and
    //   manually copies each member to the new object being constructed.
    //   The parameter is (const Item& other) - a reference, not a value,
    //   to avoid infinite recursion (copying to call the copy constructor).
    // ---------------------------------------------------------------
    Item(const Item& other)
        : x(other.x), y(other.y), type(other.type), value(other.value),
        collected(other.collected), symbol(other.symbol)
    {
    }

    // ---------------------------------------------------------------
    // applyTo(player)
    // Applies this item's effect to the player and marks it collected.
    // A collected item has no further effect if the player walks over it again.
    // ---------------------------------------------------------------
    void applyTo(Player& player) {
        if (collected) return;
        switch (type) {
        case OXYGEN:   player.refillOxygen(value);  break;
        case HEALTH:   player.heal(value);           break;
        case BATTERY:  player.refillBattery(value);  break;
        case TREASURE: break;  // treasure has no stat effect - only the win condition
        case KEY:      break;  // reserved for future use
        }
        collected = true;
    }
};


/*==========================================================
 * CLASS: World
 *
 * The central authority on everything in the game world.
 * Owns the map grid, all enemies, and all items.
 * Holds a non-owning pointer to the single Player object
 * (which is created and owned by main / GameManager).
 *
 * TEACHING POINT - Composition and ownership:
 *   "The player and all enemies naturally belong to the world"
 *   (assignment spec). World OWNS Enemy objects: it creates them
 *   (new MovingEnemy(...)) and deletes them in its destructor.
 *   This is a composition relationship - enemies cannot exist
 *   independently of the World.
 *   World holds a POINTER to Player but does NOT own it.
 *   Player is created outside World and just shared with it.
 *   This distinction matters for the destructor: World must NOT
 *   delete player because it did not allocate it.
 *
 * TEACHING POINT - World as movement authority:
 *   The spec says World "controls movement possibilities".
 *   Nothing moves itself directly on the map. Player and Enemy
 *   both go through World to check and execute moves. This keeps
 *   the map grid consistent: only one place ever writes to a tile.
 *==========================================================*/
class World {
public:
    char** map;         // 2D character array loaded from the .map file
    int    rows;        // map height in tiles
    int    cols;        // map width in tiles
    string mapFilePath; // stored so 'r' reload can reopen the same file

    // When true, the next render() call draws each row with a short delay
    // for a cinematic scan-line effect. Set on every load; cleared after one use.
    bool animateNextRender;

    // Non-owning pointer to the player. World uses it to check position and
    // apply damage, but does NOT delete it (Player is owned by GameManager/main).
    Player* player;

    // Owning collections. World allocates these objects and is responsible
    // for freeing them in its destructor.
    vector<Enemy*> enemies;   // all enemies on the current level
    vector<Item>   items;     // all collectible items on the current level

    // Constructor: initialise all pointers to null/zero so they are safe
    // to dereference-check before the first map is loaded.
    World()
        : map(nullptr), rows(0), cols(0),
        animateNextRender(false), player(nullptr)
    {
    }

    // ---------------------------------------------------------------
    // Destructor
    // Frees the map grid and all heap-allocated Enemy objects.
    //
    // TEACHING POINT - Destructor and ownership:
    //   We iterate over the enemies vector and 'delete' each pointer because
    //   World created them with 'new'. The vector itself (a stack object)
    //   is destroyed automatically when World goes out of scope, but the
    //   Enemy objects on the heap are not - we must free them manually here.
    //   Player is intentionally NOT deleted: World does not own it.
    // ---------------------------------------------------------------
    ~World() {
        freeMap();
        for (Enemy* e : enemies) delete e;
        enemies.clear();
    }

    // Copying a World is disabled: the map and enemy pointers make a safe
    // shallow copy impossible, and a deep copy is never needed.
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    // ---------------------------------------------------------------
    // Tile access helpers
    // ---------------------------------------------------------------

    // getTile(x, y) - returns the char at column x, row y.
    // Returns 'x' (wall) for out-of-bounds so callers treat the map
    // edge as a wall without needing a separate bounds check.
    char getTile(int x, int y) const {
        if (x < 0 || x >= cols || y < 0 || y >= rows) return 'x';
        return map[y][x];
    }

    // setTile(x, y, c) - writes a character to the map grid.
    // All map writes go through here to keep bounds-checking in one place.
    void setTile(int x, int y, char c) {
        if (x >= 0 && x < cols && y >= 0 && y < rows) map[y][x] = c;
    }

    bool isWalkable(int x, int y) const { return getTile(x, y) != 'x'; }

    // ---------------------------------------------------------------
    // Treasure tracking
    // ---------------------------------------------------------------

    // treasureCount() - counts collected and total treasure items.
    // Returns pair<int,int>: first = found so far, second = total on level.
    // Using pair<int,int> (C++14 compatible) instead of structured bindings
    // which require C++17 (and caused the C2429 compiler errors).
    pair<int, int> treasureCount() const {
        int total = 0, found = 0;
        for (const Item& it : items) {
            if (it.type == Item::TREASURE) {
                total++;
                if (it.collected) found++;
            }
        }
        return pair<int, int>(found, total);
    }

    // allTreasuresFound() - true when every treasure on this level is collected.
    // Only returns true when total > 0, so levels without '$' tiles are unaffected.
    bool allTreasuresFound() const {
        pair<int, int> tc = treasureCount();
        int found = tc.first, total = tc.second;
        return total > 0 && found == total;
    }

    // ---------------------------------------------------------------
    // loadFromFile(filepath)
    // Reads a .map text file, builds the 2D char grid, and scans every
    // tile to create the correct game objects for special characters:
    //   'P' -> set as player spawn point
    //   'M' -> create MovingEnemy on the heap
    //   'S' -> create StationaryEnemy on the heap
    //   '$' -> create Treasure Item in the items vector
    //
    // TEACHING POINT - Dynamic memory and file I/O:
    //   We use malloc/free (C style) for the map array to match the style
    //   from earlier weeks. new/delete would also be valid C++ style.
    //   The map dimensions are not known until the file is read, so a
    //   fixed-size array cannot be used - this is why dynamic allocation
    //   is needed. We read into a vector<string> first (easy to work with),
    //   then copy into the 2D char array (the actual game grid).
    // ---------------------------------------------------------------
    int loadFromFile(const string& filepath) {
        ifstream file(filepath);
        if (!file.is_open()) {
            cout << "Error: Cannot open " << filepath << endl;
            return -1;
        }

        // Read every non-empty line into a temporary string buffer
        vector<string> lines;
        string line;
        while (getline(file, line))
            if (!line.empty()) lines.push_back(line);
        file.close();

        if (lines.empty()) { cout << "Error: Empty map." << endl; return -1; }

        // Free previous map and game objects before allocating new ones
        freeMap();
        for (Enemy* e : enemies) delete e;
        enemies.clear();
        items.clear();

        mapFilePath = filepath;
        rows = (int)lines.size();
        cols = (int)lines[0].size();

        // Allocate the 2D map array row by row
        map = (char**)malloc(rows * sizeof(char*));
        for (int i = 0; i < rows; i++) {
            map[i] = (char*)malloc((cols + 1) * sizeof(char));  // +1 for null terminator
            for (int j = 0; j < cols; j++)
                map[i][j] = (j < (int)lines[i].size()) ? lines[i][j] : 'x';
            map[i][cols] = '\0';
        }

        // Scan every tile and create game objects for special characters
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                char c = map[i][j];
                if (c == 'P' && player) {
                    player->x = j; player->y = i;
                    player->setSpawn(j, i);
                }
                else if (c == 'M') {
                    // 'new' allocates on the heap; the pointer is stored in
                    // the vector so we can delete it later in the destructor
                    enemies.push_back(new MovingEnemy(j, i, 'M', 25));
                }
                else if (c == 'S') {
                    enemies.push_back(new StationaryEnemy(j, i, 'S', 15));
                }
                else if (c == '$') {
                    // Items are stored by value in the vector (no new/delete needed)
                    items.push_back(Item(j, i, Item::TREASURE, 0, '$'));
                }
            }
        }

        // Give the player a fresh visibility grid and reveal the spawn area
        if (player) {
            player->allocVisibility(rows, cols);
            player->revealAround(player->x, player->y, 3);
        }

        // Flag: next render() call should play the row-by-row animation
        animateNextRender = true;

        if (!enemies.empty())
            cout << "[" << enemies.size() << " enemy/enemies on this level]" << endl;

        pair<int, int> tc = treasureCount();
        if (tc.second > 0)
            cout << "[" << tc.second << " treasure(s) hidden - find them all to advance!]" << endl;

        return 0;
    }

    // ---------------------------------------------------------------
    // tryMovePlayer(dx, dy)
    // The single authority on whether the player can move one tile.
    // dx/dy are each -1, 0, or +1 indicating the intended direction.
    //
    // Rules enforced here (World controls all movement):
    //   1. Cannot leave the map boundary.
    //   2. Cannot enter a wall tile ('x').
    //   3. Moving into an UNSEEN tile that has a hidden enemy triggers
    //      an ambush: the enemy gets a free hit and the player is blocked.
    //   4. Moving into a visible tile occupied by a stationary enemy hurts.
    //   5. Stepping onto a treasure tile collects it automatically.
    //
    // Returns true if the player actually moved; false if blocked.
    // ---------------------------------------------------------------
    bool tryMovePlayer(int dx, int dy) {
        if (!player) return false;

        int nx = player->x + dx;    // candidate new column
        int ny = player->y + dy;    // candidate new row

        // Rule 1: boundary check
        if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) {
            cout << "Can't move there!" << endl;
            return false;
        }

        char target = map[ny][nx];
        bool dark = !player->isVisible(nx, ny);

        // Rule 2: walls block movement in all conditions
        if (target == 'x') {
            cout << "Blocked by obstacle!" << endl;
            return false;
        }

        // Rule 3: moving into a DARK tile
        if (dark) {
            Enemy* lurker = enemyAt(nx, ny);
            if (lurker && lurker->alive) {
                // Hidden enemy strikes first - player is knocked back
                player->takeDamage(lurker->giveDamage());
                lurker->active = true;  // enemy is now awake and will start chasing
                cout << "*** Something lurks in the dark and strikes! -"
                    << lurker->giveDamage() << " health ***" << endl;
                player->setVisible(nx, ny);  // the encounter reveals the tile
                return false;                // player did NOT enter the tile
            }
            // No enemy here - player steps into darkness and reveals the tile
            player->setVisible(nx, ny);
        }

        // Rule 4: visible stationary enemy blocks entry and damages the player
        Enemy* standing = enemyAt(nx, ny);
        if (standing && standing->alive) {
            player->takeDamage(standing->giveDamage());
            cout << "*** Ouch! You walked into something sharp! -"
                << standing->giveDamage() << " health ***" << endl;
            return false;
        }

        // Move is valid: update the map grid and the player's stored position
        setTile(player->x, player->y, 'o');  // vacate the old tile (becomes open floor)
        player->x = nx;
        player->y = ny;
        setTile(nx, ny, 'P');                // mark the new tile as the player's position

        // Reveal a small circle of tiles around the new position
        player->revealAround(nx, ny, 3);

        // Rule 5: collect any treasure on the destination tile
        for (Item& item : items) {
            if (!item.collected && item.x == nx && item.y == ny
                && item.type == Item::TREASURE) {
                item.applyTo(*player);   // sets collected = true
                pair<int, int> tc = treasureCount();
                cout << "*** You found a treasure! ("
                    << tc.first << "/" << tc.second << " collected) ***" << endl;
                break;  // only one item can occupy a tile
            }
        }

        return true;
    }

    // ---------------------------------------------------------------
    // illuminateTile(dx, dy)
    // Fires a ray of light from the player in direction (dx, dy).
    // Every tile along the ray is permanently revealed. The ray stops
    // when it hits a wall ('x') or the map edge.
    // Costs 5% battery per keypress (deducted once, not per tile).
    //
    // This is the flashlight mechanic: the player can scan a corridor
    // before entering it. Safer than moving blind, but drains battery.
    // ---------------------------------------------------------------
    bool illuminateTile(int dx, int dy) {
        if (!player) return false;
        if (dx == 0 && dy == 0) return false;   // must provide a direction

        if (!player->hasBattery()) {
            cout << "Battery empty! Cannot illuminate." << endl;
            return false;
        }

        // Deduct battery cost once per keypress regardless of ray length
        player->battery = max(0, player->battery - 5);

        bool foundEnemy = false;
        bool foundWall = false;
        bool foundTreasure = false;
        int  tilesRevealed = 0;

        // Walk the ray tile by tile, starting adjacent to the player
        int tx = player->x + dx;
        int ty = player->y + dy;

        while (tx >= 0 && tx < cols && ty >= 0 && ty < rows) {
            bool wasNew = !player->isVisible(tx, ty);  // was this tile dark before?
            player->setVisible(tx, ty);                 // permanently reveal it
            tilesRevealed++;

            char t = map[ty][tx];

            // If the beam reveals an enemy for the first time, wake it up
            if (wasNew && (t == 'M' || t == 'S')) {
                for (Enemy* e : enemies) {
                    if (e->alive && !e->active && e->x == tx && e->y == ty) {
                        e->active = true;
                        foundEnemy = true;
                        cout << "Your light catches something - an enemy wakes up!" << endl;
                    }
                }
            }

            // Note if the beam reveals a treasure tile for the first time
            if (wasNew && t == '$') foundTreasure = true;

            // Walls absorb the beam: reveal the wall tile then stop the ray
            if (t == 'x') { foundWall = true; break; }

            tx += dx;
            ty += dy;
        }

        // Report what the flashlight found to the player
        if (tilesRevealed == 0)  cout << "Nothing to illuminate there." << endl;
        else if (foundEnemy)          cout << "Illuminated: ENEMY spotted in the darkness!" << endl;
        else if (foundTreasure)       cout << "Illuminated: you spot a glint of TREASURE!" << endl;
        else if (foundWall)           cout << "Illuminated: corridor ends at a wall." << endl;
        else                          cout << "Illuminated: passage stretches into the dark." << endl;

        return true;
    }

    // ---------------------------------------------------------------
    // updateEnemyActivation()
    // Scans all enemies and activates any that are now inside the player's
    // visible area. Called after every player move.
    //
    // TEACHING POINT - Separation of concerns:
    //   World controls the visibility array (what the player can see).
    //   It therefore also decides which enemies are "spotted". Neither
    //   Player nor Enemy needs to know the other's position directly -
    //   World is the intermediary that holds that knowledge.
    // ---------------------------------------------------------------
    void updateEnemyActivation() {
        if (!player) return;
        for (Enemy* e : enemies)
            if (e->alive && !e->active && player->isVisible(e->x, e->y))
                e->active = true;
    }

    // ---------------------------------------------------------------
    // updateEnemies()
    // Calls update() on every living enemy once per player turn.
    // Returns true if any enemy reached the player's tile (caller handles death).
    //
    // TEACHING POINT - Polymorphic dispatch:
    //   e->update() is a virtual call. Even though 'e' is declared as
    //   Enemy*, the correct version (MovingEnemy::update or
    //   StationaryEnemy::update) is called automatically at runtime
    //   based on the actual type of each object. This is the core benefit
    //   of polymorphism: the loop here doesn't need to know or care which
    //   type each enemy is.
    // ---------------------------------------------------------------
    bool updateEnemies() {
        if (!player) return false;
        for (Enemy* e : enemies) {
            if (!e->alive) continue;
            bool hit = e->update(player->x, player->y, map, rows, cols);
            if (hit) {
                player->takeDamage(e->giveDamage());
                cout << "*** Enemy strikes! -" << e->giveDamage() << " health ***" << endl;
                if (!player->isAlive()) return true;
            }
        }
        return false;
    }

    // ---------------------------------------------------------------
    // render()
    // Draws the map to the console with fog-of-war applied.
    // Dark tiles (not yet seen) are printed as a space character ' '.
    // Visible tiles show their actual map character (wall, floor, enemy, etc.)
    // On the first render after a load, rows appear one at a time with an
    // 18ms delay for a cinematic scan-line visual effect.
    // ---------------------------------------------------------------
    void render() {
        if (!map) return;

        bool animate = animateNextRender;
        animateNextRender = false;  // consume the flag - animation plays only once

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                // Fog of war: unseen tiles appear as blank space
                if (player && !player->isVisible(j, i))
                    cout << ' ';
                else
                    cout << map[i][j];
            }
            cout << '\n' << flush;

            // Animated load: small delay between rows for visual effect
            if (animate)
                this_thread::sleep_for(chrono::milliseconds(18));
        }
    }

    // freeMap() - releases the heap-allocated map grid.
    // Called in the destructor and at the start of loadFromFile().
    void freeMap() {
        if (map) {
            for (int i = 0; i < rows; i++) free(map[i]);  // free each row array
            free(map);                                      // free the row-pointer array
            map = nullptr;
        }
        rows = cols = 0;
    }

private:
    // enemyAt(ex, ey) - internal helper used only within World.
    // Returns the first living enemy at grid position (ex, ey), or nullptr.
    // Used by tryMovePlayer() to detect if a destination tile is occupied.
    Enemy* enemyAt(int ex, int ey) {
        for (Enemy* e : enemies)
            if (e->alive && e->x == ex && e->y == ey) return e;
        return nullptr;
    }
};