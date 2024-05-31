#ifndef PATH_CPP
#define PATH_CPP

#include <stdlib.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>

#include <vector>
#include <deque>

#define PATH_BLOCK_EMPTY -3
#define PATH_BLOCK_TO_CALCULATE -2
#define PATH_BLOCK_WALL -1
#define PATH_BLOCK_UP 0
#define PATH_BLOCK_LEFT 1
#define PATH_BLOCK_DOWN 2
#define PATH_BLOCK_RIGHT 3

class Path_Block {
public:
    int type;
    Path_Block(int type) : type(type) {}
};

class Path_Grid {
public:
    int size; // sqrt(path_Grid.size())
    std::vector<Path_Block> grid;

    float scan_Height;
};

Path_Grid top_Grid;
Path_Grid bottom_Grid;

std::deque<Vector2> path_Todo_List = {};
Vector2 path_Target = {};

Path_Block *Path_Grid_Access(Path_Grid *grid, int x, int y) {
    return &grid->grid[x * grid->size + y];
}

bool Map_Collide(std::vector<BoundingBox> map, BoundingBox target) {
    for(BoundingBox bbox : map) {
        if(CheckCollisionBoxes(bbox, target))
            return true;
    }
    return false;
}

Vector2 Path_World_To_Grid(Path_Grid *grid, Vector3 position, BoundingBox house_Bbox) {
    return {Remap(position.x, house_Bbox.min.x, house_Bbox.max.x, 0.f, grid->size), Remap(position.z, house_Bbox.min.z, house_Bbox.max.z, 0.f, grid->size)};
}

Model arrow;

Path_Grid Init_Path(int size, std::vector<BoundingBox> bboxes, BoundingBox house_Bbox, float scan_Height) {
    arrow = LoadModel("assets/models/arrow.glb");
    Path_Grid new_Grid;
    
    new_Grid.size = size;
    new_Grid.scan_Height = scan_Height;

    new_Grid.grid.clear();

    for(int index = 0; index < size * size; index++) {
        new_Grid.grid.push_back(Path_Block(PATH_BLOCK_EMPTY));
    }

    // x, z
    Vector2 house_Size = {fabs(house_Bbox.min.x - house_Bbox.max.x), fabs(house_Bbox.min.z - house_Bbox.max.z)};
    float map_Size = std::max(house_Size.x, house_Size.y);
    float tile_Size = map_Size / new_Grid.size;

    int x = 0, y = 0;
    for(float x_Position = house_Bbox.min.x; x_Position < house_Bbox.max.x; x_Position += tile_Size) {
        y = 0;
        for(float y_Position = house_Bbox.min.z; y_Position < house_Bbox.max.z; y_Position += tile_Size) {
            Path_Block *block = Path_Grid_Access(&new_Grid, x, y);
            Vector2 position = {x_Position, y_Position};
            BoundingBox bbox = {{position.x - tile_Size / 2.f, scan_Height - 1.f, position.y - tile_Size / 2.f}, {position.x + tile_Size / 2.f, scan_Height + 1.f, position.y + tile_Size / 2.f}};

            if(Map_Collide(bboxes, bbox)) {
                block->type = PATH_BLOCK_WALL;
            }
            y++;
        }
        x++;
    }

    return new_Grid;
}

void Draw_Path_3D(Path_Grid *grid, BoundingBox house_Bbox, std::vector<BoundingBox> bboxes) {
    Vector2 house_Size = {fabs(house_Bbox.min.x - house_Bbox.max.x), fabs(house_Bbox.min.z - house_Bbox.max.z)};
    float map_Size = std::max(house_Size.x, house_Size.y);
    float tile_Size = map_Size / grid->size;

    int x = 0, y = 0;
    for(float x_Position = house_Bbox.min.x; x_Position < house_Bbox.max.x; x_Position += tile_Size) {
        y = 0;
        for(float y_Position = house_Bbox.min.z; y_Position < house_Bbox.max.z; y_Position += tile_Size) {
            Path_Block *block = Path_Grid_Access(grid, x, y);
            Vector2 position = {x_Position, y_Position};
            BoundingBox bbox = {{position.x - tile_Size / 2.f, grid->scan_Height - 1.f, position.y - tile_Size / 2.f}, {position.x + tile_Size / 2.f, grid->scan_Height + 1.f, position.y + tile_Size / 2.f}};

            bool collision = block->type == PATH_BLOCK_WALL;

            Color color;
            if(collision) color = WHITE;
            else {
                if(block->type == PATH_BLOCK_UP) { // nahoru
                    color = RED;
                } else if(block->type == PATH_BLOCK_LEFT) { // vlevo
                    color = GREEN;
                } else if(block->type == PATH_BLOCK_DOWN) { // dolů
                    color = BLUE;
                } else if(block->type == PATH_BLOCK_RIGHT) { // vpravo
                    color = YELLOW;
                } else {
                    color = BLACK;
                }
            }
            color = Fade(color, 0.25f);

            DrawCubeV({position.x, grid->scan_Height, position.y}, {tile_Size, 0.5f, tile_Size}, color);
            if(block->type >= 0) {
                DrawModelEx(arrow, {position.x, grid->scan_Height, position.y}, {0.f, 1.f, 0.f}, block->type * 90, Vector3One(), WHITE);
            }
            y++;
        }
        x++;
    }
}

void Path_Try_Moving(Path_Grid *grid) {
    Vector2 current_Position = path_Todo_List.front();
    Path_Block *current_Block = Path_Grid_Access(grid, current_Position.x, current_Position.y);
    path_Todo_List.pop_front();

    int block_Direction = current_Block->type;
    for(int direction = 0; direction < 4; direction++) {
        Vector2 next_Position = {current_Position.x + cosf((direction * 90 + 180) * DEG2RAD), current_Position.y + sinf(direction * 90 * DEG2RAD)};
        Path_Block *next_Block = Path_Grid_Access(grid, next_Position.x, next_Position.y);
        if(next_Block->type == PATH_BLOCK_TO_CALCULATE) {
            next_Block->type = direction;
            path_Todo_List.push_back(next_Position);
        }
    }
}

// Získat kolize s střelou (pouze mapa)
RayCollision Get_Collision_Ray(Ray ray, std::vector<BoundingBox> bboxes) {
    RayCollision collision = { 0 };
    collision.distance = 1000000.f;
    for (int m = 0; m < bboxes.size(); m++)
    {
        RayCollision houseCollision = GetRayCollisionBox(ray, bboxes[m]);

        if (houseCollision.hit)
        {
            if(houseCollision.distance < collision.distance) {
                collision = houseCollision;
            }
        }
    }

    return collision;
}

void Path_Find(Path_Grid *grid, Vector2 player_Position, std::vector<BoundingBox> bboxes) {
    path_Todo_List.clear();
    for(int x = 0; x < grid->size; x++) {
        for(int y = 0; y < grid->size; y++) {
            if(Path_Grid_Access(grid, x, y)->type != PATH_BLOCK_WALL) {
                Path_Grid_Access(grid, x, y)->type = PATH_BLOCK_TO_CALCULATE; // ?
            }
        }
    }

    Path_Block *player_Block = Path_Grid_Access(grid, player_Position.x, player_Position.y);
    path_Target = player_Position;
    
    if(player_Block->type == PATH_BLOCK_TO_CALCULATE)
        player_Block->type = PATH_BLOCK_UP;

    path_Todo_List.push_back(player_Position);

    while(!path_Todo_List.empty()) {
        Path_Try_Moving(grid);
    }
}

void Path_Update_Father(Path_Grid* grid, Vector3 *enemy_Position, float *enemy_Rotation, BoundingBox house_Bbox) {
    Vector2 grid_Position = Path_World_To_Grid(grid, *enemy_Position, house_Bbox);
    grid_Position = {roundf(grid_Position.x), roundf(grid_Position.y)};
    Path_Block *block = Path_Grid_Access(grid, grid_Position.x, grid_Position.y);

    Vector3 old_Position = *enemy_Position;
    
    Vector2 old_Grid_Position = Path_World_To_Grid(grid, old_Position, house_Bbox);
    old_Grid_Position = {roundf(old_Grid_Position.x), roundf(old_Grid_Position.y)};

    float old_Rotation = *enemy_Rotation;

    float target_Rotation = block->type * 90;
    float addition = (((((int)target_Rotation - (int)*enemy_Rotation) % 360) + 540) % 360) - 180;
    *enemy_Rotation += addition / 10.f;
    *enemy_Position = Vector3Add(*enemy_Position, {cosf(*enemy_Rotation * DEG2RAD) * GetFrameTime() * 5.f, 0.f, -sinf(*enemy_Rotation * DEG2RAD) * GetFrameTime() * 5.f});

    grid_Position = Path_World_To_Grid(grid, *enemy_Position, house_Bbox);
    grid_Position = {roundf(grid_Position.x), roundf(grid_Position.y)};

    if(Path_Grid_Access(grid, grid_Position.x, old_Grid_Position.y)->type < 0) {
        enemy_Position->x = old_Position.x;
    }

    if(Path_Grid_Access(grid, old_Grid_Position.x, grid_Position.y)->type < 0) {
        enemy_Position->y = old_Position.y;
    }
}

/* STARÉ VYKRESLOVÁNÍ
void Draw_Path(Vector3 position, BoundingBox house_Bbox) {
    Vector2 house_Size = {fabs(house_Bbox.min.x - house_Bbox.max.x), fabs(house_Bbox.min.z - house_Bbox.max.z)};
    float map_Size = std::max(house_Size.x, house_Size.y);
    
    float tile_Size = 10.f;
    Vector2 offset = {-(tile_Size * path_Grid_Size) / 2.f + GetScreenWidth() / 2.f, -(tile_Size * path_Grid_Size) / 2.f + GetScreenHeight() / 2.f};
    for(int x = 0; x < path_Grid_Size; x++) {
        for(int y = 0; y < path_Grid_Size; y++) {
            Rectangle rectangle = {offset.x + x * tile_Size, offset.y + y * tile_Size, tile_Size, tile_Size};
            Path_Block *block = Path_Grid(x, y);
            DrawRectangleRec(rectangle, block->type == 0 ? BLACK : RED);
            DrawRectangleLinesEx(rectangle, 1.f, WHITE);
        }
    }

    Vector2 player_Position = {Remap(position.x, -map_Size / 2.f, map_Size / 2.f, -map_Size / 2.f * tile_Size, map_Size / 2.f * tile_Size), Remap(position.z, -map_Size / 2.f, map_Size / 2.f, -map_Size / 2.f * tile_Size, map_Size / 2.f * tile_Size)};
    Rectangle player = {player_Position.x - tile_Size / 2.f + GetScreenWidth() / 2.f, player_Position.y - tile_Size / 2.f + GetScreenHeight() / 2.f, tile_Size, tile_Size};
    DrawRectangleRec(player, WHITE);
}
*/

#endif