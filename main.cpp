#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

// --- Game states and story data ---
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
    Vector3 pos{};
    Vector3 vel{};
    float radius = 0.25f;
    float damage = 25.0f;
    bool isEnemy = false;
    bool active = true;
    Color color = SKYBLUE;
};

struct Enemy3D {
    Vector3 pos{};
    Vector3 vel{};
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
    Vector3 pos{};
    bool collected = false;
    float rotAngle = 0.0f;
};

struct Particle3D {
    Vector3 pos{};
    Vector3 vel{};
    float size = 0.25f;
    float life = 1.0f;
    float maxLife = 1.0f;
    Color color = SKYBLUE;
};

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
    SaveData save{};

    Camera3D cam{};
    Texture2D glowTex{};
    Model playerModel{};
    Model droneModel{};
    Model bossModel{};
    Model pillarModel{};

    Vector3 playerPos{0.0f, 0.0f, 0.0f};
    float playerYaw = 0.0f;
    float hp = 150.0f;
    float maxHp = 150.0f;
    float energy = 100.0f;
    float maxEnergy = 100.0f;
    float fireTimer = 0.0f;
    float dashCooldown = 0.0f;
    float empCooldown = 0.0f;
    float invulnTimer = 0.0f;
    float cameraShake = 0.0f;

    int coresCollected = 0;
    int enemiesKilled = 0;
    bool bossSpawned = false;
    bool bossDefeated = false;
    bool missionCompletionStarted = false;
    float spawnTimer = 2.0f;

    bool playingIntro = true;
    size_t dialogueIndex = 0;
    float charTimer = 0.0f;
    size_t visibleChars = 0;

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
        if (!IsWindowReady()) return;
        SetTargetFPS(60);

        Image glowImg = GenImageGradientRadial(64, 64, 0.1f, WHITE, BLANK);
        glowTex = LoadTextureFromImage(glowImg);
        UnloadImage(glowImg);

        playerModel = LoadModelFromMesh(GenMeshCylinder(0.6f, 1.8f, 16));
        droneModel = LoadModelFromMesh(GenMeshSphere(0.75f, 16, 16));
        bossModel = LoadModelFromMesh(GenMeshKnot(1.2f, 2.2f, 16, 64));
        pillarModel = LoadModelFromMesh(GenMeshCube(2.0f, 5.5f, 2.0f));

        buildCampaignDatabase();
        loadGame();
        screenW = GetScreenWidth();
        screenH = GetScreenHeight();
    }

    void buildCampaignDatabase() {
        campaign = {
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
            if (!in || save.currentChapter < 0 || save.currentChapter >= static_cast<int>(campaign.size())) {
                save = SaveData{};
            }
        }
    }

    void startChapter(int index) {
        if (campaign.empty()) return;
        save.currentChapter = std::clamp(index, 0, static_cast<int>(campaign.size()) - 1);
        saveGame();
        playerPos = {0.0f, 0.0f, 0.0f};
        maxHp = 120.0f + static_cast<float>(save.armorTier) * 40.0f;
        hp = maxHp;
        energy = maxEnergy = 100.0f;
        coresCollected = 0;
        enemiesKilled = 0;
        bossSpawned = false;
        bossDefeated = false;
        missionCompletionStarted = false;
        spawnTimer = 1.5f;
        fireTimer = dashCooldown = empCooldown = invulnTimer = cameraShake = 0.0f;
        projectiles.clear();
        enemies.clear();
        cores.clear();
        particles.clear();
        arenaPillars.clear();

        const StoryChapter& chap = campaign[save.currentChapter];
        for (int i = 0; i < 16; ++i) {
            const float ang = static_cast<float>(i) * (2.0f * PI / 16.0f);
            const float dist = (i % 2 == 0) ? 14.0f : 24.0f;
            arenaPillars.push_back({std::cos(ang) * dist, 2.75f, std::sin(ang) * dist});
        }
        for (int i = 0; i < chap.requiredCores; ++i) {
            const float ang = static_cast<float>(i) * (2.0f * PI / static_cast<float>(chap.requiredCores)) + 0.4f;
            cores.push_back({{std::cos(ang) * 18.0f, 1.2f, std::sin(ang) * 18.0f}, false, 0.0f});
        }
        playingIntro = true;
        dialogueIndex = 0;
        visibleChars = 0;
        charTimer = 0.0f;
        state = ScreenState::CUTSCENE;
    }

    void spawnParticles(Vector3 pos, Color col, int count, float spd = 8.0f) {
        for (int i = 0; i < count; ++i) {
            const float a = static_cast<float>(GetRandomValue(0, 360)) * DEG2RAD;
            const float vy = static_cast<float>(GetRandomValue(10, 60)) * 0.1f;
            const float s = static_cast<float>(GetRandomValue(20, 100)) * 0.01f * spd;
            particles.push_back({pos, {std::cos(a) * s, vy, std::sin(a) * s}, 0.35f, 0.6f, 0.6f, col});
        }
    }

    void triggerEmpNova() {
        if (empCooldown > 0.0f || energy < 40.0f) return;
        energy -= 40.0f;
        empCooldown = std::max(3.5f, 8.0f - static_cast<float>(save.empTier));
        cameraShake = 0.6f;
        const float novaDmg = 65.0f + static_cast<float>(save.empTier) * 35.0f;
        spawnParticles(playerPos, SKYBLUE, 60, 16.0f);
        for (Enemy3D& e : enemies) {
            if (!e.active || Vector3Distance(playerPos, e.pos) >= 14.0f) continue;
            e.hp -= novaDmg;
            Vector3 push = Vector3Subtract(e.pos, playerPos);
            if (Vector3Length(push) > 0.001f) push = Vector3Normalize(push);
            e.pos = Vector3Add(e.pos, Vector3Scale(push, 4.0f));
        }
    }

    void updateTouchAndInput(Vector2& moveOut, Vector2& aimOut, bool& firingOut) {
        screenW = GetScreenWidth();
        screenH = GetScreenHeight();
        leftJoyCenter = {160.0f, static_cast<float>(screenH) - 160.0f};
        rightJoyCenter = {static_cast<float>(screenW) - 160.0f, static_cast<float>(screenH) - 160.0f};
        leftJoyKnob = leftJoyCenter;
        rightJoyKnob = rightJoyCenter;
        moveOut = {0.0f, 0.0f};
        aimOut = {0.0f, 0.0f};
        firingOut = false;

        const int touchCount = GetTouchPointCount();
        for (int i = 0; i < touchCount; ++i) {
            const Vector2 tp = GetTouchPosition(i);
            if (tp.x < static_cast<float>(screenW) * 0.45f) {
                const Vector2 diff = Vector2Subtract(tp, leftJoyCenter);
                const float len = Vector2Length(diff);
                if (len > 0.001f) {
                    const float clamped = std::min(len, joyRadius);
                    const Vector2 dir = Vector2Scale(diff, 1.0f / len);
                    leftJoyKnob = Vector2Add(leftJoyCenter, Vector2Scale(dir, clamped));
                    moveOut = Vector2Scale(dir, clamped / joyRadius);
                }
            } else {
                const Vector2 dashBtn{rightJoyCenter.x - 135.0f, rightJoyCenter.y - 60.0f};
                const Vector2 empBtn{rightJoyCenter.x - 60.0f, rightJoyCenter.y - 145.0f};
                if (Vector2Distance(tp, dashBtn) < 48.0f && dashCooldown <= 0.0f) {
                    dashCooldown = 1.6f;
                    invulnTimer = 0.35f;
                    const Vector3 dashDir{std::sin(playerYaw), 0.0f, std::cos(playerYaw)};
                    playerPos = Vector3Add(playerPos, Vector3Scale(dashDir, 5.0f));
                    spawnParticles(playerPos, WHITE, 20, 10.0f);
                } else if (Vector2Distance(tp, empBtn) < 48.0f) {
                    triggerEmpNova();
                } else {
                    const Vector2 diff = Vector2Subtract(tp, rightJoyCenter);
                    const float len = Vector2Length(diff);
                    if (len > 10.0f) {
                        const float clamped = std::min(len, joyRadius);
                        const Vector2 dir = Vector2Scale(diff, 1.0f / len);
                        rightJoyKnob = Vector2Add(rightJoyCenter, Vector2Scale(dir, clamped));
                        aimOut = dir;
                        firingOut = true;
                    }
                }
            }
        }

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) moveOut.y -= 1.0f;
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) moveOut.y += 1.0f;
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) moveOut.x -= 1.0f;
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) moveOut.x += 1.0f;
        if (Vector2Length(moveOut) > 1.0f) moveOut = Vector2Normalize(moveOut);

        if (IsKeyPressed(KEY_SPACE) && dashCooldown <= 0.0f) {
            dashCooldown = 1.6f;
            invulnTimer = 0.35f;
            Vector3 dashDir{moveOut.x, 0.0f, moveOut.y};
            if (Vector3Length(dashDir) < 0.01f) dashDir = {std::sin(playerYaw), 0.0f, std::cos(playerYaw)};
            dashDir = Vector3Normalize(dashDir);
            playerPos = Vector3Add(playerPos, Vector3Scale(dashDir, 5.0f));
            spawnParticles(playerPos, WHITE, 20, 10.0f);
        }
        if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_Q)) triggerEmpNova();

        if (touchCount == 0 && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            const Vector2 mp = GetMousePosition();
            const Vector2 center{static_cast<float>(screenW) * 0.5f, static_cast<float>(screenH) * 0.5f};
            aimOut = Vector2Normalize(Vector2Subtract(mp, center));
            firingOut = true;
        }
    }

    void spawnEnemy(int type) {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const float distance = static_cast<float>(GetRandomValue(18, 25));
        Enemy3D e;
        e.pos = {playerPos.x + std::cos(angle) * distance, 0.0f, playerPos.z + std::sin(angle) * distance};
        e.type = type;
        if (type == 3) {
            const int act = campaign[save.currentChapter].actNumber;
            e.radius = 2.0f;
            e.maxHp = e.hp = 650.0f + static_cast<float>(act) * 250.0f;
            e.speed = 2.6f + static_cast<float>(act) * 0.15f;
            e.attackCooldown = 1.2f;
            e.color = campaign[save.currentChapter].envColor;
        } else if (type == 2) {
            e.radius = 1.15f;
            e.maxHp = e.hp = 190.0f;
            e.speed = 2.4f;
            e.attackCooldown = 1.8f;
            e.color = ORANGE;
        } else if (type == 1) {
            e.radius = 0.7f;
            e.maxHp = e.hp = 75.0f;
            e.speed = 3.0f;
            e.attackCooldown = 1.35f;
            e.color = PURPLE;
        } else {
            e.radius = 0.8f;
            e.maxHp = e.hp = 100.0f;
            e.speed = 4.2f;
            e.attackCooldown = 1.7f;
            e.color = RED;
        }
        enemies.push_back(e);
    }

    void beginOutro() {
        if (missionCompletionStarted) return;
        missionCompletionStarted = true;
        save.credits += 100 + campaign[save.currentChapter].actNumber * 50;
        save.playerLevel += 1;
        saveGame();
        playingIntro = false;
        dialogueIndex = 0;
        visibleChars = 0;
        charTimer = 0.0f;
        state = ScreenState::CUTSCENE;
    }

    void update(float dt) {
        screenW = GetScreenWidth();
        screenH = GetScreenHeight();
        cameraShake = std::max(0.0f, cameraShake - dt * 2.2f);
        dashCooldown = std::max(0.0f, dashCooldown - dt);
        empCooldown = std::max(0.0f, empCooldown - dt);
        fireTimer = std::max(0.0f, fireTimer - dt);
        invulnTimer = std::max(0.0f, invulnTimer - dt);

        if (state == ScreenState::TITLE) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                startChapter(save.currentChapter);
            }
            return;
        }

        if (state == ScreenState::ENDING) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                save = SaveData{};
                saveGame();
                state = ScreenState::TITLE;
            }
            return;
        }

        if (state == ScreenState::UPGRADE_HUB) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER)) {
                const Vector2 mp = GetMousePosition();
                const int cx = screenW / 2;
                if (CheckCollisionPointRec(mp, {static_cast<float>(cx - 320), 220, 200, 150}) && save.credits >= 150) {
                    save.credits -= 150; save.weaponTier++; saveGame();
                } else if (CheckCollisionPointRec(mp, {static_cast<float>(cx - 100), 220, 200, 150}) && save.credits >= 150) {
                    save.credits -= 150; save.armorTier++; saveGame();
                } else if (CheckCollisionPointRec(mp, {static_cast<float>(cx + 120), 220, 200, 150}) && save.credits >= 150) {
                    save.credits -= 150; save.empTier++; saveGame();
                } else if (CheckCollisionPointRec(mp, {static_cast<float>(cx - 140), 430, 280, 65}) || IsKeyPressed(KEY_ENTER)) {
                    if (save.currentChapter + 1 < static_cast<int>(campaign.size())) startChapter(save.currentChapter + 1);
                    else state = ScreenState::ENDING;
                }
            }
            return;
        }

        const StoryChapter& chap = campaign[save.currentChapter];
        if (state == ScreenState::CUTSCENE) {
            const auto& lines = playingIntro ? chap.introDialogue : chap.outroDialogue;
            if (dialogueIndex >= lines.size()) {
                if (playingIntro) {
                    state = ScreenState::GAMEPLAY;
                } else {
                    if (save.currentChapter + 1 >= static_cast<int>(campaign.size())) state = ScreenState::ENDING;
                    else state = ScreenState::UPGRADE_HUB;
                }
                return;
            }

            charTimer += dt;
            if (charTimer >= 0.025f) {
                charTimer = 0.0f;
                if (visibleChars < lines[dialogueIndex].text.size()) visibleChars++;
            }

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || GetTouchPointCount() > 0) {
                if (visibleChars < lines[dialogueIndex].text.size()) {
                    visibleChars = lines[dialogueIndex].text.size();
                } else {
                    dialogueIndex++;
                    visibleChars = 0;
                    charTimer = 0.0f;
                }
            }
            return;
        }

        if (state != ScreenState::GAMEPLAY) return;

        Vector2 moveInput{}, aimInput{};
        bool firing = false;
        updateTouchAndInput(moveInput, aimInput, firing);
        Vector3 moveDir{moveInput.x, 0.0f, moveInput.y};
        if (Vector3Length(moveDir) > 1.0f) moveDir = Vector3Normalize(moveDir);
        playerPos = Vector3Add(playerPos, Vector3Scale(moveDir, 8.5f * dt));
        playerPos.x = std::clamp(playerPos.x, -32.0f, 32.0f);
        playerPos.z = std::clamp(playerPos.z, -32.0f, 32.0f);
        energy = std::min(maxEnergy, energy + 9.0f * dt);

        if (Vector2Length(aimInput) > 0.1f) playerYaw = std::atan2(aimInput.x, aimInput.y);
        if (firing && fireTimer <= 0.0f) {
            const Vector3 dir{std::sin(playerYaw), 0.0f, std::cos(playerYaw)};
            const Vector3 origin = Vector3Add(playerPos, Vector3Scale(dir, 1.1f));
            projectiles.push_back({origin, Vector3Scale(dir, 24.0f), 0.28f, 28.0f + save.weaponTier * 8.0f, false, true, SKYBLUE});
            fireTimer = std::max(0.09f, 0.24f - static_cast<float>(save.weaponTier) * 0.025f);
        }

        for (MemoryCore& core : cores) {
            if (core.collected) continue;
            core.rotAngle += 90.0f * dt;
            if (Vector3Distance(playerPos, core.pos) < 1.7f) {
                core.collected = true;
                coresCollected++;
                save.credits += 10;
                spawnParticles(core.pos, SKYBLUE, 18, 8.0f);
                saveGame();
            }
        }

        spawnTimer -= dt;
        int activeNonBoss = 0;
        for (const Enemy3D& e : enemies) if (e.active && e.type != 3) activeNonBoss++;
        if (enemiesKilled < chap.requiredKills && activeNonBoss < 7 && spawnTimer <= 0.0f) {
            int type = GetRandomValue(0, 9) < 6 ? 0 : (GetRandomValue(0, 1) == 0 ? 1 : 2);
            spawnEnemy(type);
            spawnTimer = std::max(0.55f, 1.8f - chap.actNumber * 0.18f);
        }
        if (chap.hasBoss && enemiesKilled >= chap.requiredKills && !bossSpawned) {
            spawnEnemy(3);
            bossSpawned = true;
        }

        for (Enemy3D& e : enemies) {
            if (!e.active) continue;
            e.attackCooldown -= dt;
            Vector3 toPlayer = Vector3Subtract(playerPos, e.pos);
            const float distance = Vector3Length(toPlayer);
            if (distance > 0.01f) toPlayer = Vector3Scale(toPlayer, 1.0f / distance);

            if (e.type == 1) {
                if (distance > 10.0f) e.pos = Vector3Add(e.pos, Vector3Scale(toPlayer, e.speed * dt));
                else if (distance < 7.0f) e.pos = Vector3Subtract(e.pos, Vector3Scale(toPlayer, e.speed * dt));
            } else if (distance > e.radius + 1.2f) {
                e.pos = Vector3Add(e.pos, Vector3Scale(toPlayer, e.speed * dt));
            }
            if (distance < e.radius + 0.9f && invulnTimer <= 0.0f) {
                hp -= (e.type == 3 ? 35.0f : 22.0f) * dt;
            }
            if (e.attackCooldown <= 0.0f && distance > 2.0f) {
                const float projectileSpeed = (e.type == 1 || e.type == 3) ? 13.5f : 9.0f;
                const float damage = (e.type == 3 ? 22.0f : (e.type == 1 ? 15.0f : 10.0f));
                projectiles.push_back({Vector3Add(e.pos, Vector3Scale(toPlayer, e.radius)), Vector3Scale(toPlayer, projectileSpeed), 0.28f, damage, true, true, e.color});
                e.attackCooldown = (e.type == 3) ? 0.8f : ((e.type == 1) ? 1.4f : 2.0f);
            }
        }

        for (Projectile3D& p : projectiles) {
            if (!p.active) continue;
            p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
            if (std::fabs(p.pos.x) > 45.0f || std::fabs(p.pos.z) > 45.0f) { p.active = false; continue; }
            if (p.isEnemy) {
                if (Vector3Distance(p.pos, playerPos) < p.radius + 0.7f) {
                    if (invulnTimer <= 0.0f) hp -= p.damage;
                    p.active = false;
                    spawnParticles(p.pos, p.color, 6, 5.0f);
                }
            } else {
                for (Enemy3D& e : enemies) {
                    if (!e.active) continue;
                    if (Vector3Distance(p.pos, e.pos) < p.radius + e.radius) {
                        e.hp -= p.damage;
                        p.active = false;
                        spawnParticles(p.pos, e.color, 5, 4.0f);
                        if (e.hp <= 0.0f) {
                            e.active = false;
                            enemiesKilled++;
                            if (e.type == 3) bossDefeated = true;
                            save.credits += (e.type == 3 ? 120 : 5);
                            spawnParticles(e.pos, e.color, e.type == 3 ? 65 : 18, e.type == 3 ? 18.0f : 8.0f);
                            saveGame();
                        }
                        break;
                    }
                }
            }
        }
        projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile3D& p) { return !p.active; }), projectiles.end());
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [](const Enemy3D& e) { return !e.active; }), enemies.end());

        for (Particle3D& p : particles) {
            p.life -= dt;
            p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
            p.vel.y -= 8.0f * dt;
            p.size = std::max(0.03f, p.size - dt * 0.22f);
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle3D& p) { return p.life <= 0.0f; }), particles.end());

        if (hp <= 0.0f) {
            hp = 0.0f;
            state = ScreenState::TITLE;
            return;
        }
        if (coresCollected >= chap.requiredCores && enemiesKilled >= chap.requiredKills && (!chap.hasBoss || bossDefeated)) beginOutro();
    }

    void drawWorld() {
        const StoryChapter& chap = campaign[save.currentChapter];
        cam.position = {playerPos.x + 0.0f, 22.0f, playerPos.z + 23.0f};
        cam.target = {playerPos.x, 0.0f, playerPos.z};
        cam.up = {0.0f, 1.0f, 0.0f};
        cam.fovy = 45.0f;
        cam.projection = CAMERA_PERSPECTIVE;
        if (cameraShake > 0.0f) {
            cam.position.x += static_cast<float>(GetRandomValue(-100, 100)) / 100.0f * cameraShake;
            cam.position.z += static_cast<float>(GetRandomValue(-100, 100)) / 100.0f * cameraShake;
        }

        BeginMode3D(cam);
        DrawPlane({0.0f, -0.15f, 0.0f}, {100.0f, 100.0f}, Color{9, 14, 26, 255});
        DrawGrid(60, 1.0f);
        for (const Vector3& pos : arenaPillars) {
            DrawModel(pillarModel, pos, 1.0f, Fade(chap.envColor, 0.68f));
            DrawCubeWiresV(pos, {2.05f, 5.55f, 2.05f}, Fade(chap.envColor, 0.8f));
        }
        for (const MemoryCore& core : cores) {
            if (core.collected) continue;
            Vector3 p = core.pos;
            p.y += std::sin(core.rotAngle * DEG2RAD) * 0.25f;
            DrawSphere(p, 0.72f, SKYBLUE);
            DrawSphereWires(p, 0.9f, 8, 8, Fade(WHITE, 0.75f));
            DrawLine3D({p.x - 1.1f, p.y, p.z}, {p.x + 1.1f, p.y, p.z}, Fade(SKYBLUE, 0.65f));
            DrawLine3D({p.x, p.y - 1.1f, p.z}, {p.x, p.y + 1.1f, p.z}, Fade(SKYBLUE, 0.65f));
        }
        for (const Enemy3D& e : enemies) {
            if (e.type == 3) {
                DrawModel(bossModel, {e.pos.x, e.pos.y + 1.8f, e.pos.z}, 1.15f, e.color);
                DrawSphereWires({e.pos.x, e.pos.y + 1.0f, e.pos.z}, e.radius + 0.8f, 12, 12, Fade(e.color, 0.5f));
            } else {
                DrawModel(droneModel, {e.pos.x, e.pos.y + 0.8f, e.pos.z}, e.type == 2 ? 1.35f : 0.85f, e.color);
            }
            const float barW = e.type == 3 ? 4.0f : 1.6f;
            const float frac = std::clamp(e.hp / std::max(1.0f, e.maxHp), 0.0f, 1.0f);
            DrawCube({e.pos.x, e.pos.y + e.radius + 1.0f, e.pos.z}, barW, 0.08f, 0.08f, DARKGRAY);
            DrawCube({e.pos.x - barW * (1.0f - frac) * 0.5f, e.pos.y + e.radius + 1.0f, e.pos.z}, barW * frac, 0.1f, 0.1f, RED);
        }
        for (const Projectile3D& p : projectiles) {
            DrawSphere(p.pos, p.radius, p.color);
            DrawSphereWires(p.pos, p.radius + 0.12f, 5, 5, Fade(p.color, 0.55f));
        }
        for (const Particle3D& p : particles) {
            DrawSphere(p.pos, p.size, Fade(p.color, std::clamp(p.life / std::max(0.01f, p.maxLife), 0.0f, 1.0f)));
        }
        const Vector3 playerModelPos{playerPos.x, 0.95f, playerPos.z};
        DrawModelEx(playerModel, playerModelPos, {0.0f, 1.0f, 0.0f}, playerYaw * RAD2DEG, {1.0f, 1.0f, 1.0f}, invulnTimer > 0.0f ? WHITE : SKYBLUE);
        DrawSphere({playerPos.x, 0.13f, playerPos.z}, 0.55f, Fade(SKYBLUE, 0.25f));
        EndMode3D();
    }

    void drawBars() {
        DrawText(TextFormat("VEX-9  |  LV %d", save.playerLevel), 24, 18, 22, SKYBLUE);
        DrawText("HULL", 24, 50, 16, LIGHTGRAY);
        DrawRectangle(84, 51, 220, 17, DARKGRAY);
        DrawRectangle(84, 51, static_cast<int>(220.0f * std::clamp(hp / std::max(1.0f, maxHp), 0.0f, 1.0f)), 17, (hp < maxHp * 0.3f ? RED : LIME));
        DrawText(TextFormat("%.0f / %.0f", hp, maxHp), 312, 50, 16, WHITE);
        DrawText("ENERGY", 24, 76, 16, LIGHTGRAY);
        DrawRectangle(84, 78, 220, 14, DARKGRAY);
        DrawRectangle(84, 78, static_cast<int>(220.0f * std::clamp(energy / maxEnergy, 0.0f, 1.0f)), 14, SKYBLUE);
        DrawText(TextFormat("CREDITS %d", save.credits), screenW - 220, 20, 20, GOLD);

        const StoryChapter& chap = campaign[save.currentChapter];
        DrawText(chap.title.c_str(), screenW / 2 - MeasureText(chap.title.c_str(), 22) / 2, 18, 22, chap.envColor);
        DrawText(chap.objectiveText.c_str(), screenW / 2 - MeasureText(chap.objectiveText.c_str(), 16) / 2, 48, 16, RAYWHITE);
        DrawText(TextFormat("CORES %d/%d   UNITS %d/%d", coresCollected, chap.requiredCores, enemiesKilled, chap.requiredKills), screenW / 2 - 180, 72, 16, WHITE);
        if (chap.hasBoss && bossSpawned && !bossDefeated) {
            DrawText(chap.bossName.c_str(), screenW / 2 - MeasureText(chap.bossName.c_str(), 20) / 2, 100, 20, RED);
        }
        DrawText(TextFormat("WEAPON T%d   ARMOR T%d   EMP T%d", save.weaponTier, save.armorTier, save.empTier), screenW - 280, 48, 16, LIGHTGRAY);
    }

    void drawMobileControls() {
        leftJoyCenter = {160.0f, static_cast<float>(screenH) - 160.0f};
        rightJoyCenter = {static_cast<float>(screenW) - 160.0f, static_cast<float>(screenH) - 160.0f};
        DrawCircle(static_cast<int>(leftJoyCenter.x), static_cast<int>(leftJoyCenter.y), joyRadius, Fade(SKYBLUE, 0.12f));
        DrawCircleLines(static_cast<int>(leftJoyCenter.x), static_cast<int>(leftJoyCenter.y), joyRadius, Fade(SKYBLUE, 0.65f));
        DrawCircle(static_cast<int>(leftJoyKnob.x), static_cast<int>(leftJoyKnob.y), 31.0f, Fade(WHITE, 0.32f));
        DrawCircleLines(static_cast<int>(leftJoyKnob.x), static_cast<int>(leftJoyKnob.y), 31.0f, Fade(WHITE, 0.8f));

        DrawCircle(static_cast<int>(rightJoyCenter.x), static_cast<int>(rightJoyCenter.y), joyRadius, Fade(RED, 0.10f));
        DrawCircleLines(static_cast<int>(rightJoyCenter.x), static_cast<int>(rightJoyCenter.y), joyRadius, Fade(RED, 0.55f));
        DrawCircle(static_cast<int>(rightJoyKnob.x), static_cast<int>(rightJoyKnob.y), 31.0f, Fade(WHITE, 0.28f));
        DrawCircleLines(static_cast<int>(rightJoyKnob.x), static_cast<int>(rightJoyKnob.y), 31.0f, Fade(WHITE, 0.8f));

        const Vector2 dashBtn{rightJoyCenter.x - 135.0f, rightJoyCenter.y - 60.0f};
        const Vector2 empBtn{rightJoyCenter.x - 60.0f, rightJoyCenter.y - 145.0f};
        DrawCircle(static_cast<int>(dashBtn.x), static_cast<int>(dashBtn.y), 28.0f, Fade(WHITE, 0.24f));
        DrawCircleLines(static_cast<int>(dashBtn.x), static_cast<int>(dashBtn.y), 28.0f, WHITE);
        DrawText("DASH", static_cast<int>(dashBtn.x) - 24, static_cast<int>(dashBtn.y) - 7, 14, WHITE);
        DrawCircle(static_cast<int>(empBtn.x), static_cast<int>(empBtn.y), 28.0f, Fade(SKYBLUE, 0.35f));
        DrawCircleLines(static_cast<int>(empBtn.x), static_cast<int>(empBtn.y), 28.0f, WHITE);
        DrawText("EMP", static_cast<int>(empBtn.x) - 16, static_cast<int>(empBtn.y) - 7, 14, WHITE);
    }

    void drawCutscene() {
        const StoryChapter& chap = campaign[save.currentChapter];
        const auto& lines = playingIntro ? chap.introDialogue : chap.outroDialogue;
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.68f));
        DrawRectangle(25, screenH - 235, screenW - 50, 205, Color{5, 12, 28, 235});
        DrawRectangleLines(25, screenH - 235, screenW - 50, 205, Fade(chap.envColor, 0.9f));
        DrawText(chap.title.c_str(), 48, screenH - 218, 20, chap.envColor);
        if (dialogueIndex < lines.size()) {
            const DialogueLine& line = lines[dialogueIndex];
            DrawText(line.speaker.c_str(), 48, screenH - 180, 23, line.speakerColor);
            const size_t count = std::min(visibleChars, line.text.size());
            const std::string visible = line.text.substr(0, count);
            DrawText(visible.c_str(), 48, screenH - 143, 20, RAYWHITE);
            DrawText("TAP / SPACE / ENTER TO CONTINUE", screenW - 355, screenH - 60, 14, LIGHTGRAY);
        }
    }

    void drawUpgradeHub() {
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.78f));
        DrawText("UPGRADE TERMINAL", screenW / 2 - MeasureText("UPGRADE TERMINAL", 36) / 2, 70, 36, SKYBLUE);
        DrawText(TextFormat("CREDITS AVAILABLE: %d", save.credits), screenW / 2 - 130, 126, 22, GOLD);
        const int cx = screenW / 2;
        const Rectangle weapon{static_cast<float>(cx - 320), 220, 200, 150};
        const Rectangle armor{static_cast<float>(cx - 100), 220, 200, 150};
        const Rectangle emp{static_cast<float>(cx + 120), 220, 200, 150};
        DrawRectangleRec(weapon, Color{12, 35, 55, 245}); DrawRectangleLinesEx(weapon, 2, SKYBLUE);
        DrawRectangleRec(armor, Color{26, 36, 28, 245}); DrawRectangleLinesEx(armor, 2, GREEN);
        DrawRectangleRec(emp, Color{40, 23, 52, 245}); DrawRectangleLinesEx(emp, 2, PURPLE);
        DrawText(TextFormat("WEAPON T%d", save.weaponTier), cx - 302, 248, 20, SKYBLUE);
        DrawText("+ DAMAGE", cx - 302, 283, 16, WHITE);
        DrawText("COST: 150", cx - 302, 330, 16, GOLD);
        DrawText(TextFormat("ARMOR T%d", save.armorTier), cx - 82, 248, 20, GREEN);
        DrawText("+ MAX HULL", cx - 82, 283, 16, WHITE);
        DrawText("COST: 150", cx - 82, 330, 16, GOLD);
        DrawText(TextFormat("EMP T%d", save.empTier), cx + 138, 248, 20, PURPLE);
        DrawText("+ NOVA DAMAGE", cx + 138, 283, 16, WHITE);
        DrawText("COST: 150", cx + 138, 330, 16, GOLD);

        const Rectangle next{static_cast<float>(cx - 140), 430, 280, 65};
        DrawRectangleRec(next, Color{15, 60, 85, 255});
        DrawRectangleLinesEx(next, 2, SKYBLUE);
        const std::string nextText = (save.currentChapter + 1 < static_cast<int>(campaign.size())) ? "NEXT CHAPTER  >" : "COMPLETE CAMPAIGN";
        DrawText(nextText.c_str(), cx - MeasureText(nextText.c_str(), 22) / 2, 451, 22, WHITE);
        DrawText("Tap an upgrade to purchase it. Tap NEXT CHAPTER to continue.", screenW / 2 - 255, 530, 16, LIGHTGRAY);
        DrawText("Desktop: click buttons  |  Mobile: tap buttons", screenW / 2 - 190, 560, 16, GRAY);
    }

    void drawTitle() {
        DrawRectangle(0, 0, screenW, screenH, Color{4, 8, 20, 255});
        for (int i = 0; i < 28; ++i) {
            const int x = (i * 173 + 31) % std::max(1, screenW);
            const int y = (i * 97 + 19) % std::max(1, screenH);
            DrawCircle(x, y, (i % 3 == 0) ? 2.5f : 1.2f, Fade(SKYBLUE, 0.5f));
        }
        const std::string title = "CHRONO PROTOCOL";
        const std::string subtitle = "E C L I P S E";
        DrawText(title.c_str(), screenW / 2 - MeasureText(title.c_str(), 48) / 2, screenH / 2 - 130, 48, SKYBLUE);
        DrawText(subtitle.c_str(), screenW / 2 - MeasureText(subtitle.c_str(), 26) / 2, screenH / 2 - 62, 26, RED);
        DrawText("A 3D STORY ACTION RPG", screenW / 2 - MeasureText("A 3D STORY ACTION RPG", 20) / 2, screenH / 2 - 18, 20, LIGHTGRAY);
        const std::string continueText = "TAP OR PRESS ENTER TO DEPLOY";
        DrawText(continueText.c_str(), screenW / 2 - MeasureText(continueText.c_str(), 20) / 2, screenH / 2 + 70, 20, WHITE);
        DrawText(TextFormat("CHAPTER %d / %d", save.currentChapter + 1, static_cast<int>(campaign.size())), screenW / 2 - 75, screenH / 2 + 112, 16, GRAY);
        if (hp <= 0.0f) DrawText("VEX-9 DOWN — DEPLOY TO RETRY", screenW / 2 - 170, screenH / 2 + 150, 18, RED);
        DrawText("WASD / ARROWS: MOVE     MOUSE: AIM + FIRE     SPACE: DASH     E: EMP", 24, screenH - 42, 16, LIGHTGRAY);
    }

    void drawEnding() {
        DrawRectangle(0, 0, screenW, screenH, Color{4, 8, 20, 255});
        DrawText("ECLIPSE PROTOCOL COMPLETE", screenW / 2 - MeasureText("ECLIPSE PROTOCOL COMPLETE", 34) / 2, screenH / 2 - 90, 34, SKYBLUE);
        DrawText("The orbital grid is shutting down. Humanity gets another dawn.", screenW / 2 - MeasureText("The orbital grid is shutting down. Humanity gets another dawn.", 18) / 2, screenH / 2 - 25, 18, WHITE);
        DrawText("THANK YOU FOR PLAYING", screenW / 2 - MeasureText("THANK YOU FOR PLAYING", 24) / 2, screenH / 2 + 35, 24, GOLD);
        DrawText("TAP OR PRESS ENTER TO RETURN TO TITLE", screenW / 2 - MeasureText("TAP OR PRESS ENTER TO RETURN TO TITLE", 18) / 2, screenH / 2 + 95, 18, LIGHTGRAY);
    }

    void draw() {
        if (!IsWindowReady()) return;
        BeginDrawing();
        ClearBackground(BLACK);
        if (state == ScreenState::TITLE) {
            drawTitle();
        } else if (state == ScreenState::ENDING) {
            drawEnding();
        } else {
            drawWorld();
            if (state == ScreenState::GAMEPLAY) {
                drawBars();
                drawMobileControls();
                if (IsKeyDown(KEY_TAB)) {
                    DrawRectangle(0, screenH - 70, screenW, 70, Fade(BLACK, 0.75f));
                    DrawText("MOVE: WASD / LEFT STICK   AIM + FIRE: MOUSE / RIGHT STICK   DASH: SPACE / DASH BUTTON   EMP: E / Q / EMP BUTTON", 20, screenH - 45, 15, WHITE);
                }
            } else if (state == ScreenState::CUTSCENE) {
                drawCutscene();
            } else if (state == ScreenState::UPGRADE_HUB) {
                drawUpgradeHub();
            }
        }
        EndDrawing();
    }

    void close() {
        if (glowTex.id != 0) UnloadTexture(glowTex);
        if (playerModel.meshCount > 0) UnloadModel(playerModel);
        if (droneModel.meshCount > 0) UnloadModel(droneModel);
        if (bossModel.meshCount > 0) UnloadModel(bossModel);
        if (pillarModel.meshCount > 0) UnloadModel(pillarModel);
        if (IsWindowReady()) CloseWindow();
    }
};

int main() {
    ChronoProtocolGame game;
    game.init();
    while (!WindowShouldClose()) {
        game.update(GetFrameTime());
        game.draw();
    }
    game.close();
    return 0;
}
