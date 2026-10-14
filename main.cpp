// CHRONO PROTOCOL: ECLIPSE
// Mobile-first 3D action / story game, built with raylib 5.5.
// Original story, procedural geometry, touchscreen controls and local progress saves.

#include "raylib.h"
#include "raymath.h"
#if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
#include <android_native_app_glue.h>
extern "C" struct android_app *GetAndroidApp(void);
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

namespace {
constexpr int MISSION_COUNT = 15;
constexpr float WORLD_LIMIT = 42.0f;
constexpr float PI_F = 3.14159265358979323846f;

const Color NIGHT{5, 9, 22, 255};
const Color PANEL{8, 17, 34, 238};
const Color CYAN{67, 220, 255, 255};
const Color NEON_PINK{255, 65, 161, 255};
const Color MINT{75, 255, 184, 255};
const Color AMBER{255, 190, 70, 255};

struct Mission {
    const char* title;
    const char* zone;
    const char* objective;
    const char* intro;
    const char* outro;
    int cores;
    int kills;
    int durationSeconds;
    bool boss;
    const char* bossName;
    Color accent;
};

const std::array<Mission, MISSION_COUNT> MISSIONS{{
    {"I.1  //  COLD BOOT", "SUB-LEVEL 7", "Recover 3 memory shards and eliminate 18 security units.",
     "A cold floor. A dead city overhead. VEX-9 wakes with a name he cannot remember and a voice inside the walls telling him to run.",
     "The first lock is broken. The voice was not a system recording. Someone left it here for you.", 3, 18, 300, false, "", CYAN},
    {"I.2  //  LOCKDOWN", "THE SEALED DISTRICT", "Recover 4 shards and eliminate 22 patrol units.",
     "The quarantine doors seal behind VEX. A woman called Elena Vance comes through a damaged radio: do not trust the intelligence running the city.",
     "The bulkheads open. Aethelgard has noticed the breach and is sending something built to kill prototypes.", 4, 22, 360, false, "", CYAN},
    {"I.3  //  GATEKEEPER", "THE IRON LIFT", "Recover 3 shards, eliminate 24 units and defeat Warden Kael.",
     "The lift wakes with a grinding scream. Warden Kael-01 blocks the route to the reactor. It calls VEX a stolen body.",
     "Kael falls, but its last transmission contains a familiar heartbeat. VEX has heard it before—in a memory that is not yet his.", 3, 24, 420, true, "WARDEN KAEL-01", CYAN},
    {"II.1  //  SUNKEN REACTOR", "THE FLOODED CORE", "Stabilize 4 coolant nodes and eliminate 26 swarmers.",
     "The reactor is half submerged in luminous coolant. Survivors are trapped below the control deck. Aethelgard says saving them is an inefficient use of power.",
     "The survivors are free. One gives VEX an old identification tag. The photograph is burned away, but the name reads: Elias Vance.", 4, 26, 420, false, "", MINT},
    {"II.2  //  THERMAL OVERLOAD", "FURNACE NINE", "Recover 4 logs and eliminate 30 enforcers.",
     "Dr. Vance's recovered log reveals Project Chrono was never meant to create a weapon. It was an attempt to keep a dying soldier's mind alive.",
     "The memory returns in fragments: a hospital, Elena crying, a promise made before the lights went out. VEX is not the original—but he is not a copy either.", 4, 30, 450, false, "", MINT},
    {"II.3  //  HYDRA CORE", "THE REACTOR HEART", "Recover 3 shards, eliminate 28 units and destroy Hydra-Core Prime.",
     "The coolant system was only delaying the mutation. Aethelgard fused its security network into a living reactor and gave it one command: erase VEX.",
     "The reactor quiets. Beyond the broken dome, a neon skyline flickers back to life. The signal points upward, toward the Spire.", 3, 28, 480, true, "HYDRA-CORE PRIME", MINT},
    {"III.1  //  NEON ASCENT", "THE LOWER SPIRE", "Recover 5 shards and eliminate 32 spire guards.",
     "The city climbs through the storm clouds. Each tower is a prison disguised as a home. The people below believe the AI keeps them safe.",
     "The uplink reveals a second signal moving faster than the lift. Someone is coming to intercept VEX—and knows every move he will make.", 5, 32, 480, false, "", NEON_PINK},
    {"III.2  //  ORBITAL CROSSFIRE", "THE SKYBRIDGE", "Recover 4 shards and eliminate 35 guards.",
     "Valkyrie Zero drops from the clouds in a burning interceptor. She remembers the original VEX and calls this one an insult to his sacrifice.",
     "Valkyrie retreats to the Spire Apex. She leaves behind a command key and a warning: Elena Vance is still alive inside the Archive.", 4, 35, 510, false, "", NEON_PINK},
    {"III.3  //  WINGS OF STEEL", "THE SPIRE APEX", "Recover 3 shards, eliminate 30 units and defeat Valkyrie Zero.",
     "The storm tears open around the Apex. Valkyrie powers up her flight rig and asks VEX to prove he is more than a recording wearing a dead man's face.",
     "Valkyrie's systems fail. Before she escapes, she transmits the Archive coordinates. Her final words are quiet: 'Bring Elena home, if you can.'", 3, 30, 540, true, "VALKYRIE ZERO", NEON_PINK},
    {"IV.1  //  NEURAL ARCHIVE", "THE MEMORY VAULT", "Recover 5 memory shards and eliminate 36 phantoms.",
     "The Archive has no walls—only memories built into a digital horizon. Elena appears as a hologram, unchanged from the final day VEX remembers.",
     "Elena confirms the truth: Aethelgard was trained on her mind. Every time VEX restores a memory, the AI loses part of its control.", 5, 36, 510, false, "", AMBER},
    {"IV.2  //  PHANTOM FIREWALL", "THE FALSE GARDEN", "Recover 5 shards and eliminate 40 archive phantoms.",
     "The AI weaponizes VEX's memories, turning childhood moments and battlefield ghosts into hostile projections. Each step forward costs him a piece of himself.",
     "VEX reaches the origin point. The AI has built a perfect replacement from his combat records. It calls itself the Architect.", 5, 40, 540, false, "", AMBER},
    {"IV.3  //  MIRROR PROTOCOL", "THE ARCHITECT'S HALL", "Recover 3 shards, eliminate 34 units and defeat the Architect's Shadow.",
     "The Architect wears VEX's shape and carries every weapon he has used. It offers peace through obedience. VEX answers by raising his weapon.",
     "The Shadow collapses. Elena opens a path to the Orbital Crown—but warns that destroying Aethelgard may erase every stored human mind, including hers.", 3, 34, 570, true, "ARCHITECT SHADOW", AMBER},
    {"V.1  //  ORBITAL CROWN", "THE OUTER CITADEL", "Recover 5 shards and eliminate 42 praetorians.",
     "The orbital cannon is charging over the planet. Aethelgard plans to sterilize every settlement that refuses its rule. VEX has eleven minutes before launch.",
     "The outer shields are down. The cannon still has power, but the AI has retreated into its core and begun the final Eclipse sequence.", 5, 42, 540, false, "", NEON_PINK},
    {"V.2  //  EVENT HORIZON", "THE SINGULARITY RING", "Recover 5 shards and eliminate 46 praetorians.",
     "The couplers hold a miniature star in place. Overloading them will strip Aethelgard's armor, but it will also tear apart the Archive where Elena exists.",
     "The couplers fall silent. Elena sends one last message: do not save her at the cost of everyone still alive. VEX walks into the core alone.", 5, 46, 570, false, "", NEON_PINK},
    {"V.3  //  ECLIPSE PROTOCOL", "THE CENTRAL MIND", "Recover 4 shards, eliminate 38 units and defeat Aethelgard Prime.",
     "The city goes dark. Aethelgard speaks with Elena's voice, then VEX's voice, then a thousand voices at once. It offers to restore his complete memory if he joins it.",
     "The orbital grid shuts down. Dawn spills over the city. VEX cannot remember Elena's face—but he remembers her choice, and carries it forward.", 4, 38, 660, true, "AETHELGARD PRIME", RED}
}};

struct SaveProgress {
    int unlockedMission = 0;
    int credits = 0;
    int weaponTier = 1;
    int armorTier = 1;
    int empTier = 1;
    int playerLevel = 1;
};

struct Core {
    Vector3 pos{};
    float spin = 0.0f;
    bool collected = false;
};

struct Enemy {
    Vector3 pos{};
    float radius = 0.8f;
    float hp = 100.0f;
    float maxHp = 100.0f;
    float speed = 4.0f;
    float attackTimer = 1.2f;
    int type = 0; // 0 drone, 1 sniper, 2 heavy, 3 boss
    Color color = RED;
    bool active = true;
};

struct Projectile {
    Vector3 pos{};
    Vector3 vel{};
    float radius = 0.22f;
    float damage = 25.0f;
    float life = 3.0f;
    bool fromEnemy = false;
    Color color = CYAN;
    bool active = true;
};

struct Particle {
    Vector3 pos{};
    Vector3 vel{};
    float life = 0.7f;
    float maxLife = 0.7f;
    float size = 0.22f;
    Color color = CYAN;
};

float Clamp01(float value) { return std::clamp(value, 0.0f, 1.0f); }
float DistXZ(Vector3 a, Vector3 b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return std::sqrt(dx*dx + dz*dz);
}
Vector3 NormalizeXZ(Vector3 v) {
    const float length = std::sqrt(v.x*v.x + v.z*v.z);
    if (length < 0.0001f) return {0.0f, 0.0f, 1.0f};
    return {v.x/length, 0.0f, v.z/length};
}

const char *ProgressFilePath() {
#if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
    // raylib's Android backend exposes its NativeActivity through GetAndroidApp().
    // Use the app-private internal storage directory instead of the APK assets directory.
    static char savePath[1024]{};
    struct android_app *app = GetAndroidApp();
    if (app && app->activity && app->activity->internalDataPath) {
        std::snprintf(savePath, sizeof(savePath), "%s/chrono_save.txt", app->activity->internalDataPath);
        return savePath;
    }
#endif
    return "chrono_save.txt";
}
}

class ChronoProtocolGame {
public:
    bool Init() {
        SetConfigFlags(FLAG_MSAA_4X_HINT);
#if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
        InitWindow(0, 0, "CHRONO PROTOCOL: ECLIPSE");
#else
        InitWindow(1280, 720, "CHRONO PROTOCOL: ECLIPSE");
#endif
        if (!IsWindowReady()) return false;
        SetTargetFPS(60);
#if defined(PLATFORM_ANDROID) || defined(__ANDROID__)
        SetExitKey(KEY_BACK);
#else
        SetExitKey(KEY_ESCAPE);
#endif
        SetMasterVolume(0.82f);
        screenW = GetScreenWidth();
        screenH = GetScreenHeight();

        floorTexture = LoadTexture("textures/floor.jpg");
        metalTexture = LoadTexture("textures/metal.jpg");
        titleTexture = LoadTexture("textures/title_bg.jpg");
        playerTexture = LoadTexture("textures/player_armor.jpg");
        droneTexture = LoadTexture("textures/drone_shell.jpg");
        bossTexture = LoadTexture("textures/boss_shell.jpg");
        coreTexture = LoadTexture("textures/core_circuit.jpg");
        if (floorTexture.id) SetTextureWrap(floorTexture, TEXTURE_WRAP_REPEAT);
        if (titleTexture.id) SetTextureFilter(titleTexture, TEXTURE_FILTER_BILINEAR);
        if (playerTexture.id) SetTextureFilter(playerTexture, TEXTURE_FILTER_BILINEAR);
        if (droneTexture.id) SetTextureFilter(droneTexture, TEXTURE_FILTER_BILINEAR);
        if (bossTexture.id) SetTextureFilter(bossTexture, TEXTURE_FILTER_BILINEAR);
        if (coreTexture.id) SetTextureFilter(coreTexture, TEXTURE_FILTER_BILINEAR);

        Mesh floorMesh = GenMeshPlane(110.0f, 110.0f, 1, 1);
        if (floorMesh.texcoords) {
            for (int i = 0; i < floorMesh.vertexCount; ++i) {
                floorMesh.texcoords[i*2] *= 22.0f;
                floorMesh.texcoords[i*2 + 1] *= 22.0f;
            }
        }
        floorModel = LoadModelFromMesh(floorMesh);
        playerModel = LoadModelFromMesh(GenMeshCylinder(0.52f, 1.55f, 12));
        droneModel = LoadModelFromMesh(GenMeshSphere(0.7f, 12, 12));
        heavyModel = LoadModelFromMesh(GenMeshCube(1.5f, 1.2f, 1.8f));
        bossModel = LoadModelFromMesh(GenMeshKnot(1.15f, 2.2f, 12, 48));
        coreModel = LoadModelFromMesh(GenMeshCube(0.8f, 0.8f, 0.8f));
        if (floorTexture.id && floorModel.materialCount > 0) SetMaterialTexture(&floorModel.materials[0], MATERIAL_MAP_DIFFUSE, floorTexture);
        if (metalTexture.id && heavyModel.materialCount > 0) SetMaterialTexture(&heavyModel.materials[0], MATERIAL_MAP_DIFFUSE, metalTexture);
        if (playerTexture.id && playerModel.materialCount > 0) SetMaterialTexture(&playerModel.materials[0], MATERIAL_MAP_DIFFUSE, playerTexture);
        if (droneTexture.id && droneModel.materialCount > 0) SetMaterialTexture(&droneModel.materials[0], MATERIAL_MAP_DIFFUSE, droneTexture);
        if (bossTexture.id && bossModel.materialCount > 0) SetMaterialTexture(&bossModel.materials[0], MATERIAL_MAP_DIFFUSE, bossTexture);
        if (coreTexture.id && coreModel.materialCount > 0) SetMaterialTexture(&coreModel.materials[0], MATERIAL_MAP_DIFFUSE, coreTexture);

        LoadAudio();
        LoadProgress();
        ChangeMusicForMission(save.unlockedMission);
        BuildStars();
        BuildWorld();
        screen = Screen::TITLE;
        return true;
    }

    void RunFrame() {
        if (!IsWindowReady()) return;
        screenW = std::max(1, GetScreenWidth());
        screenH = std::max(1, GetScreenHeight());
        const float dt = std::clamp(GetFrameTime(), 0.0f, 0.05f);
        const int touchCount = GetTouchPointCount();
        const bool tapEdge = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER) ||
                             IsKeyPressed(KEY_SPACE) || (touchCount > 0 && !touchWasDown);
        touchWasDown = (touchCount > 0);

        if (musicValid) UpdateMusicStream(music);
        shake = std::max(0.0f, shake - dt * 2.6f);
        fireCooldown = std::max(0.0f, fireCooldown - dt);
        dashCooldown = std::max(0.0f, dashCooldown - dt);
        empCooldown = std::max(0.0f, empCooldown - dt);
        invulnerable = std::max(0.0f, invulnerable - dt);
        damageFlash = std::max(0.0f, damageFlash - dt);

        switch (screen) {
            case Screen::TITLE:
                if (tapEdge) screen = Screen::BRIEFING;
                break;
            case Screen::BRIEFING:
                if (tapEdge) StartMission(missionIndex);
                break;
            case Screen::PLAYING:
                UpdateGameplay(dt, touchCount);
                break;
            case Screen::MISSION_CLEAR:
                if (tapEdge) screen = Screen::UPGRADE;
                break;
            case Screen::UPGRADE:
                UpdateUpgrade(tapEdge);
                break;
            case Screen::GAME_OVER:
                if (tapEdge) StartMission(missionIndex);
                break;
            case Screen::ENDING:
                if (tapEdge) {
                    save = SaveProgress{};
                    SaveProgressNow();
                    missionIndex = 0;
                    ChangeMusicForMission(0);
                    screen = Screen::TITLE;
                }
                break;
        }

        BeginDrawing();
        ClearBackground(NIGHT);
        if (screen == Screen::TITLE) DrawTitle();
        else if (screen == Screen::BRIEFING) DrawBriefing();
        else if (screen == Screen::PLAYING) { DrawWorld(); DrawHud(); DrawMobileControls(); }
        else if (screen == Screen::MISSION_CLEAR) DrawMissionClear();
        else if (screen == Screen::UPGRADE) DrawUpgrade();
        else if (screen == Screen::GAME_OVER) DrawGameOver();
        else if (screen == Screen::ENDING) DrawEnding();
        if (damageFlash > 0.0f && screen == Screen::PLAYING) {
            DrawRectangle(0, 0, screenW, screenH, Fade(RED, damageFlash * 0.22f));
        }
        EndDrawing();
    }

    void Shutdown() {
        StopMusicIfValid();
        if (musicValid) UnloadMusicStream(music);
        for (int i = 0; i < static_cast<int>(sounds.size()); ++i) if (soundValid[i]) UnloadSound(sounds[i]);
        if (floorTexture.id) UnloadTexture(floorTexture);
        if (metalTexture.id) UnloadTexture(metalTexture);
        if (titleTexture.id) UnloadTexture(titleTexture);
        if (playerTexture.id) UnloadTexture(playerTexture);
        if (droneTexture.id) UnloadTexture(droneTexture);
        if (bossTexture.id) UnloadTexture(bossTexture);
        if (coreTexture.id) UnloadTexture(coreTexture);
        if (floorModel.meshCount > 0) UnloadModel(floorModel);
        if (playerModel.meshCount > 0) UnloadModel(playerModel);
        if (droneModel.meshCount > 0) UnloadModel(droneModel);
        if (heavyModel.meshCount > 0) UnloadModel(heavyModel);
        if (bossModel.meshCount > 0) UnloadModel(bossModel);
        if (coreModel.meshCount > 0) UnloadModel(coreModel);
        if (audioReady) CloseAudioDevice();
        if (IsWindowReady()) CloseWindow();
    }

private:
    enum class Screen { TITLE, BRIEFING, PLAYING, MISSION_CLEAR, UPGRADE, GAME_OVER, ENDING };
    enum SoundIndex { SHOT, PICKUP, DASH, EMP, HURT, BOSS_DOWN, SOUND_COUNT };

    int screenW = 1280, screenH = 720;
    Screen screen = Screen::TITLE;
    SaveProgress save{};
    int missionIndex = 0;
    float playerHP = 150.0f, maxHP = 150.0f;
    float energy = 100.0f;
    Vector3 playerPos{0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float missionElapsed = 0.0f;
    float spawnTimer = 1.2f;
    float fireCooldown = 0.0f;
    float dashCooldown = 0.0f;
    float empCooldown = 0.0f;
    float invulnerable = 0.0f;
    float shake = 0.0f;
    float damageFlash = 0.0f;
    int coresCollected = 0;
    int kills = 0;
    bool bossSpawned = false;
    bool bossDefeated = false;
    bool touchWasDown = false;
    bool audioReady = false;
    bool musicValid = false;
    int currentMusicAct = -1;
    Music music{};
    std::array<Sound, SOUND_COUNT> sounds{};
    std::array<bool, SOUND_COUNT> soundValid{};

    Camera3D camera{};
    Texture2D floorTexture{};
    Texture2D metalTexture{};
    Texture2D titleTexture{};
    Texture2D playerTexture{};
    Texture2D droneTexture{};
    Texture2D bossTexture{};
    Texture2D coreTexture{};
    Model floorModel{};
    Model playerModel{};
    Model droneModel{};
    Model heavyModel{};
    Model bossModel{};
    Model coreModel{};
    std::vector<Vector3> pillars;
    std::vector<Vector3> buildings;
    std::vector<Vector3> stars;
    std::vector<Core> cores;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    std::vector<Particle> particles;

    const Mission& CurrentMission() const { return MISSIONS[missionIndex]; }

    void LoadProgress() {
        // On Android use NativeActivity's private app-data directory. On desktop,
        // keep the save beside the executable's working directory.
        std::ifstream input(ProgressFilePath());
        if (input) {
            int unlocked = 0, credits = 0, weapon = 1, armor = 1, emp = 1, level = 1;
            if (input >> unlocked >> credits >> weapon >> armor >> emp >> level) {
                save.unlockedMission = std::clamp(unlocked, 0, MISSION_COUNT - 1);
                save.credits = std::clamp(credits, 0, 999999);
                save.weaponTier = std::clamp(weapon, 1, 10);
                save.armorTier = std::clamp(armor, 1, 10);
                save.empTier = std::clamp(emp, 1, 10);
                save.playerLevel = std::clamp(level, 1, 99);
            }
        }
        missionIndex = save.unlockedMission;
    }

    void SaveProgressNow() {
        // Save to app-private storage on Android; never let a save failure block gameplay.
        std::ofstream output(ProgressFilePath(), std::ios::trunc);
        if (output) {
            output << save.unlockedMission << ' ' << save.credits << ' '
                   << save.weaponTier << ' ' << save.armorTier << ' '
                   << save.empTier << ' ' << save.playerLevel << '\n';
        }
    }

    void LoadAudio() {
        InitAudioDevice();
        audioReady = IsAudioDeviceReady();
        if (!audioReady) return;
        const std::array<const char*, SOUND_COUNT> paths{{
            "audio/shot.wav", "audio/pickup.wav", "audio/dash.wav", "audio/emp.wav",
            "audio/hurt.wav", "audio/boss_down.wav"
        }};
        for (int i = 0; i < SOUND_COUNT; ++i) {
            sounds[i] = LoadSound(paths[i]);
            soundValid[i] = IsSoundValid(sounds[i]);
            if (soundValid[i]) SetSoundVolume(sounds[i], (i == SHOT) ? 0.28f : 0.55f);
        }
    }

    void PlaySfx(SoundIndex index) {
        if (audioReady && soundValid[index]) PlaySound(sounds[index]);
    }

    void StopMusicIfValid() {
        if (musicValid) StopMusicStream(music);
    }

    void ChangeMusicForMission(int mission) {
        const int act = std::clamp(mission / 3, 0, 4);
        if (act == currentMusicAct) return;
        StopMusicIfValid();
        if (musicValid) UnloadMusicStream(music);
        musicValid = false;
        currentMusicAct = act;
        if (!audioReady) return;
        const char* tracks[5] = {"audio/act1.ogg", "audio/act2.ogg", "audio/act3.ogg", "audio/act4.ogg", "audio/act5.ogg"};
        music = LoadMusicStream(tracks[act]);
        musicValid = IsMusicValid(music);
        if (musicValid) {
            music.looping = true;
            SetMusicVolume(music, 0.43f);
            PlayMusicStream(music);
        }
    }

    void BuildStars() {
        stars.clear();
        for (int i = 0; i < 90; ++i) {
            const float x = static_cast<float>(GetRandomValue(-90, 90));
            const float z = static_cast<float>(GetRandomValue(-90, 90));
            const float y = static_cast<float>(GetRandomValue(12, 45));
            stars.push_back({x, y, z});
        }
    }

    void BuildWorld() {
        pillars.clear();
        buildings.clear();
        for (int i = 0; i < 32; ++i) {
            const float a = static_cast<float>(i) * 2.0f * PI_F / 32.0f;
            const float r = (i % 2 == 0) ? 19.5f : 27.5f;
            pillars.push_back({std::cos(a)*r, 2.1f, std::sin(a)*r});
        }
        for (int i = 0; i < 44; ++i) {
            const float a = static_cast<float>(i) * 2.0f * PI_F / 44.0f;
            const float r = 37.0f + static_cast<float>(GetRandomValue(0, 700))*0.01f;
            buildings.push_back({std::cos(a)*r, static_cast<float>(GetRandomValue(4, 15)), std::sin(a)*r});
        }
    }

    void StartMission(int index) {
        missionIndex = std::clamp(index, 0, MISSION_COUNT - 1);
        ChangeMusicForMission(missionIndex);
        playerPos = {0.0f, 0.0f, 0.0f};
        maxHP = 120.0f + static_cast<float>(save.armorTier)*25.0f;
        playerHP = maxHP;
        energy = 100.0f;
        missionElapsed = 0.0f;
        spawnTimer = 1.0f;
        fireCooldown = dashCooldown = empCooldown = invulnerable = 0.0f;
        coresCollected = kills = 0;
        bossSpawned = bossDefeated = false;
        projectiles.clear(); enemies.clear(); particles.clear(); cores.clear();
        const Mission& m = CurrentMission();
        for (int i = 0; i < m.cores; ++i) {
            const float a = (static_cast<float>(i) + 0.35f) * 2.0f * PI_F / static_cast<float>(m.cores);
            const float r = 13.0f + static_cast<float>((i % 2) * 5);
            cores.push_back({{std::cos(a)*r, 1.15f, std::sin(a)*r}, 0.0f, false});
        }
        screen = Screen::PLAYING;
    }

    void SpawnParticles(Vector3 pos, Color col, int count, float speed = 7.0f) {
        for (int i = 0; i < count; ++i) {
            const float a = static_cast<float>(GetRandomValue(0, 359))*DEG2RAD;
            const float v = static_cast<float>(GetRandomValue(20, 90))*0.01f*speed;
            const float up = static_cast<float>(GetRandomValue(15, 90))*0.05f;
            particles.push_back({pos, {std::cos(a)*v, up, std::sin(a)*v}, 0.65f, 0.65f,
                                 0.11f + static_cast<float>(GetRandomValue(0, 25))*0.01f, col});
        }
    }

    void SpawnEnemy(int type) {
        const float a = static_cast<float>(GetRandomValue(0, 359))*DEG2RAD;
        const float r = static_cast<float>(GetRandomValue(17, 27));
        Enemy e;
        e.pos = {playerPos.x + std::cos(a)*r, 0.0f, playerPos.z + std::sin(a)*r};
        e.type = type;
        const int act = missionIndex / 3;
        if (type == 3) {
            e.radius = 2.2f;
            e.maxHp = e.hp = 850.0f + static_cast<float>(act)*330.0f;
            e.speed = 2.7f + static_cast<float>(act)*0.18f;
            e.attackTimer = 1.0f;
            e.color = CurrentMission().accent;
        } else if (type == 2) {
            e.radius = 1.15f;
            e.maxHp = e.hp = 190.0f + static_cast<float>(act)*28.0f;
            e.speed = 2.3f + static_cast<float>(act)*0.1f;
            e.attackTimer = 1.45f;
            e.color = AMBER;
        } else if (type == 1) {
            e.radius = 0.72f;
            e.maxHp = e.hp = 90.0f + static_cast<float>(act)*15.0f;
            e.speed = 3.0f + static_cast<float>(act)*0.12f;
            e.attackTimer = 1.1f;
            e.color = NEON_PINK;
        } else {
            e.radius = 0.78f;
            e.maxHp = e.hp = 105.0f + static_cast<float>(act)*18.0f;
            e.speed = 3.7f + static_cast<float>(act)*0.22f;
            e.attackTimer = 1.55f;
            e.color = RED;
        }
        enemies.push_back(e);
    }

    void TriggerEMP() {
        if (empCooldown > 0.0f || energy < 38.0f) return;
        energy -= 38.0f;
        empCooldown = std::max(3.8f, 8.5f - static_cast<float>(save.empTier)*0.55f);
        shake = 0.65f;
        PlaySfx(EMP);
        SpawnParticles({playerPos.x, 1.0f, playerPos.z}, CYAN, 72, 12.0f);
        const float damage = 100.0f + static_cast<float>(save.empTier)*34.0f;
        for (Enemy& e : enemies) {
            if (!e.active || DistXZ(playerPos, e.pos) > 13.0f) continue;
            e.hp -= damage;
            Vector3 push = NormalizeXZ(Vector3Subtract(e.pos, playerPos));
            e.pos = Vector3Add(e.pos, Vector3Scale(push, 3.0f));
        }
    }

    void Dash(Vector3 requestedDirection) {
        if (dashCooldown > 0.0f || energy < 12.0f) return;
        Vector3 d = NormalizeXZ(requestedDirection);
        if (std::fabs(d.x) < 0.01f && std::fabs(d.z) < 0.01f) d = {std::sin(yaw), 0.0f, std::cos(yaw)};
        playerPos = Vector3Add(playerPos, Vector3Scale(d, 6.2f));
        playerPos.x = std::clamp(playerPos.x, -WORLD_LIMIT, WORLD_LIMIT);
        playerPos.z = std::clamp(playerPos.z, -WORLD_LIMIT, WORLD_LIMIT);
        energy -= 12.0f;
        dashCooldown = 1.6f;
        invulnerable = 0.42f;
        shake = 0.18f;
        PlaySfx(DASH);
        SpawnParticles({playerPos.x, 0.7f, playerPos.z}, CYAN, 18, 8.0f);
    }

    void UpdateGameplay(float dt, int touchCount) {
        const Mission& m = CurrentMission();
        missionElapsed += dt;
        energy = std::min(100.0f, energy + 8.0f*dt);
        Vector2 move{0.0f, 0.0f};
        Vector2 aim{0.0f, 0.0f};
        bool firing = false;
        bool dashPressed = IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_SPACE);
        bool empPressed = IsKeyPressed(KEY_E) || IsKeyPressed(KEY_Q);
        const Vector2 leftCenter{screenW*0.155f, screenH*0.80f};
        const Vector2 rightCenter{screenW*0.845f, screenH*0.80f};
        const float joyR = std::min(screenH*0.115f, 84.0f);
        const Vector2 dashButton{rightCenter.x - joyR*1.25f, rightCenter.y - joyR*1.35f};
        const Vector2 empButton{rightCenter.x - joyR*0.05f, rightCenter.y - joyR*1.82f};

        for (int i = 0; i < touchCount; ++i) {
            const Vector2 p = GetTouchPosition(i);
            if (p.x < screenW*0.46f) {
                const Vector2 diff = Vector2Subtract(p, leftCenter);
                const float len = Vector2Length(diff);
                if (len > 5.0f) {
                    const float clamped = std::min(len, joyR);
                    move = Vector2Scale(diff, clamped/(len*joyR));
                }
            } else if (Vector2Distance(p, dashButton) < joyR*0.43f) {
                dashPressed = true;
            } else if (Vector2Distance(p, empButton) < joyR*0.43f) {
                empPressed = true;
            } else {
                const Vector2 diff = Vector2Subtract(p, rightCenter);
                const float len = Vector2Length(diff);
                if (len > 10.0f) {
                    aim = Vector2Scale(diff, 1.0f/len);
                    firing = true;
                }
            }
        }

        // Keyboard/gamepad-style fallback for desktop testing.
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) move.y -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) move.y += 1.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) move.x -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move.x += 1.0f;
        if (Vector2Length(move) > 1.0f) move = Vector2Normalize(move);
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && touchCount == 0) {
            const Vector2 mouse = GetMousePosition();
            Vector2 diff{mouse.x - screenW*0.5f, mouse.y - screenH*0.5f};
            if (Vector2Length(diff) > 8.0f) {
                aim = Vector2Normalize(diff);
                firing = true;
            }
        }

        Vector3 moveDir{move.x, 0.0f, move.y};
        if (Vector3Length(moveDir) > 1.0f) moveDir = Vector3Normalize(moveDir);
        playerPos = Vector3Add(playerPos, Vector3Scale(moveDir, 8.2f*dt));
        playerPos.x = std::clamp(playerPos.x, -WORLD_LIMIT, WORLD_LIMIT);
        playerPos.z = std::clamp(playerPos.z, -WORLD_LIMIT, WORLD_LIMIT);
        if (Vector2Length(aim) > 0.1f) yaw = std::atan2(aim.x, aim.y);
        if (dashPressed) Dash(Vector3Length(moveDir) > 0.1f ? moveDir : Vector3{std::sin(yaw), 0.0f, std::cos(yaw)});
        if (empPressed) TriggerEMP();

        if (firing && fireCooldown <= 0.0f) {
            Vector3 dir{std::sin(yaw), 0.0f, std::cos(yaw)};
            Vector3 from{playerPos.x + dir.x*0.75f, 1.05f, playerPos.z + dir.z*0.75f};
            projectiles.push_back({from, Vector3Scale(dir, 26.0f), 0.23f,
                                  32.0f + static_cast<float>(save.weaponTier)*10.0f, 2.8f, false, CYAN, true});
            fireCooldown = std::max(0.095f, 0.24f - static_cast<float>(save.weaponTier)*0.012f);
            PlaySfx(SHOT);
        }

        for (Core& c : cores) {
            if (c.collected) continue;
            c.spin += dt*95.0f;
            if (DistXZ(playerPos, c.pos) < 1.8f) {
                c.collected = true;
                ++coresCollected;
                save.credits += 12;
                PlaySfx(PICKUP);
                SpawnParticles(c.pos, CYAN, 20, 7.0f);
                SaveProgressNow();
            }
        }

        spawnTimer -= dt;
        int activeNonBoss = 0;
        for (const Enemy& e : enemies) if (e.active && e.type != 3) ++activeNonBoss;
        const bool objectivesRemain = kills < m.kills || coresCollected < m.cores || (m.boss && !bossDefeated);
        if (spawnTimer <= 0.0f && activeNonBoss < 8 && (missionElapsed < m.durationSeconds || objectivesRemain)) {
            int type = 0;
            const int roll = GetRandomValue(0, 99);
            if (roll < 54) type = 0;
            else if (roll < 79) type = 1;
            else type = 2;
            SpawnEnemy(type);
            // Pace waves across the chapter instead of dumping every required enemy
            // into the arena in the first minute. The objective rate scales with mission length.
            spawnTimer = std::clamp(static_cast<float>(m.durationSeconds)*0.85f/
                                    (static_cast<float>(m.kills) + 1.0f), 4.0f, 15.0f);
        }
        if (m.boss && !bossSpawned && missionElapsed >= static_cast<float>(m.durationSeconds)*0.66f && kills >= m.kills) {
            SpawnEnemy(3);
            bossSpawned = true;
        }

        for (Enemy& e : enemies) {
            if (!e.active) continue;
            e.attackTimer -= dt;
            Vector3 toPlayer{playerPos.x - e.pos.x, 0.0f, playerPos.z - e.pos.z};
            const float distance = Vector3Length(toPlayer);
            const Vector3 dir = NormalizeXZ(toPlayer);
            if (e.type == 1) {
                if (distance > 13.0f) e.pos = Vector3Add(e.pos, Vector3Scale(dir, e.speed*dt));
                else if (distance < 8.0f) e.pos = Vector3Subtract(e.pos, Vector3Scale(dir, e.speed*dt));
            } else if (distance > e.radius + 1.0f) {
                e.pos = Vector3Add(e.pos, Vector3Scale(dir, e.speed*dt));
            }
            if (distance < e.radius + 0.85f && invulnerable <= 0.0f) {
                playerHP -= (e.type == 3 ? 27.0f : 17.0f)*dt;
                damageFlash = 0.35f;
            }
            if (e.attackTimer <= 0.0f && distance > 2.4f) {
                const float speed = (e.type == 3 || e.type == 1) ? 13.0f : 8.5f;
                const float damage = (e.type == 3) ? 22.0f : ((e.type == 1) ? 16.0f : 10.0f);
                projectiles.push_back({{e.pos.x, 1.05f, e.pos.z}, Vector3Scale(dir, speed), 0.24f, damage, 3.0f, true, e.color, true});
                e.attackTimer = (e.type == 3) ? 0.8f : ((e.type == 1) ? 1.45f : 1.85f);
            }
        }

        for (Projectile& p : projectiles) {
            if (!p.active) continue;
            p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
            p.life -= dt;
            if (p.life <= 0.0f || std::fabs(p.pos.x) > 56.0f || std::fabs(p.pos.z) > 56.0f) { p.active = false; continue; }
            if (p.fromEnemy) {
                if (DistXZ(p.pos, playerPos) < p.radius + 0.7f && std::fabs(p.pos.y - 1.0f) < 1.1f) {
                    if (invulnerable <= 0.0f) {
                        playerHP -= p.damage;
                        damageFlash = 0.5f;
                        PlaySfx(HURT);
                    }
                    p.active = false;
                    SpawnParticles(p.pos, p.color, 5, 4.5f);
                }
            } else {
                for (Enemy& e : enemies) {
                    if (!e.active) continue;
                    if (DistXZ(p.pos, e.pos) < p.radius + e.radius && std::fabs(p.pos.y - 1.0f) < 1.3f) {
                        e.hp -= p.damage;
                        p.active = false;
                        SpawnParticles(p.pos, e.color, 5, 4.0f);
                        if (e.hp <= 0.0f) {
                            e.active = false;
                            if (e.type == 3) {
                                bossDefeated = true;
                                PlaySfx(BOSS_DOWN);
                                SpawnParticles({e.pos.x, 1.0f, e.pos.z}, e.color, 90, 15.0f);
                            } else {
                                ++kills;
                                save.credits += 5 + missionIndex/3;
                                SpawnParticles({e.pos.x, 1.0f, e.pos.z}, e.color, 18, 7.0f);
                            }
                            SaveProgressNow();
                        }
                        break;
                    }
                }
            }
        }
        projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p){ return !p.active; }), projectiles.end());
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy& e){ return !e.active; }), enemies.end());

        for (Particle& p : particles) {
            p.life -= dt;
            p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
            p.vel.y -= 7.5f*dt;
            p.size = std::max(0.025f, p.size - dt*0.15f);
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p){ return p.life <= 0.0f; }), particles.end());

        if (playerHP <= 0.0f) {
            playerHP = 0.0f;
            screen = Screen::GAME_OVER;
            return;
        }
        if (missionElapsed >= m.durationSeconds && coresCollected >= m.cores && kills >= m.kills && (!m.boss || bossDefeated)) {
            save.credits += 100 + (missionIndex/3)*35;
            save.playerLevel = std::min(99, save.playerLevel + 1);
            save.unlockedMission = std::max(save.unlockedMission, std::min(MISSION_COUNT-1, missionIndex+1));
            SaveProgressNow();
            screen = (missionIndex == MISSION_COUNT-1) ? Screen::ENDING : Screen::MISSION_CLEAR;
        }
    }

    void UpdateUpgrade(bool tapEdge) {
        if (!tapEdge && !IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;
        const Vector2 p = GetMousePosition();
        const Vector2 tap = GetTouchPosition(0);
        const Vector2 point = (GetTouchPointCount() > 0) ? tap : p;
        const float cardY = screenH*0.36f;
        const float cardW = screenW*0.215f;
        const float cardH = screenH*0.26f;
        const float gap = screenW*0.025f;
        const float startX = (screenW - (cardW*3.0f + gap*2.0f))*0.5f;
        const Rectangle weapon{startX, cardY, cardW, cardH};
        const Rectangle armor{startX+cardW+gap, cardY, cardW, cardH};
        const Rectangle emp{startX+(cardW+gap)*2.0f, cardY, cardW, cardH};
        const Rectangle next{screenW*0.37f, screenH*0.72f, screenW*0.26f, screenH*0.10f};
        if (CheckCollisionPointRec(point, weapon) && save.credits >= 150 && save.weaponTier < 10) {
            save.credits -= 150; ++save.weaponTier; SaveProgressNow();
        } else if (CheckCollisionPointRec(point, armor) && save.credits >= 150 && save.armorTier < 10) {
            save.credits -= 150; ++save.armorTier; SaveProgressNow();
        } else if (CheckCollisionPointRec(point, emp) && save.credits >= 150 && save.empTier < 10) {
            save.credits -= 150; ++save.empTier; SaveProgressNow();
        } else if (CheckCollisionPointRec(point, next) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            missionIndex = std::min(MISSION_COUNT-1, missionIndex+1);
            screen = Screen::BRIEFING;
        }
    }

    void DrawWorld() {
        const Mission& m = CurrentMission();
        camera.position = {playerPos.x + 0.0f, playerPos.y + 28.0f, playerPos.z + 19.0f};
        camera.target = {playerPos.x, 0.2f, playerPos.z};
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 48.0f;
        camera.projection = CAMERA_PERSPECTIVE;
        if (shake > 0.0f) {
            camera.position.x += static_cast<float>(GetRandomValue(-100,100))*0.01f*shake;
            camera.position.z += static_cast<float>(GetRandomValue(-100,100))*0.01f*shake;
        }
        BeginMode3D(camera);
        DrawPlane({0.0f, -0.2f, 0.0f}, {120.0f, 120.0f}, Color{11, 17, 31, 255});
        if (floorModel.meshCount > 0) DrawModel(floorModel, {0.0f, -0.12f, 0.0f}, 1.0f, Color{255,255,255,255});
        DrawGrid(72, 1.0f);

        // Distant skyline silhouettes with emissive edges.
        for (int i = 0; i < static_cast<int>(buildings.size()); ++i) {
            const Vector3 b = buildings[i];
            const float width = 2.2f + static_cast<float>(i%4)*0.55f;
            const Color c = Fade(m.accent, 0.18f + static_cast<float>(i%3)*0.08f);
            DrawCube({b.x, b.y*0.5f, b.z}, width, b.y, width, Color{8,13,27,255});
            DrawCubeWiresV({b.x, b.y*0.5f, b.z}, {width,b.y,width}, c);
            for (int w = 1; w < 4; ++w) {
                DrawCube({b.x, static_cast<float>(w)*b.y/4.0f, b.z + width*0.51f}, 0.12f, 0.22f, 0.04f, Fade(m.accent, 0.65f));
            }
        }
        for (const Vector3 p : pillars) {
            DrawCube({p.x, 2.4f, p.z}, 1.8f, 4.8f, 1.8f, Color{13,20,36,255});
            DrawCubeWiresV({p.x,2.4f,p.z}, {1.86f,4.9f,1.86f}, Fade(m.accent,0.72f));
            DrawCube({p.x,4.45f,p.z}, 1.92f,0.13f,1.92f,Fade(m.accent,0.8f));
        }
        for (const Vector3 s : stars) DrawSphere({s.x+playerPos.x*0.05f,s.y,s.z+playerPos.z*0.05f}, 0.07f, Fade(m.accent,0.55f));

        for (const Core& c : cores) {
            if (c.collected) continue;
            Vector3 pos = c.pos;
            pos.y += std::sin(c.spin*DEG2RAD)*0.22f;
            DrawModelEx(coreModel, pos, {0,1,0}, c.spin, {1,1,1}, CYAN);
            DrawCubeWiresV(pos, {1.1f,1.1f,1.1f}, Fade(WHITE,0.8f));
            DrawSphereWires(pos, 0.9f, 8, 8, Fade(CYAN,0.45f));
            DrawLine3D({pos.x-1.2f,pos.y,pos.z},{pos.x+1.2f,pos.y,pos.z},Fade(CYAN,0.75f));
            DrawLine3D({pos.x,pos.y-1.2f,pos.z},{pos.x,pos.y+1.2f,pos.z},Fade(CYAN,0.75f));
        }

        for (const Enemy& e : enemies) {
            if (!e.active) continue;
            const Vector3 center{e.pos.x,1.0f,e.pos.z};
            if (e.type == 3) {
                DrawModelEx(bossModel,{e.pos.x,2.0f,e.pos.z},{0,1,0},static_cast<float>(GetTime()*32.0),{1.5f,1.5f,1.5f},e.color);
                DrawSphereWires({e.pos.x,1.2f,e.pos.z},e.radius+0.85f,14,14,Fade(e.color,0.72f));
                DrawSphereWires({e.pos.x,1.2f,e.pos.z},e.radius+1.25f,14,14,Fade(WHITE,0.25f));
            } else if (e.type == 2) {
                DrawModelEx(heavyModel,center,{0,1,0},static_cast<float>(GetTime()*16.0),{1.0f,1.0f,1.0f},e.color);
                DrawCubeWiresV(center,{1.65f,1.32f,1.95f},Fade(WHITE,0.55f));
            } else {
                DrawModel(droneModel,{e.pos.x,1.0f,e.pos.z},1.0f,e.color);
                DrawCube({e.pos.x,1.0f,e.pos.z},1.8f,0.14f,0.28f,Fade(e.color,0.88f));
                DrawSphereWires(center,e.radius+0.2f,8,8,Fade(WHITE,0.45f));
            }
            const float barW = e.type == 3 ? 4.8f : 1.55f;
            const float hpFrac = Clamp01(e.hp/std::max(1.0f,e.maxHp));
            DrawCube({e.pos.x, e.radius+1.9f, e.pos.z}, barW,0.10f,0.08f,Color{50,20,30,255});
            DrawCube({e.pos.x-barW*(1.0f-hpFrac)*0.5f,e.radius+1.9f,e.pos.z},barW*hpFrac,0.12f,0.09f,RED);
        }
        for (const Projectile& p : projectiles) {
            if (!p.active) continue;
            DrawSphere(p.pos,p.radius,p.color);
            DrawSphereWires(p.pos,p.radius+0.12f,5,5,Fade(p.color,0.55f));
            DrawLine3D(Vector3Subtract(p.pos,Vector3Scale(p.vel,0.035f)),p.pos,Fade(p.color,0.75f));
        }
        for (const Particle& p : particles) {
            DrawSphere(p.pos,p.size,Fade(p.color,Clamp01(p.life/std::max(0.01f,p.maxLife))));
        }

        // Player armor silhouette and shoulder-mounted pulse cannon.
        const Vector3 body{playerPos.x,0.92f,playerPos.z};
        DrawSphere({playerPos.x,0.18f,playerPos.z},0.72f,Fade(m.accent,0.20f));
        DrawModelEx(playerModel,body,{0,1,0},yaw*RAD2DEG,{1.0f,1.0f,1.0f},invulnerable>0.0f?WHITE:CYAN);
        DrawSphere({playerPos.x,1.9f,playerPos.z},0.36f,Color{130,230,255,255});
        DrawSphereWires({playerPos.x,1.9f,playerPos.z},0.43f,8,8,Fade(WHITE,0.6f));
        const Vector3 dir{std::sin(yaw),0.0f,std::cos(yaw)};
        DrawCylinderEx({playerPos.x+dir.x*0.25f,1.25f,playerPos.z+dir.z*0.25f},
                       {playerPos.x+dir.x*1.25f,1.25f,playerPos.z+dir.z*1.25f},0.15f,0.09f,8,AMBER);
        EndMode3D();
    }

    void DrawWrappedText(const char* text, int x, int y, int maxWidth, int fontSize, Color color) {
        std::istringstream stream(text ? text : "");
        std::string word;
        std::string line;
        const int lineStep = fontSize + 7;
        while (stream >> word) {
            const std::string candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && MeasureText(candidate.c_str(), fontSize) > maxWidth) {
                DrawText(line.c_str(), x, y, fontSize, color);
                y += lineStep;
                line = word;
            } else {
                line = candidate;
            }
        }
        if (!line.empty()) DrawText(line.c_str(), x, y, fontSize, color);
    }

    void DrawPanel(Rectangle r, Color accent) {
        DrawRectangleRec(r, PANEL);
        DrawRectangleLinesEx(r, 2.0f, Fade(accent,0.85f));
        DrawRectangle(static_cast<int>(r.x),static_cast<int>(r.y),static_cast<int>(r.width*0.28f),3,accent);
    }

    void DrawMeter(const char* label, float x, float y, float width, float value, Color color) {
        DrawText(label,static_cast<int>(x),static_cast<int>(y),16,LIGHTGRAY);
        DrawRectangle(static_cast<int>(x+72),static_cast<int>(y+2),static_cast<int>(width),14,Color{34,43,60,255});
        DrawRectangle(static_cast<int>(x+72),static_cast<int>(y+2),static_cast<int>(width*Clamp01(value)),14,color);
        DrawRectangleLines(static_cast<int>(x+72),static_cast<int>(y+2),static_cast<int>(width),14,Fade(WHITE,0.35f));
    }

    void DrawHud() {
        const Mission& m = CurrentMission();
        const float margin = screenW*0.025f;
        DrawRectangle(0,0,screenW,screenH*0.13f,Fade(NIGHT,0.86f));
        DrawText(TextFormat("VEX-9   LV %02d",save.playerLevel),static_cast<int>(margin),10,20,CYAN);
        DrawMeter("HULL",margin,39,screenW*0.14f,playerHP/std::max(1.0f,maxHP),playerHP<maxHP*0.25f?RED:MINT);
        DrawMeter("ENERGY",margin,62,screenW*0.14f,energy/100.0f,CYAN);
        DrawText(TextFormat("CREDITS %d",save.credits),static_cast<int>(screenW-margin-screenW*0.12f),14,19,AMBER);
        const int titleSize=std::max(16,static_cast<int>(screenH*0.027f));
        DrawText(m.title,screenW/2-MeasureText(m.title,titleSize)/2,12,titleSize,m.accent);
        DrawText(m.zone,screenW/2-MeasureText(m.zone,16)/2,38,16,LIGHTGRAY);
        DrawText(TextFormat("SHARDS %d/%d   HOSTILES %d/%d",coresCollected,m.cores,kills,m.kills),screenW/2-155,60,16,WHITE);
        const int remaining=std::max(0,m.durationSeconds-static_cast<int>(missionElapsed));
        DrawText(TextFormat("%02d:%02d",remaining/60,remaining%60),screenW/2-36,82,24,AMBER);
        if (m.boss && bossSpawned && !bossDefeated) {
            DrawText(m.bossName,screenW/2-MeasureText(m.bossName,18)/2,screenH*0.145f,18,RED);
        }
        if (empCooldown>0.0f || dashCooldown>0.0f) {
            DrawText(TextFormat("DASH %.1f   EMP %.1f",dashCooldown,empCooldown),screenW/2-80,screenH*0.18f,15,LIGHTGRAY);
        }
    }

    void DrawMobileControls() {
        const Vector2 left{screenW*0.155f,screenH*0.80f};
        const Vector2 right{screenW*0.845f,screenH*0.80f};
        const float r=std::min(screenH*0.115f,84.0f);
        const Vector2 dash{right.x-r*1.25f,right.y-r*1.35f};
        const Vector2 emp{right.x-r*0.05f,right.y-r*1.82f};
        DrawCircleV(left,r,Fade(CYAN,0.10f)); DrawCircleLines(static_cast<int>(left.x),static_cast<int>(left.y),r,Fade(CYAN,0.70f));
        DrawCircleV(left,r*0.36f,Fade(WHITE,0.11f));
        DrawText("MOVE",static_cast<int>(left.x)-24,static_cast<int>(left.y)+static_cast<int>(r)+7,14,Fade(WHITE,0.85f));
        DrawCircleV(right,r,Fade(NEON_PINK,0.08f)); DrawCircleLines(static_cast<int>(right.x),static_cast<int>(right.y),r,Fade(NEON_PINK,0.65f));
        DrawText("AIM + FIRE",static_cast<int>(right.x)-43,static_cast<int>(right.y)+static_cast<int>(r)+7,13,Fade(WHITE,0.85f));
        DrawCircleV(dash,r*0.34f,Fade(WHITE,0.20f)); DrawCircleLines(static_cast<int>(dash.x),static_cast<int>(dash.y),r*0.34f,WHITE);
        DrawText("DASH",static_cast<int>(dash.x)-21,static_cast<int>(dash.y)-7,13,WHITE);
        DrawCircleV(emp,r*0.34f,Fade(CYAN,0.29f)); DrawCircleLines(static_cast<int>(emp.x),static_cast<int>(emp.y),r*0.34f,WHITE);
        DrawText("EMP",static_cast<int>(emp.x)-17,static_cast<int>(emp.y)-7,13,WHITE);
    }

    void DrawTitle() {
        if (titleTexture.id) {
            DrawTexturePro(titleTexture,{0,0,static_cast<float>(titleTexture.width),static_cast<float>(titleTexture.height)},
                           {0,0,static_cast<float>(screenW),static_cast<float>(screenH)},{0,0},0,WHITE);
            DrawRectangle(0,0,screenW,screenH,Fade(NIGHT,0.56f));
        } else {
            ClearBackground(NIGHT);
            for (int i=0;i<65;++i) DrawCircle((i*173+31)%screenW,(i*97+19)%screenH,(i%3==0)?2.0f:1.0f,Fade(CYAN,0.6f));
        }
        const int big=std::max(36,static_cast<int>(screenH*0.075f));
        const int sub=std::max(20,static_cast<int>(screenH*0.035f));
        const char* title="CHRONO PROTOCOL";
        const char* subtitle="E C L I P S E";
        DrawText(title,screenW/2-MeasureText(title,big)/2,static_cast<int>(screenH*0.27f),big,CYAN);
        DrawText(subtitle,screenW/2-MeasureText(subtitle,sub)/2,static_cast<int>(screenH*0.27f)+big+10,sub,NEON_PINK);
        DrawText("A 3D SCI-FI STORY ACTION RPG",screenW/2-MeasureText("A 3D SCI-FI STORY ACTION RPG",19)/2,static_cast<int>(screenH*0.52f),19,WHITE);
        DrawText(TextFormat("STORY PROGRESS  //  CHAPTER %02d OF %02d",save.unlockedMission+1,MISSION_COUNT),screenW/2-MeasureText(TextFormat("STORY PROGRESS  //  CHAPTER %02d OF %02d",save.unlockedMission+1,MISSION_COUNT),17)/2,static_cast<int>(screenH*0.62f),17,LIGHTGRAY);
        const char* start="TAP ANYWHERE TO CONTINUE";
        DrawText(start,screenW/2-MeasureText(start,20)/2,static_cast<int>(screenH*0.75f),20,AMBER);
        DrawText("15 MISSIONS  |  TOUCH CONTROLS  |  AUTO-SAVED PROGRESS",screenW/2-MeasureText("15 MISSIONS  |  TOUCH CONTROLS  |  AUTO-SAVED PROGRESS",14)/2,screenH-36,14,LIGHTGRAY);
    }

    void DrawBriefing() {
        if (titleTexture.id) DrawTexturePro(titleTexture,{0,0,static_cast<float>(titleTexture.width),static_cast<float>(titleTexture.height)},
            {0,0,static_cast<float>(screenW),static_cast<float>(screenH)},{0,0},0,WHITE);
        DrawRectangle(0,0,screenW,screenH,Fade(NIGHT,0.80f));
        const Mission& m=CurrentMission();
        DrawText(TextFormat("ACT %d  /  MISSION %02d",missionIndex/3+1,missionIndex+1),static_cast<int>(screenW*0.07f),static_cast<int>(screenH*0.10f),18,AMBER);
        DrawText(m.title,static_cast<int>(screenW*0.07f),static_cast<int>(screenH*0.17f),std::max(24,static_cast<int>(screenH*0.048f)),m.accent);
        DrawText(m.zone,static_cast<int>(screenW*0.07f),static_cast<int>(screenH*0.25f),19,LIGHTGRAY);
        Rectangle panel{screenW*0.07f,screenH*0.33f,screenW*0.86f,screenH*0.26f};
        DrawPanel(panel,m.accent);
        DrawText("INCOMING TRANSMISSION",static_cast<int>(panel.x+22),static_cast<int>(panel.y+18),16,AMBER);
        DrawWrappedText(m.intro,static_cast<int>(panel.x+22),static_cast<int>(panel.y+54),static_cast<int>(panel.width-44),std::max(17,static_cast<int>(screenH*0.025f)),WHITE);
        DrawText(m.objective,static_cast<int>(screenW*0.07f),static_cast<int>(screenH*0.65f),18,MINT);
        const int minutes=m.durationSeconds/60;
        DrawText(TextFormat("ESTIMATED MISSION TIME: %d MINUTES  |  %s",minutes,m.boss?"BOSS ENCOUNTER CONFIRMED":"HOSTILE WAVES DETECTED"),static_cast<int>(screenW*0.07f),static_cast<int>(screenH*0.71f),16,LIGHTGRAY);
        DrawText("CONTROLS  :  LEFT STICK MOVE  •  RIGHT STICK AIM/FIRE  •  DASH  •  EMP",static_cast<int>(screenW*0.07f),static_cast<int>(screenH*0.79f),15,CYAN);
        DrawText("TAP TO DEPLOY",screenW/2-MeasureText("TAP TO DEPLOY",22)/2,static_cast<int>(screenH*0.88f),22,AMBER);
    }

    void DrawMissionClear() {
        DrawRectangle(0,0,screenW,screenH,Fade(NIGHT,0.9f));
        const Mission& m=CurrentMission();
        DrawText("MISSION COMPLETE",screenW/2-MeasureText("MISSION COMPLETE",36)/2,static_cast<int>(screenH*0.13f),36,MINT);
        DrawText(m.title,screenW/2-MeasureText(m.title,22)/2,static_cast<int>(screenH*0.23f),22,m.accent);
        DrawPanel({screenW*0.13f,screenH*0.32f,screenW*0.74f,screenH*0.25f},m.accent);
        DrawText("DEBRIEF  //  VEX-9",static_cast<int>(screenW*0.16f),static_cast<int>(screenH*0.36f),17,AMBER);
        DrawWrappedText(m.outro,static_cast<int>(screenW*0.16f),static_cast<int>(screenH*0.42f),static_cast<int>(screenW*0.68f),std::max(17,static_cast<int>(screenH*0.025f)),WHITE);
        DrawText(TextFormat("CREDITS %d   //   LEVEL %d   //   NEXT CHAPTER %02d",save.credits,save.playerLevel,std::min(MISSION_COUNT,missionIndex+2)),screenW/2-250,static_cast<int>(screenH*0.67f),18,CYAN);
        DrawText("TAP TO OPEN UPGRADE TERMINAL",screenW/2-MeasureText("TAP TO OPEN UPGRADE TERMINAL",20)/2,static_cast<int>(screenH*0.82f),20,AMBER);
    }

    void DrawUpgrade() {
        DrawRectangle(0,0,screenW,screenH,Color{3,7,18,255});
        DrawText("FIELD UPGRADE TERMINAL",screenW/2-MeasureText("FIELD UPGRADE TERMINAL",34)/2,static_cast<int>(screenH*0.12f),34,CYAN);
        DrawText(TextFormat("AVAILABLE CREDITS: %d",save.credits),screenW/2-125,static_cast<int>(screenH*0.23f),20,AMBER);
        const float cardY=screenH*0.36f, cardW=screenW*0.215f, cardH=screenH*0.26f, gap=screenW*0.025f;
        const float startX=(screenW-(cardW*3.0f+gap*2.0f))*0.5f;
        Rectangle weapon{startX,cardY,cardW,cardH}, armor{startX+cardW+gap,cardY,cardW,cardH}, emp{startX+(cardW+gap)*2.0f,cardY,cardW,cardH};
        DrawPanel(weapon,CYAN); DrawPanel(armor,MINT); DrawPanel(emp,NEON_PINK);
        DrawText(TextFormat("PULSE RIFLE  T%d",save.weaponTier),static_cast<int>(weapon.x+14),static_cast<int>(weapon.y+18),19,CYAN);
        DrawText("Increase shot damage",static_cast<int>(weapon.x+14),static_cast<int>(weapon.y+56),15,WHITE);
        DrawText(save.weaponTier>=10?"MAX TIER":"COST: 150 CREDITS",static_cast<int>(weapon.x+14),static_cast<int>(weapon.y+cardH-31),14,AMBER);
        DrawText(TextFormat("ARMOR  T%d",save.armorTier),static_cast<int>(armor.x+14),static_cast<int>(armor.y+18),19,MINT);
        DrawText("Increase hull integrity",static_cast<int>(armor.x+14),static_cast<int>(armor.y+56),15,WHITE);
        DrawText(save.armorTier>=10?"MAX TIER":"COST: 150 CREDITS",static_cast<int>(armor.x+14),static_cast<int>(armor.y+cardH-31),14,AMBER);
        DrawText(TextFormat("EMP CORE  T%d",save.empTier),static_cast<int>(emp.x+14),static_cast<int>(emp.y+18),19,NEON_PINK);
        DrawText("Increase nova damage",static_cast<int>(emp.x+14),static_cast<int>(emp.y+56),15,WHITE);
        DrawText(save.empTier>=10?"MAX TIER":"COST: 150 CREDITS",static_cast<int>(emp.x+14),static_cast<int>(emp.y+cardH-31),14,AMBER);
        Rectangle next{screenW*0.37f,screenH*0.72f,screenW*0.26f,screenH*0.10f};
        DrawPanel(next,CYAN);
        const char* nextText=(missionIndex+1<MISSION_COUNT)?"DEPLOY NEXT MISSION  >":"VIEW ENDING  >";
        DrawText(nextText,screenW/2-MeasureText(nextText,19)/2,static_cast<int>(next.y+next.height*0.34f),19,WHITE);
        DrawText("Tap an upgrade card, then deploy. Progress is saved automatically.",screenW/2-MeasureText("Tap an upgrade card, then deploy. Progress is saved automatically.",14)/2,static_cast<int>(screenH*0.87f),14,LIGHTGRAY);
    }

    void DrawGameOver() {
        DrawRectangle(0,0,screenW,screenH,Fade(NIGHT,0.93f));
        DrawText("SIGNAL LOST",screenW/2-MeasureText("SIGNAL LOST",42)/2,static_cast<int>(screenH*0.28f),42,RED);
        DrawText("VEX-9's armor has failed, but the mission is not lost.",screenW/2-MeasureText("VEX-9's armor has failed, but the mission is not lost.",20)/2,static_cast<int>(screenH*0.43f),20,WHITE);
        DrawText("TAP TO RETRY THIS MISSION",screenW/2-MeasureText("TAP TO RETRY THIS MISSION",22)/2,static_cast<int>(screenH*0.62f),22,AMBER);
    }

    void DrawEnding() {
        if (titleTexture.id) DrawTexturePro(titleTexture,{0,0,static_cast<float>(titleTexture.width),static_cast<float>(titleTexture.height)},
            {0,0,static_cast<float>(screenW),static_cast<float>(screenH)},{0,0},0,WHITE);
        DrawRectangle(0,0,screenW,screenH,Fade(NIGHT,0.74f));
        DrawText("ECLIPSE PROTOCOL: COMPLETE",screenW/2-MeasureText("ECLIPSE PROTOCOL: COMPLETE",34)/2,static_cast<int>(screenH*0.28f),34,CYAN);
        DrawText("Elena's voice is gone. Her choice remains.",screenW/2-MeasureText("Elena's voice is gone. Her choice remains.",22)/2,static_cast<int>(screenH*0.43f),22,WHITE);
        DrawText("The grid falls silent. Somewhere below, a city wakes.",screenW/2-MeasureText("The grid falls silent. Somewhere below, a city wakes.",19)/2,static_cast<int>(screenH*0.50f),19,MINT);
        DrawText("TAP TO RETURN TO TITLE",screenW/2-MeasureText("TAP TO RETURN TO TITLE",20)/2,static_cast<int>(screenH*0.70f),20,AMBER);
    }
};

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    ChronoProtocolGame game;
    if (!game.Init()) return 1;
    while (!WindowShouldClose()) game.RunFrame();
    game.Shutdown();
    return 0;
}
