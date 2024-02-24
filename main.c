#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>
#include <raymath.h>

// 4:3 = 640x480, 800x600, 1024x768 -  16:9 = 1280x720, 1920x1080

const int WIDTH = 1280;
const int HEIGHT = 720;
const int CELL_SIZE = 20;
const int LIMIT_FACTOR = 3;
const int MAX_FPS = 60;
const int ANIMATION_FRAME_DELAY = MAX_FPS / 4;
const int FONT_SIZE = 20;

static const char* keysHint = "L-click to add, R-click to remove\n"
  "M-click to scroll grid, wheel to zoom\n"
  "Space to pause/unpause\n"
  "R to init random grid\n"
  "C to clear grid\n"
  "H to hide this\n";
  
static const int NEIGHBOURS[8][2] = {
  {-1, -1}, {0, -1}, {1, -1},
  {-1,  0}, {1,  0},
  {-1,  1}, {0,  1}, {1,  1}
};

typedef struct {
  bool *state;
  bool *stateBuffer;
  int width;
  int height;
  size_t alive;
} Grid;

Grid newGrid(int width, int height) {
  Grid grid;
  grid.width = width;
  grid.height = height;
  grid.state = malloc(width * height * sizeof(bool));
  grid.stateBuffer = malloc(width * height * sizeof(bool));

  return grid;
}

void deleteGrid(Grid* grid) {
  free(grid->state);
  free(grid->stateBuffer);
}

void swapBuffers(Grid* grid) {
  bool* tmp = grid->state;
  grid->state = grid->stateBuffer;
  grid->stateBuffer = tmp;
}

int getIndex(Grid *grid, int x, int y) {
  return (x < 0 || x >= grid->width || y < 0 || y >= grid->height)
    ? -1 
    : grid->width * y + x;
}

void copyBuffer(Grid* grid) {
  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      int idx = getIndex(grid, x, y);
      grid->stateBuffer[idx] = grid->state[idx];
    }
  }
}

void initEmpty(Grid* grid) {
  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      int idx = getIndex(grid, x, y);
      grid->state[idx] = false;
    }
  }

  copyBuffer(grid);
}

void initRandom(Grid* grid) {
  srand(time(0));

  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      int idx = getIndex(grid, x, y);
      bool value = rand() & 1;
      grid->state[idx] = value;
    }
  }

  copyBuffer(grid);
}

unsigned int countCellNeighbours(Grid* grid, int x, int y) {
  unsigned int count = 0;
  for (int n=0; n<8; n++) {
    int idx = getIndex(grid, x + NEIGHBOURS[n][0], y + NEIGHBOURS[n][1]);
    if (idx != -1 && grid->state[idx]) count++; 
  }

  return count;
}

bool nextCellState(Grid* grid, int x, int y) {
  unsigned int neighbours = countCellNeighbours(grid, x, y); 
  int idx = getIndex(grid, x, y);

  if ( grid->state[idx] ) {
    if ( neighbours == 2 || neighbours == 3 ) return true; 
  } else {
    if ( neighbours == 3 ) return true;
  }

  return false;
}

void updateGrid(Grid *grid) {
  grid->alive = 0;
  
  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      int idx = getIndex(grid, x, y);
      bool state = nextCellState(grid, x, y);
      grid->stateBuffer[idx] = state;
      grid->alive += state ? 1 : 0;
    }
  }

  swapBuffers(grid);
}

void drawGrid(Grid * grid) {
  for (int y = 0; y < grid->height; y++) {
    for (int x = 0; x < grid->width; x++) {
      int idx = getIndex(grid, x, y);
      if (grid->state[idx]) {
        DrawRectangle(
          x * CELL_SIZE, 
          y * CELL_SIZE, 
          CELL_SIZE, CELL_SIZE, RAYWHITE
        );
      }
    }
  }
}

bool getCell(Grid* grid, int x, int y) {
  int idx = getIndex(grid, x, y);
  return grid->state[idx];
}

void setCell(Grid* grid, int x, int y, bool state) {
  int idx = getIndex(grid, x, y);
  if (idx == -1) return;
  grid->state[idx] = state;
}

void drawLines() {
  for (int i = 0; i <= WIDTH*LIMIT_FACTOR; i += CELL_SIZE) {
    DrawLine(i, 0, i, HEIGHT*LIMIT_FACTOR, DARKGRAY);
  }

  for (int i = 0; i <= HEIGHT*LIMIT_FACTOR; i += CELL_SIZE) {
    DrawLine(0, i, WIDTH*LIMIT_FACTOR, i, DARKGRAY);
  }
}

void handleMouseScroll(Camera2D* camera) {
  Vector2 dm = GetMouseDelta();

  camera->target.x -= dm.x * 1 / camera->zoom;
  camera->target.y -= dm.y * 1 / camera->zoom;

  camera->target.x = Clamp(camera->target.x, 0, WIDTH*LIMIT_FACTOR);
  camera->target.y = Clamp(camera->target.y, 0, HEIGHT*LIMIT_FACTOR);
}

void handleMouseZoom(Camera2D* camera) {
  float zooming = GetMouseWheelMove() * 0.125;
  if ( zooming != 0 ) {
    camera->zoom += zooming;
    camera->zoom = Clamp(camera->zoom, 0.3, 1.5);
  }
}

Vector2 mouseScreenPositionToWorldPosition(Camera2D camera) {
  Vector2 m = GetMousePosition();
  Vector2 mworld = GetScreenToWorld2D(m, camera);
  return (Vector2){(int)mworld.x / CELL_SIZE, (int)mworld.y / CELL_SIZE };
}

int main() {
  InitWindow(WIDTH, HEIGHT, "LIFE");
  SetTargetFPS(MAX_FPS);

  const Vector2 center = { .x = WIDTH/2, .y = HEIGHT/2 };
  Grid grid = newGrid((WIDTH*LIMIT_FACTOR) / CELL_SIZE, (HEIGHT*LIMIT_FACTOR) / CELL_SIZE);
  initRandom(&grid);

  Camera2D camera = {
    .target = (Vector2) {center.x*LIMIT_FACTOR, center.y*LIMIT_FACTOR},
    .offset = center,
    .rotation = 0,
    .zoom = 1
  };

  bool animationOn = true;
  bool showHint = true;

  int timer = 0;
  char textBuffer[32];

  while (!WindowShouldClose())
  {
    if (IsKeyPressed(KEY_SPACE)) {
      animationOn = !animationOn;
    }
    
    if (IsKeyPressed(KEY_H)) {
      showHint = !showHint;
    }
    
    if (IsKeyPressed(KEY_R)) {
      initRandom(&grid);
    }

    if (IsKeyPressed(KEY_C)) {
      initEmpty(&grid);
    }

    if (animationOn && timer >= ANIMATION_FRAME_DELAY) {
      timer = 0;
      updateGrid(&grid);
    }

    BeginDrawing();
    {
      ClearBackground(BLACK);

      if ( IsMouseButtonDown(2) ) {
        handleMouseScroll(&camera);
      }
      handleMouseZoom(&camera);

      Vector2 mworld = mouseScreenPositionToWorldPosition(camera);

      if ( IsMouseButtonDown(0) ) {
        setCell(&grid, mworld.x, mworld.y, true);
      } else if ( IsMouseButtonDown(1) ) {
        setCell(&grid, mworld.x, mworld.y, false);
      }

      BeginMode2D(camera);
      {
        drawGrid(&grid);
        drawLines();

        // Red cursor
        DrawRectangle(
          mworld.x * CELL_SIZE, mworld.y * CELL_SIZE,
          CELL_SIZE, CELL_SIZE, RED
        );
      }
      EndMode2D();

      sprintf(textBuffer, "LIFE - FPS: %d", GetFPS());
      SetWindowTitle(textBuffer);
    }

    if (showHint) DrawText(keysHint, 10, 10, FONT_SIZE, GOLD);
    EndDrawing();

    timer += animationOn ? 1 : 0;
  }

  deleteGrid(&grid);
  CloseWindow();
  return 0;
}