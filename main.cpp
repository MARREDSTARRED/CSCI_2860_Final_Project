#include <SDL.h>
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib> 
#include <ctime>   
#include <algorithm>

// --- Configuration Constants ---
const int TILE_SIZE = 32;
const int MAP_WIDTH = 24;
const int MAP_HEIGHT = 16;
const int SCREEN_WIDTH = MAP_WIDTH * TILE_SIZE;
const int SCREEN_HEIGHT = MAP_HEIGHT * TILE_SIZE;

// --- 1. Map Structures & Geometry ---
struct Tile {
    bool isWalkable = false;
};

struct Room {
    int x, y, w, h;

    bool intersects(const Room& other) const {
        return (x <= other.x + other.w && x + w >= other.x &&
            y <= other.y + other.h && y + h >= other.y);
    }

    void getCenter(int& centerX, int& centerY) const {
        centerX = x + w / 2;
        centerY = y + h / 2;
    }
};

class Map {
private:
    int width;
    int height;
    std::vector<Tile> tiles;

public:
    Map(int w, int h) : width(w), height(h) {
        tiles.resize(width * height, Tile());
    }
    int getIndex(int x, int y) const { return (y * width) + x; }
    bool isInBounds(int x, int y) const { return x >= 0 && x < width && y >= 0 && y < height; }
    Tile& getTile(int x, int y) { return tiles[getIndex(x, y)]; }

    bool isWalkable(int x, int y) const {
        if (!isInBounds(x, y)) return false;
        return tiles[getIndex(x, y)].isWalkable;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    void clear() {
        std::fill(tiles.begin(), tiles.end(), Tile{ false });
    }
};

// --- 2. Map Generation Functions ---
void carveHorizontalTunnel(Map& map, int x1, int x2, int y) {
    for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) {
        if (map.isInBounds(x, y)) map.getTile(x, y).isWalkable = true;
    }
}

void carveVerticalTunnel(Map& map, int y1, int y2, int x) {
    for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) {
        if (map.isInBounds(x, y)) map.getTile(x, y).isWalkable = true;
    }
}

std::vector<Room> buildSpaceWreck(Map& map) {
    map.clear();
    std::vector<Room> placedRooms;

    const int MAX_ROOMS = 8;
    const int MIN_SIZE = 3;
    const int MAX_SIZE = 6;

    for (int i = 0; i < MAX_ROOMS; ++i) {
        int w = MIN_SIZE + rand() % (MAX_SIZE - MIN_SIZE + 1);
        int h = MIN_SIZE + rand() % (MAX_SIZE - MIN_SIZE + 1);
        int x = 1 + rand() % (map.getWidth() - w - 2);
        int y = 1 + rand() % (map.getHeight() - h - 2);

        Room newRoom{ x, y, w, h };

        bool overlap = false;
        for (const auto& room : placedRooms) {
            if (newRoom.intersects(room)) {
                overlap = true;
                break;
            }
        }

        if (!overlap) {
            for (int ry = newRoom.y; ry < newRoom.y + newRoom.h; ++ry) {
                for (int rx = newRoom.x; rx < newRoom.x + newRoom.w; ++rx) {
                    map.getTile(rx, ry).isWalkable = true;
                }
            }

            if (!placedRooms.empty()) {
                int newX, newY, prevX, prevY;
                newRoom.getCenter(newX, newY);
                placedRooms.back().getCenter(prevX, prevY);

                if (rand() % 2 == 0) {
                    carveHorizontalTunnel(map, prevX, newX, prevY);
                    carveVerticalTunnel(map, prevY, newY, newX);
                }
                else {
                    carveVerticalTunnel(map, prevY, newY, prevX);
                    carveHorizontalTunnel(map, prevX, newX, newY);
                }
            }
            placedRooms.push_back(newRoom);
        }
    }
    return placedRooms;
}

// --- 3. Entities (Salvage & Enemies) ---
struct Salvage {
    int x, y;
    bool active = true;
};

struct Enemy {
    int x, y;
    bool active = true;

    void takeTurn(const Map& gameMap, int playerX, int playerY, bool& playerDead) {
        if (!active) return;

        int dir = rand() % 4;
        int dx = 0, dy = 0;
        if (dir == 0) dy = -1;
        else if (dir == 1) dy = 1;
        else if (dir == 2) dx = -1;
        else if (dir == 3) dx = 1;

        int targetX = x + dx;
        int targetY = y + dy;

        if (targetX == playerX && targetY == playerY) {
            playerDead = true;
            std::cout << "\n*** A monster got you! GAME OVER. ***\n";
            std::cout << "Press 'R' to restart or 'ESC' to quit.\n";
            return;
        }

        if (gameMap.isWalkable(targetX, targetY)) {
            x = targetX;
            y = targetY;
        }
    }
};

// --- 4. Player Logic ---
class Salvager {
public:
    int x, y;

    Salvager(int startX, int startY) : x(startX), y(startY) {}

    bool move(int dx, int dy, const Map& gameMap, std::vector<Enemy>& enemies, std::vector<Salvage>& salvages, int& score) {
        int targetX = x + dx;
        int targetY = y + dy;

        if (!gameMap.isWalkable(targetX, targetY)) return false;

        for (auto& enemy : enemies) {
            if (enemy.active && enemy.x == targetX && enemy.y == targetY) {
                enemy.active = false;
                std::cout << "You defeated an alien monstrosity!\n";
                return true;
            }
        }

        x = targetX;
        y = targetY;

        for (auto& salvage : salvages) {
            if (salvage.active && salvage.x == x && salvage.y == y) {
                salvage.active = false;
                score += 100;
                std::cout << "Salvage collected! Score: " << score << "\n";
            }
        }
        return true;
    }
};

// --- 5. Game State Controllers ---
bool checkWinCondition(const std::vector<Enemy>& enemies, const std::vector<Salvage>& salvages) {
    for (const auto& enemy : enemies) {
        if (enemy.active) return false;
    }
    for (const auto& salvage : salvages) {
        if (salvage.active) return false;
    }
    return true;
}

void setupNewGame(Map& gameMap, Salvager& player, std::vector<Enemy>& alienList, std::vector<Salvage>& scrapList, int& score, bool& isGameOver, bool& isGameWon, bool& needsRender) {
    std::vector<Room> layoutRooms = buildSpaceWreck(gameMap);

    int startX, startY;
    layoutRooms[0].getCenter(startX, startY);
    player.x = startX;
    player.y = startY;

    scrapList.clear();
    alienList.clear();

    for (size_t i = 1; i < layoutRooms.size(); ++i) {
        int rx, ry;
        layoutRooms[i].getCenter(rx, ry);
        scrapList.push_back({ rx, ry });

        if (gameMap.isWalkable(rx + 1, ry)) {
            alienList.push_back({ rx + 1, ry });
        }
        else {
            alienList.push_back({ rx, ry });
        }
    }

    score = 0;
    isGameOver = false;
    isGameWon = false;
    needsRender = true;
    std::cout << "\n====================================\n";
    std::cout << "BOARDING NEW WRECK... COMMENCE SALVAGE\n";
    std::cout << "====================================\n";
}

// --- 6. Main Game Loop ---
int main(int argc, char* argv[]) {
    srand((unsigned)time(0));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;

    SDL_Window* window = SDL_CreateWindow("AstraScrip", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    Map gameMap(MAP_WIDTH, MAP_HEIGHT);
    Salvager player(0, 0);
    std::vector<Salvage> scrapList;
    std::vector<Enemy> alienList;

    int score = 0;
    bool isGameOver = false;
    bool isGameWon = false;
    bool isRunning = true;
    bool needsRender = true;
    SDL_Event event;

    // Initialize the very first map
    setupNewGame(gameMap, player, alienList, scrapList, score, isGameOver, isGameWon, needsRender);

    while (isRunning) {
        if (needsRender) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            // Render Map Grid
            for (int my = 0; my < gameMap.getHeight(); ++my) {
                for (int mx = 0; mx < gameMap.getWidth(); ++mx) {
                    SDL_Rect tileRect = { mx * TILE_SIZE, my * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                    if (gameMap.isWalkable(mx, my)) SDL_SetRenderDrawColor(renderer, 35, 35, 45, 255);
                    else SDL_SetRenderDrawColor(renderer, 70, 70, 80, 255);
                    SDL_RenderFillRect(renderer, &tileRect);
                    SDL_SetRenderDrawColor(renderer, 20, 20, 25, 255);
                    SDL_RenderDrawRect(renderer, &tileRect);
                }
            }

            // Render Salvage
            for (const auto& salvage : scrapList) {
                if (salvage.active) {
                    SDL_Rect sRect = { salvage.x * TILE_SIZE + 8, salvage.y * TILE_SIZE + 8, TILE_SIZE - 16, TILE_SIZE - 16 };
                    SDL_SetRenderDrawColor(renderer, 235, 210, 50, 255);
                    SDL_RenderFillRect(renderer, &sRect);
                }
            }

            // Render Enemies
            for (const auto& enemy : alienList) {
                if (enemy.active) {
                    SDL_Rect eRect = { enemy.x * TILE_SIZE + 4, enemy.y * TILE_SIZE + 4, TILE_SIZE - 8, TILE_SIZE - 8 };
                    SDL_SetRenderDrawColor(renderer, 220, 60, 60, 255);
                    SDL_RenderFillRect(renderer, &eRect);
                }
            }

            // Render Salvager Color based on game state
            SDL_Rect playerRect = { player.x * TILE_SIZE + 2, player.y * TILE_SIZE + 2, TILE_SIZE - 4, TILE_SIZE - 4 };
            if (isGameWon) {
                SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255); // Gold (Win)
            }
            else if (isGameOver) {
                SDL_SetRenderDrawColor(renderer, 100, 0, 0, 255); // Dark Red (Dead)
            }
            else {
                SDL_SetRenderDrawColor(renderer, 50, 210, 215, 255); // Cyan (Alive)
            }
            SDL_RenderFillRect(renderer, &playerRect);

            SDL_RenderPresent(renderer);
            needsRender = false;
        }

        if (SDL_WaitEvent(&event)) {
            if (event.type == SDL_QUIT) isRunning = false;

            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) isRunning = false;

                // Handle Game Over / Win State
                if (isGameOver || isGameWon) {
                    if (event.key.keysym.sym == SDLK_r) {
                        setupNewGame(gameMap, player, alienList, scrapList, score, isGameOver, isGameWon, needsRender);
                    }
                }
                // Handle Normal Gameplay State
                else {
                    bool turnTaken = false;
                    switch (event.key.keysym.sym) {
                    case SDLK_w: case SDLK_UP:    turnTaken = player.move(0, -1, gameMap, alienList, scrapList, score); break;
                    case SDLK_s: case SDLK_DOWN:  turnTaken = player.move(0, 1, gameMap, alienList, scrapList, score); break;
                    case SDLK_a: case SDLK_LEFT:  turnTaken = player.move(-1, 0, gameMap, alienList, scrapList, score); break;
                    case SDLK_d: case SDLK_RIGHT: turnTaken = player.move(1, 0, gameMap, alienList, scrapList, score); break;
                    }

                    if (turnTaken) {
                        for (auto& enemy : alienList) {
                            enemy.takeTurn(gameMap, player.x, player.y, isGameOver);
                        }

                        // Check if the board is cleared
                        if (!isGameOver && checkWinCondition(alienList, scrapList)) {
                            isGameWon = true;
                            std::cout << "\n*** MISSION ACCOMPLISHED! ***\n";
                            std::cout << "Sector clear. Final Score: " << score << "\n";
                            std::cout << "Press 'R' to board a new wreck or 'ESC' to quit.\n";
                        }

                        needsRender = true;
                    }
                }
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}