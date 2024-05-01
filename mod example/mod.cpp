#include <raylib.h>

#ifdef __WIN32__
#define MOD_API __declspec(dllexport)
#else
#define MOD_API
#endif

extern "C" {

MOD_API void Init() {}
MOD_API void Update_2D() {
    DrawText("Hello from the Update_2D function", 5, 5, 35, RED);
}
MOD_API void Update_3D() {
    DrawCube({0.f, 7.5f, 0.f}, 2.f, 2.f, 2.f, RED);
}

}