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

void gameOver();


/*==========================================================
 * CLASS: Player
 *==========================================================*/
class Player {
public:
    int health;
    int oxygen;
    int battery;
    int lives;
    int score;      // NEW: accumulated score from gold coins and treasures

    int x, y;
    int spawnX, spawnY;

    bool** visibility;
    int visRows;
    int visCols;

    Player(int startX = 0, int startY = 0)
        : health(100), oxygen(100), battery(100), lives(3), score(0),
        x(startX), y(startY), spawnX(startX), spawnY(startY),
        visibility(nullptr), visRows(0), visCols(0)
    {
    }

    ~Player() {
        freeVisibility();
        cout << "The diver's tale ends here. Farewell, brave soul.\n";
        gameOver();
    }

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    void allocVisibility(int r, int c) {
        freeVisibility();
        visRows = r;
        visCols = c;
        visibility = new bool* [r];
        for (int i = 0; i < r; i++)
            visibility[i] = new bool[c]();
    }

    void freeVisibility() {
        if (visibility) {
            for (int i = 0; i < visRows; i++)
                delete[] visibility[i];
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

    void revealAround(int cx, int cy, int radius) {
        for (int dy = -radius; dy <= radius; dy++)
            for (int dx = -radius; dx <= radius; dx++)
                if (dx * dx + dy * dy <= radius * radius)
                    setVisible(cx + dx, cy + dy);
    }

    bool illuminate(int cx, int cy) {
        if (battery <= 0) return false;
        battery = max(0, battery - 5);
        setVisible(cx, cy);
        return true;
    }

    void setSpawn(int sx, int sy) { spawnX = sx; spawnY = sy; }
    void reduceOxygen(int amount) { oxygen = max(0, oxygen - amount); }
    void takeDamage(int amount) { health = max(0, health - amount); }
    void heal(int amount) { health = min(100, health + amount); }
    void refillOxygen(int amount) { oxygen = min(100, oxygen + amount); }
    void refillBattery(int amount) { battery = min(100, battery + amount); }
    void addScore(int amount) { score += amount; }

    bool isAlive()    const { return health > 0; }
    bool hasOxygen()  const { return oxygen > 0; }
    bool hasBattery() const { return battery > 0; }

    bool loseLife() {
        lives--;
        if (lives <= 0) { lives = 0; return false; }
        health = oxygen = battery = 100;
        x = spawnX;
        y = spawnY;
        return true;
    }

    void reset(int sx, int sy) {
        health = oxygen = battery = 100;
        lives = 3;
        score = 0;
        x = sx; y = sy;
        spawnX = sx; spawnY = sy;
    }
};


/*==========================================================
 * BASE CLASS: Enemy
 *==========================================================*/
class Enemy {
public:
    int  x, y;
    int  health;
    int  damage;
    char symbol;
    bool alive;
    bool active;

protected:
    int moveTimer;
    int moveCooldown;

public:
    Enemy(int sx = 0, int sy = 0, char sym = 'M', int dmg = 25, int cooldown = 2)
        : x(sx), y(sy), health(50), damage(dmg), symbol(sym),
        alive(true), active(false), moveTimer(0), moveCooldown(cooldown)
    {
    }

    virtual ~Enemy() {}

    virtual bool update(int playerX, int playerY,
        char** map, int rows, int cols) = 0;

    int giveDamage() const { return damage; }

    void takeDamage(int amount) {
        health -= amount;
        if (health <= 0) { health = 0; alive = false; }
    }

    Enemy(const Enemy&) = delete;
    Enemy& operator=(const Enemy&) = delete;

protected:
    // Enemies treat collectible item tiles ('B', 'O', 'G') the same as open floor:
    // they can walk over them without picking them up.
    bool canStep(int nx, int ny, char** map, int rows, int cols) const {
        if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) return false;
        char t = map[ny][nx];
        // 'o' = open floor, 'P' = player tile
        // 'B','O','G' = collectible items: passable by enemies (they don't pick up)
        return (t == 'o' || t == 'P' || t == 'B' || t == 'O' || t == 'G');
    }
};


/*==========================================================
 * SUBCLASS: MovingEnemy  (symbol 'M')
 *==========================================================*/
class MovingEnemy : public Enemy {
public:
    MovingEnemy(int sx = 0, int sy = 0, char sym = 'M', int dmg = 25)
        : Enemy(sx, sy, sym, dmg, 2)
    {
    }

    bool update(int playerX, int playerY,
        char** map, int rows, int cols) override
    {
        if (!alive) return false;

        moveTimer++;
        if (moveTimer < moveCooldown) return false;
        moveTimer = 0;

        int newX = x, newY = y;

        if (active) {
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
            if (rand() % 4 == 0) return false;
            int dirs[4][2] = { {0,-1}, {0,1}, {-1,0}, {1,0} };
            int d = rand() % 4;
            int tx = x + dirs[d][0];
            int ty = y + dirs[d][1];
            if (canStep(tx, ty, map, rows, cols)) { newX = tx; newY = ty; }
        }

        bool hitPlayer = (newX == playerX && newY == playerY);

        if (!hitPlayer) {
            // When moving, preserve collectible item symbols on the destination tile.
            // The enemy passes over the item without picking it up.
            char destTile = map[newY][newX];
            bool destIsItem = (destTile == 'B' || destTile == 'O' || destTile == 'G');

            // The enemy's old tile is always 'M' (drawn by World), not the item symbol,
            // because items and enemies cannot share a tile on spawn. Restore to 'o'.
            map[y][x] = 'o';

            // Only overwrite the destination if it is NOT an item tile.
            // Items stay visible on the map; the enemy occupies the tile logically
            // but the item symbol takes display priority (the item is still there).
            if (!destIsItem) {
                map[newY][newX] = symbol;
            }
            // If the destination IS an item tile, we move the enemy's logical position
            // but leave the map character as the item symbol so the player can see it.
        }
        x = newX; y = newY;
        return hitPlayer;
    }
};


/*==========================================================
 * SUBCLASS: StationaryEnemy  (symbol 'S')
 *==========================================================*/
class StationaryEnemy : public Enemy {
public:
    StationaryEnemy(int sx = 0, int sy = 0, char sym = 'S', int dmg = 15)
        : Enemy(sx, sy, sym, dmg, 999)
    {
    }

    bool update(int, int, char**, int, int) override { return false; }
};


/*==========================================================
 * BASE CLASS: CollectibleItem
 *
 * Abstract base for all collectible items placed on the map.
 * Subclasses define what happens when the player picks them up
 * by implementing the pure virtual applyTo() method.
 *
 * TEACHING POINT - Inheritance and polymorphism for items:
 *   The assignment requires both Battery and ExtraOxygen to be
 *   handled through the same mechanisms. By storing them all as
 *   CollectibleItem* in a single vector, World can loop over every
 *   item with one piece of code and call applyTo() without knowing
 *   which concrete subclass it is dealing with. This mirrors the
 *   same polymorphic pattern used for Enemy/MovingEnemy/StationaryEnemy.
 *==========================================================*/
class CollectibleItem {
public:
    int  x, y;          // position on the map grid
    bool collected;     // true once the player has stepped on this tile
    char symbol;        // character shown on the map ('B', 'O', 'G', '$')

    // Constructor - sets position, symbol and marks uncollected
    CollectibleItem(int sx = 0, int sy = 0, char sym = '?')
        : x(sx), y(sy), collected(false), symbol(sym)
    {
    }

    // Virtual destructor required for safe polymorphic deletion.
    // Without this, deleting a Battery through a CollectibleItem* would
    // only run the base destructor, potentially leaking subclass resources.
    virtual ~CollectibleItem() {}

    // Pure virtual: every subclass must define what it does to the player.
    // Returns a human-readable message describing what was picked up.
    virtual string applyTo(Player& player) = 0;

    // Copying collectible items is disabled. They live in the world at a
    // fixed position and are managed through pointers; copying them would
    // create orphaned duplicates on the map.
    CollectibleItem(const CollectibleItem&) = delete;
    CollectibleItem& operator=(const CollectibleItem&) = delete;
};


/*==========================================================
 * SUBCLASS: BatteryItem  (symbol 'B')
 *
 * Restores the player's flashlight battery by a fixed amount.
 * Useful when exploring large dark areas.
 *==========================================================*/
class BatteryItem : public CollectibleItem {
    int rechargeAmount;  // how many battery % points to restore

public:
    BatteryItem(int sx = 0, int sy = 0, int amount = 30)
        : CollectibleItem(sx, sy, 'B'), rechargeAmount(amount)
    {
    }

    // applyTo() is called by World::tryMovePlayer() the moment the player
    // steps onto the tile. It restores battery and marks the item collected
    // so it cannot be picked up again even if the player stays on the tile.
    string applyTo(Player& player) override {
        if (collected) return "";
        player.refillBattery(rechargeAmount);
        collected = true;
        return "*** Battery recharged! +" + to_string(rechargeAmount) + "% battery ***";
    }
};


/*==========================================================
 * SUBCLASS: OxygenItem  (symbol 'O')
 *
 * Restores the player's oxygen supply by a fixed amount.
 * Critical for survival in oxygen-deprived sections.
 *==========================================================*/
class OxygenItem : public CollectibleItem {
    int refillAmount;   // how many oxygen % points to restore

public:
    OxygenItem(int sx = 0, int sy = 0, int amount = 40)
        : CollectibleItem(sx, sy, 'O'), refillAmount(amount)
    {
    }

    string applyTo(Player& player) override {
        if (collected) return "";
        player.refillOxygen(refillAmount);
        collected = true;
        return "*** Oxygen canister found! +" + to_string(refillAmount) + "% oxygen ***";
    }
};


/*==========================================================
 * SUBCLASS: GoldCoinItem  (symbol 'G')  [optional bonus item]
 *
 * Awards points to the player. Does not affect survival stats,
 * but contributes to the final score shown on the win screen.
 *==========================================================*/
class GoldCoinItem : public CollectibleItem {
    int points;         // score value of this coin

public:
    GoldCoinItem(int sx = 0, int sy = 0, int pts = 100)
        : CollectibleItem(sx, sy, 'G'), points(pts)
    {
    }

    string applyTo(Player& player) override {
        if (collected) return "";
        player.addScore(points);
        collected = true;
        return "*** Gold coin! +" + to_string(points) + " points (total: "
            + to_string(player.score) + ") ***";
    }
};


/*==========================================================
 * CLASS: Item  (legacy treasure item - unchanged)
 *
 * Kept for the existing '$' treasure / win-condition mechanism.
 * The new collectible items use the CollectibleItem hierarchy above.
 *==========================================================*/
class Item {
public:
    enum Type { TREASURE, OXYGEN, HEALTH, BATTERY, KEY };

    int  x, y;
    Type type;
    int  value;
    bool collected;
    char symbol;

    Item(int sx = 0, int sy = 0, Type t = TREASURE, int val = 10, char sym = '$')
        : x(sx), y(sy), type(t), value(val), collected(false), symbol(sym)
    {
    }

    ~Item() {}

    Item(const Item& other)
        : x(other.x), y(other.y), type(other.type), value(other.value),
        collected(other.collected), symbol(other.symbol)
    {
    }

    void applyTo(Player& player) {
        if (collected) return;
        switch (type) {
        case OXYGEN:   player.refillOxygen(value);  break;
        case HEALTH:   player.heal(value);           break;
        case BATTERY:  player.refillBattery(value);  break;
        case TREASURE: break;
        case KEY:      break;
        }
        collected = true;
    }
};


/*==========================================================
 * CLASS: World
 *==========================================================*/
class World {
public:
    char** map;
    int    rows;
    int    cols;
    string mapFilePath;

    bool animateNextRender;

    Player* player;

    vector<Enemy*>           enemies;       // owning: created with new, deleted in destructor
    vector<Item>             items;         // legacy treasure items (value type, no new/delete)
    vector<CollectibleItem*> collectibles;  // NEW: battery, oxygen, gold coins (owning pointers)

    World()
        : map(nullptr), rows(0), cols(0),
        animateNextRender(false), player(nullptr)
    {
    }

    ~World() {
        freeMap();
        for (Enemy* e : enemies)            delete e;
        for (CollectibleItem* c : collectibles) delete c;
        enemies.clear();
        collectibles.clear();
    }

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    // ---------------------------------------------------------------
    // Tile access helpers
    // ---------------------------------------------------------------
    char getTile(int x, int y) const {
        if (x < 0 || x >= cols || y < 0 || y >= rows) return 'x';
        return map[y][x];
    }

    void setTile(int x, int y, char c) {
        if (x >= 0 && x < cols && y >= 0 && y < rows) map[y][x] = c;
    }

    bool isWalkable(int x, int y) const { return getTile(x, y) != 'x'; }

    // ---------------------------------------------------------------
    // Treasure tracking (unchanged)
    // ---------------------------------------------------------------
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

    bool allTreasuresFound() const {
        pair<int, int> tc = treasureCount();
        int found = tc.first, total = tc.second;
        return total > 0 && found == total;
    }

    // ---------------------------------------------------------------
    // loadFromFile(filepath)
    // Now also recognises 'B', 'O', 'G' map symbols and creates the
    // appropriate CollectibleItem subclass for each.
    // ---------------------------------------------------------------
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
        for (Enemy* e : enemies)            delete e;
        for (CollectibleItem* c : collectibles) delete c;
        enemies.clear();
        items.clear();
        collectibles.clear();   // clear the new container too

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

        // Scan tiles and create game objects
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
                else if (c == '$') {
                    items.push_back(Item(j, i, Item::TREASURE, 0, '$'));
                }

                // NEW: collectible item symbols -> CollectibleItem subclass instances.
                // Each is allocated on the heap with 'new' and stored in collectibles.
                // World owns these objects and deletes them in the destructor and
                // at the start of the next loadFromFile() call.
                else if (c == 'B') {
                    collectibles.push_back(new BatteryItem(j, i, 30));
                }
                else if (c == 'O') {
                    collectibles.push_back(new OxygenItem(j, i, 40));
                }
                else if (c == 'G') {
                    collectibles.push_back(new GoldCoinItem(j, i, 100));
                }
            }
        }

        if (player) {
            player->allocVisibility(rows, cols);
            player->revealAround(player->x, player->y, 3);
        }

        animateNextRender = true;

        if (!enemies.empty())
            cout << "[" << enemies.size() << " enemy/enemies on this level]" << endl;

        pair<int, int> tc = treasureCount();
        if (tc.second > 0)
            cout << "[" << tc.second << " treasure(s) hidden - find them all to advance!]" << endl;

        if (!collectibles.empty())
            cout << "[" << collectibles.size()
            << " collectible item(s) on this level: B=battery  O=oxygen  G=gold]" << endl;

        return 0;
    }

    // ---------------------------------------------------------------
    // tryMovePlayer(dx, dy)
    //
    // CHANGES from original:
    //   - Collectible item tiles ('B', 'O', 'G') are NOT treated as walls.
    //     The player can move onto them freely.
    //   - After a successful move, we check whether the destination tile
    //     is a collectible item and call applyTo() if so. The item is then
    //     marked collected and its tile replaced with open floor 'o' so
    //     it is no longer visible on the map.
    //   - Enemies can walk over item tiles (handled in canStep), so item
    //     tiles must also NOT block player movement here.
    // ---------------------------------------------------------------
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
            cout << "Blocked by obstacle!" << endl;
            return false;
        }

        // Dark tile check: ambush if an enemy is hiding there.
        // Item tiles in darkness do NOT trigger an ambush - only enemies do.
        if (dark) {
            Enemy* lurker = enemyAt(nx, ny);
            if (lurker && lurker->alive) {
                player->takeDamage(lurker->giveDamage());
                lurker->active = true;
                cout << "*** Something lurks in the dark and strikes! -"
                    << lurker->giveDamage() << " health ***" << endl;
                player->setVisible(nx, ny);
                return false;
            }
            player->setVisible(nx, ny);
        }

        // Visible stationary enemy blocks entry.
        // Item tiles are NOT enemies, so this check is unaffected.
        Enemy* standing = enemyAt(nx, ny);
        if (standing && standing->alive) {
            player->takeDamage(standing->giveDamage());
            cout << "*** Ouch! You walked into something sharp! -"
                << standing->giveDamage() << " health ***" << endl;
            return false;
        }

        // Move is valid
        setTile(player->x, player->y, 'o');
        player->x = nx;
        player->y = ny;
        setTile(nx, ny, 'P');

        player->revealAround(nx, ny, 3);

        // --- Collect legacy treasure ---
        for (Item& item : items) {
            if (!item.collected && item.x == nx && item.y == ny
                && item.type == Item::TREASURE) {
                item.applyTo(*player);
                pair<int, int> tc = treasureCount();
                cout << "*** You found a treasure! ("
                    << tc.first << "/" << tc.second << " collected) ***" << endl;
                break;
            }
        }

        // --- Collect new collectible items (Battery, Oxygen, GoldCoin) ---
        // We loop over the collectibles vector and look for an uncollected item
        // at the player's new position. applyTo() applies the effect and returns
        // the pickup message; we then clear the tile on the map so the symbol
        // disappears. The CollectibleItem object remains in the vector (marked
        // collected = true) so it is never applied again, and is cleaned up by
        // the World destructor or the next loadFromFile() call.
        for (CollectibleItem* c : collectibles) {
            if (!c->collected && c->x == nx && c->y == ny) {
                string msg = c->applyTo(*player);
                if (!msg.empty()) cout << msg << endl;
                // Erase the item symbol from the map - the tile becomes open floor.
                // The 'P' we just drew above already covers it visually, but setting
                // it to 'o' ensures the tile stays open after the player leaves.
                setTile(nx, ny, 'P');  // already set above; keep as 'P' while player is here
                // (when player leaves this tile, setTile sets it to 'o' correctly)
                break;
            }
        }

        return true;
    }

    // ---------------------------------------------------------------
    // illuminateTile(dx, dy)  - unchanged from original
    // Also reports if a collectible item tile is revealed.
    // ---------------------------------------------------------------
    bool illuminateTile(int dx, int dy) {
        if (!player) return false;
        if (dx == 0 && dy == 0) return false;

        if (!player->hasBattery()) {
            cout << "Battery empty! Cannot illuminate." << endl;
            return false;
        }

        player->battery = max(0, player->battery - 5);

        bool foundEnemy = false;
        bool foundWall = false;
        bool foundTreasure = false;
        bool foundItem = false;
        int  tilesRevealed = 0;

        int tx = player->x + dx;
        int ty = player->y + dy;

        while (tx >= 0 && tx < cols && ty >= 0 && ty < rows) {
            bool wasNew = !player->isVisible(tx, ty);
            player->setVisible(tx, ty);
            tilesRevealed++;

            char t = map[ty][tx];

            if (wasNew && (t == 'M' || t == 'S')) {
                for (Enemy* e : enemies) {
                    if (e->alive && !e->active && e->x == tx && e->y == ty) {
                        e->active = true;
                        foundEnemy = true;
                        cout << "Your light catches something - an enemy wakes up!" << endl;
                    }
                }
            }

            if (wasNew && t == '$') foundTreasure = true;

            // NEW: note if the beam hits a collectible item for the first time
            if (wasNew && (t == 'B' || t == 'O' || t == 'G')) foundItem = true;

            if (t == 'x') { foundWall = true; break; }

            tx += dx;
            ty += dy;
        }

        if (tilesRevealed == 0)  cout << "Nothing to illuminate there." << endl;
        else if (foundEnemy)     cout << "Illuminated: ENEMY spotted in the darkness!" << endl;
        else if (foundTreasure)  cout << "Illuminated: you spot a glint of TREASURE!" << endl;
        else if (foundItem)      cout << "Illuminated: something useful glints in the dark!" << endl;
        else if (foundWall)      cout << "Illuminated: corridor ends at a wall." << endl;
        else                     cout << "Illuminated: passage stretches into the dark." << endl;

        return true;
    }

    void updateEnemyActivation() {
        if (!player) return;
        for (Enemy* e : enemies)
            if (e->alive && !e->active && player->isVisible(e->x, e->y))
                e->active = true;
    }

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

    void render() {
        if (!map) return;

        bool animate = animateNextRender;
        animateNextRender = false;

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (player && !player->isVisible(j, i))
                    cout << ' ';
                else
                    cout << map[i][j];
            }
            cout << '\n' << flush;

            if (animate)
                this_thread::sleep_for(chrono::milliseconds(18));
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