#ifndef MULTIPLAYER_CPP
#define MULTIPLAYER_CPP

#include <raylib.h>

#if defined(_WIN32)
// To avoid conflicting windows.h symbols with raylib, some flags are defined
// WARNING: Those flags avoid inclusion of some Win32 headers that could be required
// by user at some point and won't be included...
//-------------------------------------------------------------------------------------

// If defined, the following flags inhibit definition of the indicated items.
#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT        // DrawText() and DT_*
#define NOGDI             // All GDI defines and routines
#define NOKERNEL          // All KERNEL defines and routines
#define NOUSER            // All USER defines and routines
#define NONLS             // All NLS defines and routines
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions
#define _INC_MMSYSTEM

// Type required before windows.h inclusion
typedef struct tagMSG *LPMSG;

#include <enet/enet.h>

// Type required by some unused function...
typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

#include <objbase.h>
#include <mmreg.h>
#include <mmsystem.h>

// Some required types defined for MSVC/TinyC compiler
#if defined(_MSC_VER) || defined(__TINYC__)
    #include "propidl.h"
#endif

#undef near
#undef far

#else

#include <enet/enet.h>

#endif

#ifdef __WIN32__
#include <mingw.thread.h>
#else
#include <thread>
#endif

#include <vector>
#include <string>

#define UPDATE_RATE 100
#define PRIBINACEK_PORT 4263

namespace Multiplayer {
    bool enet_Initialised = true;
    bool connected = false;

    bool game_Started = false;

    ENetPeer *peer;
    ENetHost *client;

    ENetEvent event;

    // ----------------- Pomocné funkce -----------------
    void Send_Packet(const char* command) {
        ENetPacket *packet = enet_packet_create(command, 
                                                TextLength(command) + 1, 
                                                ENET_PACKET_FLAG_RELIABLE);
        
        enet_peer_send(peer, 0, packet);
        
        // enet_host_flush(client);
    }

    void Broadcast_Packet(const char* command) {
        ENetPacket *packet = enet_packet_create(command, 
                                                TextLength(command) + 1, 
                                                ENET_PACKET_FLAG_RELIABLE);
        
        enet_host_broadcast(client, 0, packet);
        
        // enet_host_flush(client);
    }

    void Poll_Messages() {
        while (enet_host_service(client, &event, UPDATE_RATE) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE: {
                    
                    break;
                }
            }
        }
    }

    char* Wait_For_Response() {
        while(true) {
            while (enet_host_service(client, &event, UPDATE_RATE) > 0) {
                switch (event.type) {
                    case ENET_EVENT_TYPE_RECEIVE: {
                        if(TextIsEqual((char*)event.packet->data, "START")) {
                            game_Started = true;
                            Switch_To_Scene(GAME);
                        }
                        return (char*)event.packet->data;
                    }
                }
            }
        }
    }

    // ----------------- Core server funkce -----------------
    class Player {
    public:
        std::string name;

        Vector3 position;
        Vector3 target;

        /*
        Vector3 old_Position;
        int animation_Tick;
        */

        Player() {}
    };

    std::vector<Player> players = {};

    int String_Split(const char *txt, char delim, char ***tokens) {
        int *tklen, *t, count = 1;
        char **arr, *p = (char *) txt;

        while (*p != '\0') if (*p++ == delim) count += 1;
        t = tklen = (int *)calloc (count, sizeof (int));
        for (p = (char *) txt; *p != '\0'; p++) *p == delim ? *t++ : (*t)++;
        *tokens = arr = (char **)malloc (count * sizeof (char *));
        t = tklen;
        p = *arr++ = (char *)calloc (*(t++) + 1, sizeof (char *));
        while (*txt != '\0')
        {
            if (*txt == delim)
            {
                p = *arr++ = (char *)calloc (*(t++) + 1, sizeof (char *));
                txt++;
            }
            else *p++ = *txt++;
        }
        free (tklen);
        return count;
    }

    void Parse_Player_Tick(char* raw) {
        char** lines;
        int count = String_Split(raw, '\n', &lines);

        players.clear();

        for(int player = 0; player < count; player++) {
            char** tokens;
            int token_Count = String_Split(lines[player], ' ', &tokens);

            if(token_Count == 7) {
                players.push_back(Player());

                players.back().name = tokens[0];

                players.back().position.x = atof(tokens[1]);
                players.back().position.y = atof(tokens[2]);
                players.back().position.z = atof(tokens[3]);

                players.back().target.x = atof(tokens[4]);
                players.back().target.y = atof(tokens[5]);
                players.back().target.z = atof(tokens[6]);
            }

            for(int token = 0; token < token_Count; token++) free(tokens[token]);
            free(tokens);
        }

        for(int line = 0; line < count; line++) free(lines[line]);
        free(lines);
    }
    
    Vector3 player_Position;
    Vector3 player_Target;

    void Tick() {
        Send_Packet(TextFormat("TICK %f %f %f %f %f %f", player_Position.x, player_Position.y, player_Position.z,
                                                         player_Target.x, player_Target.y, player_Target.z));
        char* raw = Wait_For_Response();
        
        Parse_Player_Tick(raw);
    }

    std::thread listening_Thread;
    bool listening = false;

    void Listen() {
        while(listening) {
            Tick();
            WaitTime(UPDATE_RATE / 1000.f);
        }
    }

    void Start_Listening() {
        listening_Thread = std::thread(Listen);
        listening_Thread.detach();
        listening = true;
    }

    void Join_Room(const char* room_Id) {
        Send_Packet(TextFormat("CONNECT %s", room_Id));

        Start_Listening();
        game_Started = false;
    }

    const char* Create_Room() {
        Send_Packet("CREATE");
        const char* response = Wait_For_Response();
        
        Start_Listening();
        game_Started = false;

        return response;
    }

    void Disconnect_Room() {
        Send_Packet("DISCONNECT");

        listening = false;
        listening_Thread.join();
    }

    const char* my_Name = "";

    void Set_Name(const char* name) {
        Send_Packet(TextFormat("SET-NAME %s", name));
        my_Name = name;
    }

    void Set_Data(Vector3 position,
                  Vector3 target) {
        player_Position = position;
        player_Target = target;
    }

    void Pickup(int item_Id) {
        Send_Packet(TextFormat("PICKUP %d", item_Id));
    }

    void Placedown() {
        Send_Packet("PLACEDOWN");
    }

    void Start_Game() {
        Broadcast_Packet("START");
        game_Started = true;
    }

    // ----------------- Funkce použity vnějším kódem -----------------

    void Disconnect() {
        ENetEvent event;
    
        enet_peer_disconnect(peer, 0);

        while (enet_host_service(client, & event, 3000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy (event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                enet_host_destroy(client);
                exit(0);
            }
        }

        enet_peer_reset(peer);
        
        enet_host_destroy(client);

        if(listening)
            listening_Thread.join();

        connected = false;
    }

    Model player_Model;

    int player_Animation_Count = 0;
    ModelAnimation *player_Animations;

    void Init() {
        player_Model = LoadModel(ASSETS_ROOT "models/player.iqm");

        Texture player_Texture = LoadTexture(ASSETS_ROOT "textures/player.png");
        SetMaterialTexture(&player_Model.materials[0], MATERIAL_MAP_DIFFUSE, player_Texture);

        player_Animations = LoadModelAnimations(ASSETS_ROOT "models/player.iqm", &player_Animation_Count);

        if(enet_initialize() != 0) {
            TraceLog(LOG_ERROR, "enet_initialize() error");
            enet_Initialised = false;
        }
        atexit(enet_deinitialize);
    }

    #define MAX_SERVERS 2

    const char* servers[MAX_SERVERS] = {
        "127.0.0.1"
    };

    bool Try_Connect(const char* server) {
        ENetAddress address;
        
        enet_address_set_host (&address, server);
        address.port = PRIBINACEK_PORT;
        
        peer = enet_host_connect(client, &address, 1, 0);    
        
        if(!peer) {
            enet_Initialised = false;
            return false;
        }
        
        if (enet_host_service (client, & event, 200) > 0 &&
            event.type == ENET_EVENT_TYPE_CONNECT) {
            TraceLog(LOG_INFO, "Connected to server %s", server);
            return true;
        } else {
            enet_peer_reset(peer);
        
            enet_Initialised = false;
            return false;
        }
    }

    void Connect() {
        client = enet_host_create(NULL, 1, 1, 0, 0);
        
        if(!client) {
            TraceLog(LOG_ERROR, "enet_host_create() error");
            enet_Initialised = false;
        }

        for(const char* server : servers) {
            bool success = Try_Connect(server);
            if(success) {
                connected = true;
                break;
            }
        }

        if(!connected)
            TraceLog(LOG_INFO, "No server online");
    }

    void Update_3D() {
        for(Player &player : players) {
            bool myself = TextIsEqual(player.name.c_str(), my_Name);
            if(!myself) {
                Vector3 difference = Vector3Subtract(player.target, player.position);
                float rotation = (-atan2(difference.z, difference.x) * RAD2DEG) + 90.f;

                DrawModelEx(player_Model, player.position, {0.f, 1.f, 0.f}, rotation, Vector3One(), WHITE);
            }
        }
    }
};

#endif // MULTIPLAYER_CPP