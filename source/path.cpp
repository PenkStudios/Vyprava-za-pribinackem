#ifndef PATH_CPP
#define PATH_CPP

#include <stdlib.h>
#include <math.h>
#include <raylib.h>

#include <vector>

class Path_Block {
public:
    int type;
    // -1 - empty
    // 0 - wall
    // 1 - up
    // 2 - left
    // 3 - down
    // 4 - right

    Path_Block(int type) : type(type) {}
};

int path_Grid_Size;
Path_Block **path_Grid;

Path_Block *Path_Grid(int x, int y) {
    return &path_Grid[x][y];
}

bool Map_Collide(std::vector<BoundingBox> map, BoundingBox target) {
    for(BoundingBox bbox : map) {
        if(CheckCollisionBoxes(bbox, target))
            return true;
    }
    return false;
}

void Init_Path(int size, std::vector<BoundingBox> bboxes, BoundingBox house_Bbox) {
    path_Grid_Size = size;

    path_Grid = (Path_Block**)malloc(size*sizeof(Path_Block *));
    for(int index = 0; index < size; index++) path_Grid[index] = (Path_Block*)malloc(size*sizeof(Path_Block));

    for(int x = 0; x < size; x++) {
        for(int y = 0; y < size; y++) {
            *Path_Grid(x, y) = Path_Block(-1);
        }
    }

    // x, z
    Vector2 house_Size = {fabs(house_Bbox.min.x - house_Bbox.max.x), fabs(house_Bbox.min.z - house_Bbox.max.z)};
    Vector2 tile_Size = {1.f, 1.f}; // house_Size.x / path_Grid_Size, house_Size.y / path_Grid_Size
    float target_Height = 20.f;

    for(int x = 0; x < size; x++) {
        for(int y = 0; y < size; y++) {
            Path_Block *block = Path_Grid(x, y);
            Vector2 position = {house_Bbox.min.x + x * tile_Size.x, house_Bbox.min.z + y * tile_Size.y};
            BoundingBox bbox = {{position.x - tile_Size.x / 2.f, target_Height - 1.f, position.y - tile_Size.y / 2.f}, {position.x + tile_Size.x / 2.f, target_Height + 1.f, position.y + tile_Size.y / 2.f}};

            if(Map_Collide(bboxes, bbox)) {
                block->type = 0;
            }
        }
    }
}

void Draw_Path(Vector3 position, BoundingBox house_Bbox) {
    float size = 500.f;
    Vector2 offset = {GetScreenWidth() / 2.f - size / 2.f, GetScreenHeight() / 2.f - size / 2.f};
    Vector2 tile_Size = {size / path_Grid_Size, size / path_Grid_Size};
    for(int x = 0; x < path_Grid_Size; x++) {
        for(int y = 0; y < path_Grid_Size; y++) {
            Rectangle rectangle = {offset.x + x * tile_Size.x, offset.y + y * tile_Size.y, tile_Size.x, tile_Size.y};
            Path_Block *block = Path_Grid(x, y);

            DrawRectangleRec(rectangle, block->type == -1 ? BLACK : RED);
            DrawRectangleLinesEx(rectangle, 1.f, WHITE);
        }
    }

    Vector2 house_Size = {fabs(house_Bbox.min.x - house_Bbox.max.x), fabs(house_Bbox.min.z - house_Bbox.max.z)};
    Rectangle player = {position.x / house_Size.x * size, position.x / house_Size.y * size, tile_Size.x, tile_Size.y};
    player.x += GetScreenWidth() / 2.f;
    player.y += GetScreenHeight() / 2.f;
    DrawRectangleRec(player, GREEN);
    DrawRectangleLinesEx(player, 1.f, WHITE);
}

#endif