#pragma once
#include <string>
#include <iostream>
using namespace std;

// Forward declaration of global game over function
// Defined in holy_diver.cpp - signals the main loop to exit
void gameOver();


/*
 * CLASS: Player
 * Represents the player character in the game.
 */
class Player {
public:
    int health;
    int oxygen;
    int lives;
    int x;      // column position on map
    int y;      // row position on map

    // Constructor - initializes player with full stats at given position
    Player(int startX = 0, int startY = 0)
        : health(100), oxygen(100), lives(3), x(startX), y(startY) {
        // TODO: allocate any dynamic resources the player might need in the future
    }

    // Destructor - called when player object is destroyed (e.g. at program end or game over)
    ~Player() {
        cout << endl << "Your adventure ends here, brave diver. Farewell..." << endl;
        gameOver(); // signal main loop to exit
    }

    // No copy constructor needed for Player - there is only ever one player instance.
    // Deleted to prevent accidental copies.
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    // Methods
    void move(int newX, int newY) {
        x = newX;
        y = newY;
    }

    void reduceOxygen(int amount) {
        oxygen -= amount;
        if (oxygen < 0) oxygen = 0;
    }

    void takeDamage(int amount) {
        health -= amount;
        if (health < 0) health = 0;
    }

    void heal(int amount) {
        health += amount;
        if (health > 100) health = 100;
    }

    void refillOxygen(int amount) {
        oxygen += amount;
        if (oxygen > 100) oxygen = 100;
    }

    bool isAlive() { return health > 0 && lives > 0; }
    bool hasOxygen() { return oxygen > 0; }

    void reset(int startX, int startY) {
        health = 100;
        oxygen = 100;
        lives = 3;
        x = startX;
        y = startY;
    }
};


/*
 * CLASS: Enemy
 * Represents an enemy on the map.
 */
class Enemy {
public:
    int x;
    int y;
    int health;
    char symbol;    // character used to represent enemy on map (e.g. 'M')
    bool alive;

    // Constructor
    Enemy(int startX = 0, int startY = 0, char sym = 'M')
        : x(startX), y(startY), health(50), symbol(sym), alive(true) {
        // TODO: allocate enemy-specific resources if needed (e.g. loot table, patrol path)
    }

    // Destructor
    ~Enemy() {
        // TODO: free any dynamically allocated enemy resources (e.g. loot table, patrol path)
    }

    // Copy constructor - explicitly defined but equivalent to compiler default.
    // Enemy only contains simple value types (int, char, bool), no dynamic memory.
    // Useful for spawning multiple enemies from one template instance.
    Enemy(const Enemy& other)
        : x(other.x), y(other.y), health(other.health),
        symbol(other.symbol), alive(other.alive) {
    }

    void move(int newX, int newY) {
        x = newX;
        y = newY;
    }

    void takeDamage(int amount) {
        health -= amount;
        if (health <= 0) {
            health = 0;
            alive = false;
        }
    }

    void update() {
        // TODO: implement enemy AI / movement logic
    }
};


/*
 * CLASS: Item
 * Represents a collectible item on the map (treasure, oxygen tank, etc.)
 */
class Item {
public:
    enum Type { TREASURE, OXYGEN, HEALTH, KEY };

    int x;
    int y;
    Type type;
    int value;      // effect value (e.g. how much oxygen it restores)
    bool collected;
    char symbol;    // character used on map

    // Constructor
    Item(int startX = 0, int startY = 0, Type t = TREASURE, int val = 10, char sym = '$')
        : x(startX), y(startY), type(t), value(val), collected(false), symbol(sym) {
        // TODO: allocate item-specific resources if needed
    }

    // Destructor
    ~Item() {
        // TODO: free any dynamically allocated item resources
    }

    // Copy constructor - explicitly defined but equivalent to compiler default.
    // Item only contains simple value types and an enum, no dynamic memory.
    // Useful for spawning multiple items of the same type from one template.
    Item(const Item& other)
        : x(other.x), y(other.y), type(other.type), value(other.value),
        collected(other.collected), symbol(other.symbol) {
    }

    void applyTo(Player& player) {
        if (collected) return;
        switch (type) {
        case OXYGEN:   player.refillOxygen(value); break;
        case HEALTH:   player.heal(value);         break;
        case TREASURE: /* TODO: add score system */  break;
        case KEY:      /* TODO: unlock doors etc */  break;
        }
        collected = true;
    }
};


/*
 * CLASS: World
 * Represents the game world / map and manages its state.
 */
class World {
public:
    char** map;
    int rows;
    int cols;
    string mapFilePath;

    // Constructor
    World() : map(nullptr), rows(0), cols(0) {
        // TODO: allocate collectable list, enemy list, etc.
    }

    // Destructor - automatically frees map memory
    ~World() {
        freeMap();
        // TODO: free collectable list, enemy list, etc.
    }

    // Copy constructor - deleted. World owns dynamic memory (char** map),
    // shallow copy would cause double-free bugs. Only one world exists at a time.
    // Use loadFromFile() to get a fresh state instead.
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void freeMap() {
        if (map != nullptr) {
            for (int i = 0; i < rows; i++) {
                free(map[i]);
            }
            free(map);
            map = nullptr;
        }
        rows = 0;
        cols = 0;
    }

    char getTile(int x, int y) {
        if (x < 0 || x >= cols || y < 0 || y >= rows) return 'x';
        return map[y][x];
    }

    void setTile(int x, int y, char c) {
        if (x < 0 || x >= cols || y < 0 || y >= rows) return;
        map[y][x] = c;
    }

    bool isWalkable(int x, int y) {
        return getTile(x, y) != 'x';
    }

    int loadFromFile(string filepath) {
        // TODO: move load_level() logic here
        mapFilePath = filepath;
        return 0;
    }

    void render() {
        if (!map) return;
        for (int i = 0; i < rows; i++) {
            printf("%s\n", map[i]);
        }
    }
};