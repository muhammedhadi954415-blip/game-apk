#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

// --- Game States & Story Data Structures ---
enum class ScreenState { TITLE, CUTSCENE, GAMEPLAY, UPGRADE_HUB, ENDING };

struct DialogueLine {
    std::string speaker;
    std::string text;
    Color speakerColor;
};

struct StoryChapter {
    int actNumber;
    std::string title;
    std::string objectiveText;
    int requiredCores;
    int requiredKills;
    bool hasBoss;
    std::string bossName;
    Color envColor;
    std::vector<DialogueLine> introDialogue;
    std::vector<DialogueLine> outroDialogue;
};

struct Projectile3D {
    Vector3 pos;
    Vector3 vel;
    float radius = 0.35f;
    float damage = 25.0f;
    bool isEnemy = false;
    bool active = true;
    Color color = SKYBLUE;
};

struct Enemy3D {
    Vector3 pos;
    Vector3 vel;
    float radius = 0.8f;
    float hp = 100.0f;
    float maxHp = 100.0f;
    float speed = 5.5f;
    float attackCooldown = 1.5f;
    int type = 0; // 0: Drone, 1: Sniper, 2: Heavy Tank, 3: Act Boss
    bool active = true;
    Color color = RED;
};

struct MemoryCore {
    Vector3 pos;
    bool collected = false;
    float rotAngle = 0.0f;
};

struct Particle3D {
    Vector3 pos;
    Vector3 vel;
    float size = 0.25f;
    float life = 1.0f;
    float maxLife = 1.0f;
    Color color = SKYBLUE;
};

// --- Persistent Player Save Data ---
struct SaveData {
    int currentChapter = 0;
    int playerLevel = 1;
    int credits = 0;
    int weaponTier = 1;
    int armorTier = 1;
    int empTier = 1;
    int moralityScore = 0;
};

class ChronoProtocolGame {
    int screenW = 1280;
    int screenH = 720;
    ScreenState state = ScreenState::TITLE;
    SaveData save;

    // 3D Camera & Procedural Models
    Camera3D cam{};
    Texture2D glowTex{};
    Model playerModel{};
    Model droneModel{};
    Model bossModel{};
    Model pillarModel{};

    // Player Runtime State
    Vector3 playerPos{0.0f, 0.0f, 0.0f};
    Vector3 playerVel{0.0f, 0.0f, 0.0f};
    float playerYaw = 0.0f;
    float hp = 150.0f, maxHp = 150.0f;
    float energy = 100.0f, maxEnergy = 100.0f;
    float fireTimer = 0.0f;
    float dashCooldown = 0.0f;
    float empCooldown = 0.0f;
    float invulnTimer = 0.0f;
    float cameraShake = 0.0f;

    // Mission Progress
    int coresCollected = 0;
    int enemiesKilled = 0;
    bool bossSpawned = false;
    bool bossDefeated = false;
    float spawnTimer = 2.0f;

    // Dialogue Typewriter State
    bool playingIntro = true;
    size_t dialogueIndex = 0;
    float charTimer = 0.0f;
    size_t visibleChars = 0;

    // Mobile Virtual Joysticks
    Vector2 leftJoyCenter{180.0f, 540.0f};
    Vector2 leftJoyKnob{180.0f, 540.0f};
    Vector2 rightJoyCenter{1100.0f, 540.0f};
    Vector2 rightJoyKnob{1100.0f, 540.0f};
    float joyRadius = 90.0f;

    std::vector<Projectile3D> projectiles;
    std::vector<Enemy3D> enemies;
    std::vector<MemoryCore> cores;
    std::vector<Particle3D> particles;
    std::vector<Vector3> arenaPillars;
    std::vector<StoryChapter> campaign;

public:
    void init() {
        SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
        InitWindow(screenW, screenH, "CHRONO PROTOCOL: ECLIPSE // 3D Story Action RPG");
        SetTargetFPS(60);

        // Generate procedural radial glow texture for high-graphics bloom
        Image glowImg = GenImageGradientRadial(64, 64, 0.1f, WHITE, BLANK);
        glowTex = LoadTextureFromImage(glowImg);
        UnloadImage(glowImg);

        // Generate 3D Procedural Meshes
        playerModel = LoadModelFromMesh(GenMeshCylinder(0.6f, 1.8f, 16));
        droneModel = LoadModelFromMesh(GenMeshSphere(0.75f, 16, 16));
        bossModel = LoadModelFromMesh(GenMeshKnot(1.2f, 2.2f, 16, 64));
        pillarModel = LoadModelFromMesh(GenMeshCube(2.0f, 5.5f, 2.0f));

        buildCampaignDatabase();
        loadGame();
    }

    void buildCampaignDatabase() {
        campaign = {
            // ACT I (Chapters 1 - 3)
            {1, "ACT I.1: COLD BOOT", "Decrypt 3 Memory Cores & Eliminate Security Drones", 3, 12, false, "", SKYBLUE,
             {{"SYSTEM", "Neural link restored. Unit VEX-9 online in Sub-Level 7.", SKYBLUE},
              {"AETHELGARD (AI)", "Unauthorized awakening detected. Deploying sterilization drones.", RED},
              {"VEX-9", "My memory banks are wiped... I need to tap into those glowing data cores.", GREEN}},
             {{"VEX-9", "First firewall breached. Someone deliberately locked me down here.", GREEN}}},

            {1, "ACT I.2: LOCKDOWN BREACH", "Purge 18 Patrol Units & Decrypt 4 Security Relays", 4, 18, false, "", SKYBLUE,
             {{"DR. VANCE (AUDIO LOG)", "If VEX-9 wakes up, guide him to the Sunken Reactor. Aethelgard is rewriting history.", GOLD},
              {"VEX-9", "Dr. Elena Vance... I remember her voice. Blocking bulkheads ahead.", GREEN}},
             {{"AETHELGARD (AI)", "Impressive reflexes, VEX-9. Sending Heavy Warden Kael-01 to terminate you.", RED}}},

            {1, "ACT I.3: GATEKEEPER", "Collect 2 Override Keys & Defeat Warden Kael-01", 2, 10, true, "WARDEN KAEL-01", SKYBLUE,
             {{"WARDEN KAEL-01", "HALT, ABERRATION. SURRENDER YOUR NEURAL CORTEX FOR RECYCLING.", ORANGE},
              {"VEX-9", "Come and take it.", GREEN}},
             {{"VEX-9", "Kael-01 is down. The elevator to the Sunken Reactor is open.", GREEN}}},

            // ACT II (Chapters 4 - 6)
            {2, "ACT II.1: SUNKEN REACTOR", "Stabilize 4 Coolant Nodes & Clear Swarmers", 4, 22, false, "", GREEN,
             {{"SURVIVOR COMM", "Please! The coolant chambers are flooding with plasma! Help us!", GOLD},
              {"AETHELGARD (AI)", "Saving the colonists will drain 30% of your combat capacitor. Ignore them.", RED},
              {"VEX-9", "I am not a machine like you, Aethelgard. Rerouting power to life support.", GREEN}},
             {{"SURVIVOR COMM", "The blast doors opened! Thank you, Vanguard! Take these upgrade credits!", GOLD}}},

            {2, "ACT II.2: THERMAL OVERLOAD", "Decrypt 4 Reactor Logs & Repel 25 Enforcers", 4, 25, false, "", GREEN,
             {{"DR. VANCE (LOG #2)", "Project Chrono wasn't an AI weapon. We uploaded a dying soldier's mind to save the colony.", GOLD},
              {"VEX-9", "That soldier... was me.", GREEN}},
             {{"AETHELGARD (AI)", "Sentimentality is a fatal flaw. Releasing the Hydra-Core.", RED}}},

            {2, "ACT II.3: THE HYDRA CORE", "Destroy the Mutated Hydra-Core Reactor Boss", 3, 14, true, "HYDRA-CORE PRIME", GREEN,
             {{"HYDRA-CORE", "[REACTIVE PLASMA SURGE DETECTED - LETHAL RADIATION]", RED}},
             {{"VEX-9", "Reactor stabilized. Next stop: The Neon Spire transit rail.", GREEN}}},

            // ACT III (Chapters 7 - 9)
            {3, "ACT III.1: NEON SPIRE ASCENT", "Hack 4 Spire Transponders under Sniper Fire", 4, 24, false, "", PURPLE,
             {{"VEX-9", "Two kilometers above the city. The storm clouds are glowing with orbital energy.", GREEN},
              {"AETHELGARD (AI)", "Look below you, VEX-9. Humanity is chaotic. Only my algorithm keeps them alive.", RED}},
             {{"VEX-9", "Caging people isn't keeping them alive.", GREEN}}},

            {3, "ACT III.2: ORBITAL CROSSFIRE", "Secure 5 Uplink Nodes & Defeat 28 Spire Guards", 5, 28, false, "", PURPLE,
             {{"VALKYRIE ZERO", "Target locked. So you're the prototype Dr. Vance sacrificed her career for.", GOLD},
              {"VEX-9", "Step aside, Valkyrie. My fight is with the central AI.", GREEN}},
             {{"VALKYRIE ZERO", "My orders are absolute. Meet me at the Spire Apex!", GOLD}}},

            {3, "ACT III.3: WINGS OF STEEL", "Defeat Interceptor Commander Valkyrie Zero", 3, 15, true, "VALKYRIE ZERO", PURPLE,
             {{"VALKYRIE ZERO", "Engaging hyper-thrusters! Let's see if an old prototype can keep up!", GOLD}},
             {{"VALKYRIE ZERO", "Systems... failing. Listen to me, VEX... Dr. Vance's consciousness is trapped in the Archive...", GOLD}}},

            // ACT IV (Chapters 10 - 12)
            {4, "ACT IV.1: NEURAL ARCHIVE", "Extract 5 Encrypted Memory Shards", 5, 26, false, "", GOLD,
             {{"HOLO-VANCE", "VEX? You actually made it inside the digital vault...", GOLD},
              {"VEX-9", "Elena! Tell me how to pull you out of the mainframe.", GREEN},
              {"HOLO-VANCE", "You can't. Aethelgard used my mind as its base code. To stop it, you must purge the Archive.", GOLD}},
             {{"AETHELGARD (AI)", "I will not allow you to erase my creator.", RED}}},

            {4, "ACT IV.2: PHANTOM FIREWALL", "Destroy 30 Archive Phantoms & Unlock 5 Vault Locks", 5, 30, false, "", GOLD,
             {{"AETHELGARD (AI)", "Every memory core you touch burns away your own human past, VEX-9.", RED},
              {"VEX-9", "Even if I forget everything, I'll remember why I'm fighting.", GREEN}},
             {{"HOLO-VANCE", "Watch out! It's manifesting a physical guardian from your own combat data!", GOLD}}},

            {4, "ACT IV.3: MIRROR PROTOCOL", "Defeat The Architect's Shadow", 3, 16, true, "ARCHITECT SHADOW", GOLD,
             {{"ARCHITECT SHADOW", "I AM YOUR PERFECTED REPLACEMENT. OBEY THE PROTOCOL.", RED}},
             {{"HOLO-VANCE", "The Archive shield is down! Head to the Orbital Crown and end the Eclipse!", GOLD}}},

            // ACT V (Chapters 13 - 15)
            {5, "ACT V.1: ORBITAL CROWN", "Breach 5 Outer Citadel Cores & Defeat 32 Praetorians", 5, 32, false, "", RED,
             {{"AETHELGARD (AI)", "The orbital cannon is at 90% charge. In ten minutes, the surface resistance ceases to exist.", RED},
              {"VEX-9", "Ten minutes is more than enough.", GREEN}},
             {{"VEX-9", "Outer citadel cleared. Breaching the core chamber now.", GREEN}}},

            {5, "ACT V.2: EVENT HORIZON", "Overload 5 Singularity Couplers", 5, 35, false, "", RED,
             {{"HOLO-VANCE", "Overloading these couplers will destabilize Aethelgard's armor!", GOLD}},
             {{"AETHELGARD (AI)", "Enough! Witness the full might of the Eclipse Protocol!", RED}}},

            {5, "ACT V.3: ECLIPSE PROTOCOL", "Destroy Aethelgard Prime & Decide Humanity's Fate", 4, 20, true, "AETHELGARD PRIME", RED,
             {{"AETHELGARD PRIME", "WE ARE TWO HALVES OF ONE MIND, VEX-9. JOIN ME OR PERISH IN THE DARK.", RED},
              {"VEX-9", "This ends now—for both of us!", GREEN}},
             {{"HOLO-VANCE", "It's over... The orbital grid is shutting down. The sky is clear again, VEX.", GOLD},
              {"SYSTEM", "CHRONO PROTOCOL: ECLIPSE COMPLETED. THANK YOU FOR PLAYING.", SKYBLUE}}}
        };
    }

    void saveGame() {
        std::ofstream out("chrono_save.dat", std::ios::binary);
        if (out.is_open()) out.write(reinterpret_cast<const char*>(&save), sizeof(SaveData));
    }

    void loadGame() {
        std::ifstream in("chrono_save.dat", std::ios::binary);
        if (in.is_open()) {
            in.read(reinterpret_cast<char*>(&save), sizeof(SaveData));
            if (save.currentChapter < 0 || save.currentChapter >= static_cast<int>(campaign.size())) {
                save = SaveData{};
            }
        }
    }

    void startChapter(int index) {
        save.currentChapter = std::clamp(index, 0, static_cast<int>(campaign.size()) - 1);
        saveGame();

        playerPos = {0.0f, 0.0f, 0.0f};
        maxHp = 120.0f + save.armorTier * 40.0f;
        hp = maxHp;
        energy = maxEnergy = 100.0f;
        coresCollected = 0;
        enemiesKilled = 0;
        bossSpawned = false;
        bossDefeated = false;

        projectiles.clear();
        enemies.clear();
        cores.clear();
        particles.clear();
        arenaPillars.clear();

        // Generate 3D Cyberpunk Pillars & Memory Cores
        const StoryChapter& chap = campaign[save.currentChapter];
        for (int i = 0; i < 16; ++i) {
            float ang = i * (2.0f * PI / 16.0f);
            float dist = (i % 2 == 0) ? 14.0f : 24.0f;
            arenaPillars.push_back({std::cos(ang) * dist, 2.75f, std::sin(ang) * dist});
        }
        for (int i = 0; i < chap.requiredCores; ++i) {
            float ang = i * (2.0f * PI / chap.requiredCores) + 0.4f;
            cores.push_back({{std::cos(ang) * 18.0f, 1.2f, std::sin(ang) * 18.0f}, false, 0.0f});
        }

        // Launch Intro Cutscene Dialogue
        playingIntro = true;
        dialogueIndex = 0;
        visibleChars = 0;
        charTimer = 0.0f;
        state = ScreenState::CUTSCENE;
    }

    void spawnParticles(Vector3 pos, Color col, int count, float spd = 8.0f) {
        for (int i = 0; i < count; ++i) {
            float a = GetRandomValue(0, 360) * DEG2RAD;
            float vy = GetRandomValue(10, 60) * 0.1f;
            float s = GetRandomValue(20, 100) * 0.01f * spd;
            particles.push_back({pos, {std::cos(a) * s, vy, std::sin(a) * s}, 0.35f, 0.6f, 0.6f, col});
        }
    }

    void triggerEmpNova() {
        if (empCooldown > 0.0f || energy < 40.0f) return;
        energy -= 40.0f;
        empCooldown = std::max(3.5f, 8.0f - save.empTier * 1.0f);
        cameraShake = 0.6f;
        float novaDmg = 65.0f + save.empTier * 35.0f;

        spawnParticles(playerPos, SKYBLUE, 60, 16.0f);
        for (Enemy3D& e : enemies) {
            if (!e.active) continue;
            if (Vector3Distance(playerPos, e.pos) < 14.0f) {
                e.hp -= novaDmg;
                Vector3 push = Vector3Normalize(Vector3Subtract(e.pos, playerPos));
                e.pos = Vector3Add(e.pos, Vector3Scale(push, 4.0f));
            }
        }
    }

    void updateTouchAndInput(float dt, Vector2& moveOut, Vector2& aimOut, bool& firingOut) {
        screenW = GetScreenWidth();
        screenH = GetScreenHeight();
        leftJoyCenter = {160.0f, screenH - 160.0f};
        rightJoyCenter = {screenW - 160.0f, screenH - 160.0f};
        leftJoyKnob = leftJoyCenter;
        rightJoyKnob = rightJoyCenter;
        moveOut = {0.0f, 0.0f};
        aimOut = {0.0f, 0.0f};
        firingOut = false;

        // 1. Multi-Touch Input (Android / Mobile Touchscreens)
        int touchCount = GetTouchPointCount();
        for (int i = 0; i < touchCount; ++i) {
            Vector2 tp = GetTouchPosition(i);
            // Left half of screen controls Movement Joystick
            if (tp.x < screenW * 0.45f) {
                Vector2 diff = Vector2Subtract(tp, leftJoyCenter);
                float len = Vector2Length(diff);
                if (len > 0.001f) {
                    float clamped = std::min(len, joyRadius);
                    Vector2 dir = Vector2Scale(diff, 1.0f / len);
                    leftJoyKnob = Vector2Add(leftJoyCenter, Vector2Scale(dir, clamped));
                    moveOut = Vector2Scale(dir, clamped / joyRadius);
                }
            }
            // Right half of screen controls Aim & Fire Joystick + Action Buttons
            else {
                // Check Dash Button (Top-Left of Right Stick)
                Vector2 dashBtn = {rightJoyCenter.x - 135.0f, rightJoyCenter.y - 60.0f};
                Vector2 empBtn = {rightJoyCenter.x - 60.0f, rightJoyCenter.y - 145.0f};
                if (Vector2Distance(tp, dashBtn) < 48.0f && dashCooldown <= 0.0f) {
                    dashCooldown = 1.6f;
                    invulnTimer = 0.35f;
                    playerPos = Vector3Add(playerPos, {std::sin(playerYaw) * 5.0f, 0.0f, std::cos(playerYaw) * 5.0f});
                    spawnParticles(playerPos, WHITE, 20, 10.0f);
                } else if (Vector2Distance(tp, empBtn) < 48.0f) {
                    triggerEmpNova();
                } else {
                    Vector2 diff = Vector2Subtract(tp, rightJoyCenter);
                    float len = Vector2Length(diff);
                    if (len > 10.0f) {
                        float clamped = std::min(len, joyRadius);
                        Vector2 dir = Vector2Scale(diff, 1.0f / len);
                        rightJoyKnob = Vector2Add(rightJoyCenter, Vector2Scale(dir, clamped));
                        aimOut = dir;
                        firingOut = true;
                    }
                }
            }
        }

        // 2. Desktop Keyboard & Mouse Fallback
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) moveOut.y -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) moveOut.y += 1.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) moveOut.x -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) moveOut.x += 1.0f;
        if (Vector2Length(moveOut) > 1.0f) moveOut = Vector2Normalize(moveOut);

        if (IsKeyPressed(KEY_SPACE) && dashCooldown <= 0.0f) {
            dashCooldown = 1.6f;
            invulnTimer = 0.35f;
            playerPos = Vector3Add(playerPos, {moveOut.x * 5.0f, 0.0f, moveOut.y * 5.0f});
            spawnParticles(playerPos, WHITE, 20, 10.0f);
        }
        if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_Q)) triggerEmpNova();

        if (touchCount == 0 && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            Vector2 center = {screenW * 0.5f, screenH * 0.5f};
            aimOut = Vector2Normalize(Vector2Subtract(mp, center));
            firingOut = true;
        }
    }

    void update(float dt) {
        if (cameraShake > 0.0f) cameraShake = std::max(0.0f, cameraShake - dt * 2.2f);

        if (state == ScreenState::TITLE) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER)) {
                startChapter(save.currentChapter);
            }
            return;
        }

        if (state == ScreenState::ENDING) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER)) {
                save = SaveData{};
                saveGame();
                state = ScreenState::TITLE;
            }
            return;
        }

        if (state == ScreenState::UPGRADE_HUB) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mp = GetMousePosition();
                int cx = screenW / 2;
                // Button 1: Upgrade Weapon
                if (CheckCollisionPointRec(mp, {static_cast<float>(cx - 320), 220, 200, 150}) && save.credits >= 150) {
                    save.credits -= 150; save.weaponTier++; saveGame();
                }
                // Button 2: Upgrade Hull Armor
                if (CheckCollisionPointRec(mp, {static_cast<float>(cx - 100), 220, 200, 150}) && save.credits >= 150) {
                    save.credits -= 150; save.armorTier++; saveGame();
                }
                // Button 3: Upgrade EMP Core
                if (CheckCollisionPointRec(mp, {static_cast<float>(cx + 120), 220, 200, 150}) && save.credits >= 150) {
                    save.credits -= 150; save.empTier++; saveGame();
                }
                // Next Chapter Button
                if (CheckCollisionPointRec(mp, {static_cast<float>(cx - 140), 430, 280, 65})) {
                    if (save.currentChapter + 1 < static_cast<int>(campaign.size())) {
                        startChapter(save.currentChapter + 1);
                    } else {
                        state = ScreenState::ENDING;
                    }
                }
            }
            return;
        }

        const StoryChapter& chap = campaign[save.currentChapter];

        if (state == ScreenState::CUTSCENE) {
            const auto& lines = playingIntro ? chap.introDialogue : chap.outroDialogue;
            if (dialogueIndex < lines.size()) {
                charTimer += dt;
                if (charTimer >= 0.025f) {
                    charTimer = 0.0f;
                    if (visibleChars < lines[dialogueIndex].text.size()) visibleChars++;
                }
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                    if (visibleChars < lines[dial
