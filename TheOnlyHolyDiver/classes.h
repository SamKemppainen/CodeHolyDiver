#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cmath>
using namespace std;

// Forward declaration - defined in holy_diver.cpp
void gameOver();


/*==========================================================
 * CLASS: Player
 * Owns: health, oxygen, battery, lives, position, visibility map.
 * The visibility array tracks which tiles have been illuminated.
 *==========================================================*/
class Player {
public:
    int health;
    int oxygen;
    int battery;    // 0-100%, consumed when illuminating tiles
    int lives;

    int x;          // column position on map
    int y;          // row position on map
    int spawnX;
    int spawnY;

    // Fog-of-war: visibility[row][col] = true means tile has been seen
    bool** visibility;
    int visRows;
    int visCols;

    // ---- Constructor ----
    Player(int startX = 0, int startY = 0)
        : health(100), oxygen(100), battery(100), lives(3),
        x(startX), y(startY), spawnX(startX), spawnY(startY),
        visibility(nullptr), visRows(0), visCols(0) {
    }

    // ---- Destructor ----
    ~Player() {
        freeVisibility();
    }

    // No copies - only one player exists
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    // ---- Visibility array management ----
    void allocVisibility(int r, int c) {
        freeVisibility();
        visRows = r; visCols = c;
        visibility = new bool* [r];
        for (int i = 0; i < r; i++)
            visibility[i] = new bool[c]();  // all false (dark)
    }

    void freeVisibility() {
        if (visibility) {
            for (int i = 0; i < visRows; i++) delete[] visibility[i];
            delete[] visibility;
            visibility = nullptr;
        }
        visRows = visCols = 0;
    }

    void setVisible(int cx, int cy) {
        if (visibility && cy >= 0 && cy < visRows && cx >= 0 && cx < visCols)
            visibility[cy][cx] = true;
    }

    bool isVisible(int cx, int cy) const {
        if (!visibility || cy < 0 || cy >= visRows || cx < 0 || cx >= visCols)
            return false;
        return visibility[cy][cx];
    }

    // Reveal all tiles within a circular radius around (cx, cy).
    // Called every move so the player always sees nearby tiles.
    void revealAround(int cx, int cy, int radius) {
        for (int dy = -radius; dy <= radius; dy++)
            for (int dx = -radius; dx <= radius; dx++)
                if (dx * dx + dy * dy <= radius * radius)
                    setVisible(cx + dx, cy + dy);
    }

    // Illuminate an adjacent tile - costs 5% battery. Returns false if no battery.
    bool illuminate(int cx, int cy) {
        if (battery <= 0) return false;
        battery = max(0, battery - 5);
        setVisible(cx, cy);
        return true;
    }

    // ---- Stat helpers ----
    void setSpawn(int sx, int sy) { spawnX = sx; spawnY = sy; }
    void reduceOxygen(int amount) { oxygen = max(0, oxygen - amount); }
    void takeDamage(int amount) { health = max(0, health - amount); }
    void heal(int amount) { health = min(100, health + amount); }
    void refillOxygen(int amount) { oxygen = min(100, oxygen + amount); }
    void refillBattery(int amount) { battery = min(100, battery + amount); }

    bool isAlive()    const { return health > 0; }
    bool hasOxygen()  const { return oxygen > 0; }
    bool hasBattery() const { return battery > 0; }

    // Lose a life and respawn. Returns false when truly dead.
    bool loseLife() {
        lives--;
        if (lives <= 0) { lives = 0; return false; }
        health = oxygen = battery = 100;
        x = spawnX; y = spawnY;
        return true;
    }

    void reset(int sx, int sy) {
        health = oxygen = battery = 100;
        lives = 3;
        x = sx; y = sy;
        spawnX = sx; spawnY = sy;
    }
};


/*==========================================================
 * BASE CLASS: Enemy
 * Subclasses override update() for different AI behaviours.
 *==========================================================*/
class Enemy {
public:
    int  x, y;
    int  health;
    int  damage;
    char symbol;
    bool alive;
    bool active;    // true once the player has seen this enemy

protected:
    int moveTimer;
    int moveCooldown;

public:
    Enemy(int sx = 0, int sy = 0, char sym = 'M', int dmg = 25, int cooldown = 2)
        : x(sx), y(sy), health(50), damage(dmg), symbol(sym),
        alive(true), active(false), moveTimer(0), moveCooldown(cooldown) {
    }

    virtual ~Enemy() {}

    // Called once per player turn. Returns true if enemy ended up on player tile.
    virtual bool update(int playerX, int playerY,
        char** map, int rows, int cols) = 0;

    int giveDamage() const { return damage; }

    void takeDamage(int amount) {
        health -= amount;
        if (health <= 0) { health = 0; alive = false; }
    }

protected:
    bool canStep(int nx, int ny, char** map, int rows, int cols) const {
        if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) return false;
        char t = map[ny][nx];
        return (t == 'o' || t == 'P');
    }
};


/*==========================================================
 * SUBCLASS: MovingEnemy  ('M' on map)
 * Chases the player when active; wanders randomly when inactive.
 *==========================================================*/
class MovingEnemy : public Enemy {
public:
    MovingEnemy(int sx = 0, int sy = 0, char sym = 'M', int dmg = 25)
        : Enemy(sx, sy, sym, dmg, 2) {
    }

    bool update(int playerX, int playerY,
        char** map, int rows, int cols) override {
        if (!alive) return false;
        moveTimer++;
        if (moveTimer < moveCooldown) return false;
        moveTimer = 0;

        int newX = x, newY = y;

        if (active) {
            // Greedy chase toward player
            int dx = playerX - x;
            int dy = playerY - y;
            bool horizFirst = (abs(dx) >= abs(dy));

            int tryX = x + (horizFirst ? (dx > 0 ? 1 : -1) : 0);
            int tryY = y + (horizFirst ? 0 : (dy > 0 ? 1 : -1));

            if (canStep(tryX, tryY, map, rows, cols)) {
                newX = tryX; newY = tryY;
            }
            else {
                int altX = x + (horizFirst ? 0 : (dx > 0 ? 1 : (dx < 0 ? -1 : 0)));
                int altY = y + (horizFirst ? (dy > 0 ? 1 : (dy < 0 ? -1 : 0)) : 0);
                if (canStep(altX, altY, map, rows, cols)) {
                    newX = altX; newY = altY;
                }
            }
        }
        else {
            // Inactive: random wander, 25% chance to skip
            if (rand() % 4 == 0) return false;
            int dirs[4][2] = { {0,-1},{0,1},{-1,0},{1,0} };
            int d = rand() % 4;
            int tx = x + dirs[d][0], ty = y + dirs[d][1];
            if (canStep(tx, ty, map, rows, cols)) { newX = tx; newY = ty; }
        }

        bool hitPlayer = (newX == playerX && newY == playerY);
        if (!hitPlayer) {
            map[y][x] = 'o';
            map[newY][newX] = symbol;
        }
        x = newX; y = newY;
        return hitPlayer;
    }
};


/*==========================================================
 * SUBCLASS: StationaryEnemy  ('S' on map)
 * Never moves. Deals damage only when the player walks into it.
 * (Contact is detected by World::tryMovePlayer, not by update().)
 *==========================================================*/
class StationaryEnemy : public Enemy {
public:
    StationaryEnemy(int sx = 0, int sy = 0, char sym = 'S', int dmg = 15)
        : Enemy(sx, sy, sym, dmg, 999) {
    }

    // Stationary enemies don't move - update is intentionally empty
    bool update(int, int, char**, int, int) override { return false; }
};


/*==========================================================
 * CLASS: Item
 *==========================================================*/
class Item {
public:
    enum Type { TREASURE, OXYGEN, HEALTH, BATTERY, KEY };

    int x, y;
    Type type;
    int  value;
    bool collected;
    char symbol;

    Item(int sx = 0, int sy = 0, Type t = TREASURE, int val = 10, char sym = '$')
        : x(sx), y(sy), type(t), value(val), collected(false), symbol(sym) {
    }

    ~Item() {}

    Item(const Item& o)
        : x(o.x), y(o.y), type(o.type), value(o.value),
        collected(o.collected), symbol(o.symbol) {
    }

    void applyTo(Player& player) {
        if (collected) return;
        switch (type) {
        case OXYGEN:   player.refillOxygen(value);  break;
        case HEALTH:   player.heal(value);           break;
        case BATTERY:  player.refillBattery(value);  break;
        case TREASURE:                               break;
        case KEY:                                    break;
        }
        collected = true;
    }
};


/*==========================================================
 * CLASS: World
 * Owns the map grid, all Enemy objects, and Items.
 * Holds a non-owning pointer to the single Player.
 * Authoritative source for movement validation, collision,
 * fog-of-war updates, and enemy activation.
 *==========================================================*/
class World {
public:
    char** map;
    int     rows;
    int     cols;
    string  mapFilePath;

    Player* player;   // non-owning - Player lives in holy_diver.cpp
    vector<Enemy*> enemies;  // owning - World creates and deletes these
    vector<Item>   items;

    World() : map(nullptr), rows(0), cols(0), player(nullptr) {}

    ~World() {
        freeMap();
        for (Enemy* e : enemies) delete e;
        enemies.clear();
    }

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    // ---- Tile access ----
    char getTile(int x, int y) const {
        if (x < 0 || x >= cols || y < 0 || y >= rows) return 'x';
        return map[y][x];
    }
    void setTile(int x, int y, char c) {
        if (x >= 0 && x < cols && y >= 0 && y < rows) map[y][x] = c;
    }
    bool isWalkable(int x, int y) const { return getTile(x, y) != 'x'; }

    // ---- Load map from file ----
    // Recognises: P (player spawn), M (moving enemy), S (stationary enemy)
    int loadFromFile(const string& filepath) {
        ifstream file(filepath);
        if (!file.is_open()) {
            cout << "Error: Cannot open " << filepath << endl;
            return -1;
        }
        vector<string> lines;
        string line;
        while (getline(file, line))
            if (!line.empty()) lines.push_back(line);
        file.close();

        if (lines.empty()) { cout << "Error: Empty map." << endl; return -1; }

        freeMap();
        for (Enemy* e : enemies) delete e;
        enemies.clear();
        items.clear();

        mapFilePath = filepath;
        rows = (int)lines.size();
        cols = (int)lines[0].size();

        map = (char**)malloc(rows * sizeof(char*));
        for (int i = 0; i < rows; i++) {
            map[i] = (char*)malloc((cols + 1) * sizeof(char));
            for (int j = 0; j < cols; j++)
                map[i][j] = (j < (int)lines[i].size()) ? lines[i][j] : 'x';
            map[i][cols] = '\0';
        }

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                char c = map[i][j];
                if (c == 'P' && player) {
                    player->x = j; player->y = i;
                    player->setSpawn(j, i);
                }
                else if (c == 'M') {
                    enemies.push_back(new MovingEnemy(j, i, 'M', 25));
                }
                else if (c == 'S') {
                    enemies.push_back(new StationaryEnemy(j, i, 'S', 15));
                }
            }
        }

        if (player) {
            player->allocVisibility(rows, cols);
            player->revealAround(player->x, player->y, 3);  // light up starting area
        }

        if (!enemies.empty())
            cout << "[" << enemies.size() << " enemy/enemies on this level]" << endl;

        return 0;
    }

    // ---- Try to move player by (dx, dy) ----
    // Returns false if blocked. Handles dark-tile encounter logic.
    bool tryMovePlayer(int dx, int dy) {
        if (!player) return false;
        int nx = player->x + dx;
        int ny = player->y + dy;

        if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) {
            cout << "Can't move there!" << endl;
            return false;
        }

        char target = map[ny][nx];
        bool dark = !player->isVisible(nx, ny);

        if (target == 'x') {
            // Blocked - still costs oxygen (per spec)
            cout << "Blocked by obstacle!" << endl;
            return false;
        }

        // Moving into darkness: hidden enemy gets a free hit
        if (dark) {
            Enemy* lurker = enemyAt(nx, ny);
            if (lurker && lurker->alive) {
                player->takeDamage(lurker->giveDamage());
                lurker->active = true;  // now it's awake
                cout << "*** Something lurks in the dark and strikes! -"
                    << lurker->giveDamage() << " health ***" << endl;
                player->setVisible(nx, ny);
                return false;  // player didn't enter the tile
            }
            // No enemy - player blunders in and reveals the tile
            player->setVisible(nx, ny);
        }

        // Check for stationary enemy on the destination tile (visible)
        Enemy* standing = enemyAt(nx, ny);
        if (standing && standing->alive) {
            // Walking into a stationary enemy damages the player
            player->takeDamage(standing->giveDamage());
            cout << "*** Ouch! You walked into something sharp! -"
                << standing->giveDamage() << " health ***" << endl;
            return false;
        }

        // Execute the move
        setTile(player->x, player->y, 'o');
        player->x = nx; player->y = ny;
        setTile(nx, ny, 'P');
        player->revealAround(nx, ny, 3);  // illuminate circle around new position
        return true;
    }

    // ---- Illuminate a tile adjacent to the player ----
    // (dx, dy) = direction: e.g. (0,-1) for up
    bool illuminateTile(int dx, int dy) {
        if (!player) return false;
        int tx = player->x + dx;
        int ty = player->y + dy;

        if (tx < 0 || tx >= cols || ty < 0 || ty >= rows) {
            cout << "Nothing to illuminate there." << endl;
            return false;
        }
        if (!player->hasBattery()) {
            cout << "Battery empty! Cannot illuminate." << endl;
            return false;
        }

        bool wasNew = !player->isVisible(tx, ty);
        player->illuminate(tx, ty);

        // Wake up any enemy now revealed
        for (Enemy* e : enemies) {
            if (e->alive && e->x == tx && e->y == ty && !e->active) {
                e->active = true;
                cout << "You've illuminated an enemy - it's now active!" << endl;
            }
        }

        if (wasNew) {
            char t = map[ty][tx];
            if (t == 'x')              cout << "Illuminated: wall." << endl;
            else if (t == 'M' || t == 'S')  cout << "Illuminated: ENEMY!" << endl;
            else                            cout << "Illuminated: open passage." << endl;
        }
        return true;
    }

    // ---- Activate enemies the player can currently see (after any move) ----
    void updateEnemyActivation() {
        if (!player) return;
        for (Enemy* e : enemies)
            if (e->alive && !e->active && player->isVisible(e->x, e->y))
                e->active = true;
    }

    // ---- Tick all enemies one step ----
    // Returns true if the player died this tick (caller must stop processing)
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

    // ---- Render: dark tiles shown as space, known tiles shown normally ----
    void render() const {
        if (!map) return;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (player && !player->isVisible(j, i))
                    cout << ' ';
                else
                    cout << map[i][j];
            }
            cout << '\n';
        }
    }

    void freeMap() {
        if (map) {
            for (int i = 0; i < rows; i++) free(map[i]);
            free(map);
            map = nullptr;
        }
        rows = cols = 0;
    }

private:
    Enemy* enemyAt(int ex, int ey) {
        for (Enemy* e : enemies)
            if (e->alive && e->x == ex && e->y == ey) return e;
        return nullptr;
    }
};