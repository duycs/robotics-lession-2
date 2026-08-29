#include "Game.h"
#include <cmath>
#include <iostream>

using namespace chai3d;
using namespace std;

Game::Game() :
    m_world(nullptr),
    m_camera(nullptr),
    m_groundMesh(nullptr),
    m_destinationLineMesh(nullptr),
    m_hitDotMesh(nullptr),
    m_lblTitle(nullptr),
    m_lblState(nullptr),
    m_lblScore(nullptr),
    m_lblTimer(nullptr),
    m_lblTurretInfo(nullptr),
    m_lblControls(nullptr),
    m_pnlGameOver(nullptr),
    m_lblGameOverTitle(nullptr),
    m_lblGameOverScore(nullptr),
    m_lblGameOverKills(nullptr),
    m_lblGameOverRestart(nullptr),
    m_pnlTelemetry(nullptr),
    m_lblTelemHeader(nullptr),
    m_lblTelemDevice(nullptr),
    m_lblTelemForce(nullptr),
    m_lblTelemAngles(nullptr),
    m_lblTelemDistance(nullptr),
    m_lblTelemMuzzle(nullptr),
    m_state(STATE_PLACEMENT),
    m_gameTimer(60.0),
    m_score(0),
    m_monstersKilled(0),
    m_spaceKeyPrev(false),
    m_rKeyPrev(false),
    m_pKeyPrev(false),
    m_recoilTimer(0.0),
    m_hitVibrationTimer(0.0),
    m_currentHapticForce(0, 0, 0),
    m_hapticDeviceName("")
{
}

Game::~Game() {
    cleanup();
}

void Game::cleanup() {
    m_turret.cleanup();
    m_monster.cleanup();

    for (auto b : m_bullets) {
        if (b) {
            b->cleanup();
            delete b;
        }
    }
    m_bullets.clear();

    if (m_groundMesh) {
        if (m_world) m_world->removeChild(m_groundMesh);
        delete m_groundMesh;
        m_groundMesh = nullptr;
    }

    if (m_destinationLineMesh) {
        if (m_world) m_world->removeChild(m_destinationLineMesh);
        delete m_destinationLineMesh;
        m_destinationLineMesh = nullptr;
    }
}

void Game::init(cWorld* world, cCamera* camera, const string& assetsDir) {
    m_world = world;
    m_camera = camera;
    m_assetsDir = assetsDir;

    setupScene();

    m_turret.init(m_world, m_assetsDir);
    m_monster.init(m_world);

    // Initialize 20 bullet instances in bullet pool
    for (int i = 0; i < 20; ++i) {
        Bullet* b = new Bullet();
        b->init(m_world);
        m_bullets.push_back(b);
    }

    setupUI();

    resetGame();
}

void Game::setupScene() {
    // 3D Ground map mesh centered at origin
    m_groundMesh = new cMesh();
    cCreateMap(m_groundMesh, 32.0, 32.0, 6, 6, cVector3d(0, 4, -0.01));
    cMaterial groundMat;
    groundMat.m_ambient.set(0.08f, 0.12f, 0.15f, 1.0f);
    groundMat.m_diffuse.set(0.15f, 0.22f, 0.28f, 1.0f);
    groundMat.m_specular.set(0.2f, 0.2f, 0.2f, 1.0f);
    groundMat.setShininess(20);
    m_groundMesh->setMaterial(groundMat);
    m_world->addChild(m_groundMesh);
    m_destinationLineMesh = nullptr; // No green line

    // Create 4 Perfect 3D Mirror Cubes embedded in ground floor plane (1.4m x 1.4m x 1.4m)
    cMaterial mirrorMat;
    mirrorMat.m_ambient.set(0.3f, 0.4f, 0.5f, 1.0f);
    mirrorMat.m_diffuse.set(0.5f, 0.8f, 1.0f, 1.0f);
    mirrorMat.m_specular.set(1.0f, 1.0f, 1.0f, 1.0f);
    mirrorMat.setShininess(120);

    cVector3d initPos[4] = {
        cVector3d( 6.0, 14.0, 0.60), // Panel 0: Top-Right (Sunken 10cm into ground floor plane)
        cVector3d(-6.0, 14.0, 0.60), // Panel 1: Top-Left (Reflection point from Panel 0)
        cVector3d(-6.0,  4.0, 0.60), // Panel 2: Mid-Left (Reflection point from Panel 1)
        cVector3d( 6.0,  4.0, 0.60)  // Panel 3: Mid-Right (Reflection point from Panel 2)
    };

    double yawAngles[4] = {
        cDegToRad(-40.0), // Angled to reflect towards Panel 1
        cDegToRad( 45.0), // Angled to reflect towards Panel 2
        cDegToRad(-45.0), // Angled to reflect towards Panel 3
        cDegToRad( 45.0)  // Angled to reflect down battlefield
    };

    cVector3d normals[4] = {
        cNormalize(cVector3d(-0.642, -0.766, 0.0)),
        cNormalize(cVector3d( 0.707, -0.707, 0.0)),
        cNormalize(cVector3d( 0.707,  0.707, 0.0)),
        cNormalize(cVector3d(-0.707, -0.707, 0.0))
    };

    for (int i = 0; i < 4; ++i) {
        TargetPanel panel;
        panel.mesh = new cMesh();
        cCreateBox(panel.mesh, 1.4, 1.4, 1.4, initPos[i]);
        panel.mesh->setMaterial(mirrorMat);

        cMatrix3d R_box;
        R_box.setExtrinsicEulerRotationDeg(0, 0, cRadToDeg(yawAngles[i]), C_EULER_ORDER_XYZ);
        panel.mesh->setLocalRot(R_box);

        panel.position = initPos[i];
        panel.rotation = R_box;
        panel.normal = normals[i];
        panel.active = true;
        m_world->addChild(panel.mesh);
        m_targetPanels.push_back(panel);
    }

    // Glowing Neon Red/Magenta Laser Impact Hit Dot (12cm sphere)
    m_hitDotMesh = new cMesh();
    cCreateSphere(m_hitDotMesh, 0.12, 24, 24);
    cMaterial hitMat;
    hitMat.m_ambient.set(1.0f, 0.0f, 0.2f, 1.0f);
    hitMat.m_diffuse.set(1.0f, 0.1f, 0.3f, 1.0f);
    hitMat.m_specular.set(1.0f, 0.8f, 0.9f, 1.0f);
    hitMat.m_emission.set(1.0f, 0.2f, 0.3f, 1.0f);
    m_hitDotMesh->setMaterial(hitMat);
    m_hitDotMesh->setShowEnabled(false);
    m_world->addChild(m_hitDotMesh);
}

void Game::setupUI() {
    m_fontTitle = NEW_CFONT_CALIBRI_28();
    m_fontHUD = NEW_CFONT_CALIBRI_20();

    // Title label
    m_lblTitle = new cLabel(m_fontTitle);
    m_lblTitle->setText("3D TOWER DEFENSE (CHAI3D)");
    m_lblTitle->m_fontColor.set(1.0f, 0.85f, 0.2f, 1.0f);
    m_lblTitle->setLocalPos(20, 730);
    m_camera->m_frontLayer->addChild(m_lblTitle);

    // Game state label
    m_lblState = new cLabel(m_fontHUD);
    m_lblState->m_fontColor.set(0.2f, 1.0f, 0.5f, 1.0f);
    m_lblState->setLocalPos(20, 700);
    m_camera->m_frontLayer->addChild(m_lblState);

    // Score label
    m_lblScore = new cLabel(m_fontHUD);
    m_lblScore->m_fontColor.set(1.0f, 1.0f, 1.0f, 1.0f);
    m_lblScore->setLocalPos(20, 670);
    m_camera->m_frontLayer->addChild(m_lblScore);

    // Timer label
    m_lblTimer = new cLabel(m_fontHUD);
    m_lblTimer->m_fontColor.set(1.0f, 0.9f, 0.3f, 1.0f);
    m_lblTimer->setLocalPos(20, 640);
    m_camera->m_frontLayer->addChild(m_lblTimer);

    // Turret info label
    m_lblTurretInfo = new cLabel(m_fontHUD);
    m_lblTurretInfo->m_fontColor.set(0.7f, 0.85f, 1.0f, 1.0f);
    m_lblTurretInfo->setLocalPos(20, 610);
    m_camera->m_frontLayer->addChild(m_lblTurretInfo);

    // Controls guide label
    m_lblControls = new cLabel(m_fontHUD);
    m_lblControls->setText("Phim: [A]/[D] Xoay Base Yaw | [W]/[S] Xoay Gun Pitch | [SPACE] Ban dan | [N] Luot choi moi");
    m_lblControls->m_fontColor.set(0.8f, 0.8f, 0.8f, 1.0f);
    m_lblControls->setLocalPos(20, 20);
    m_camera->m_frontLayer->addChild(m_lblControls);

    // Game Over Summary Labels attached directly to front layer (NO dark panel background shapes)
    m_lblGameOverTitle = new cLabel(m_fontTitle);
    m_lblGameOverTitle->setShowEnabled(false);
    m_camera->m_frontLayer->addChild(m_lblGameOverTitle);

    m_lblGameOverScore = new cLabel(m_fontHUD);
    m_lblGameOverScore->m_fontColor.set(1.0f, 1.0f, 1.0f, 1.0f);
    m_lblGameOverScore->setShowEnabled(false);
    m_camera->m_frontLayer->addChild(m_lblGameOverScore);

    m_lblGameOverKills = new cLabel(m_fontHUD);
    m_lblGameOverKills->m_fontColor.set(0.8f, 0.9f, 1.0f, 1.0f);
    m_lblGameOverKills->setShowEnabled(false);
    m_camera->m_frontLayer->addChild(m_lblGameOverKills);

    m_lblGameOverRestart = new cLabel(m_fontHUD);
    m_lblGameOverRestart->setText("AN Phim [N] de BAT DAU LUOT CHOI MOI");
    m_lblGameOverRestart->m_fontColor.set(0.2f, 1.0f, 0.5f, 1.0f);
    m_lblGameOverRestart->setShowEnabled(false);
    m_camera->m_frontLayer->addChild(m_lblGameOverRestart);

    // Bottom-Right Telemetry Labels attached directly to front layer (NO dark panel background shapes)
    m_lblTelemHeader = new cLabel(m_fontHUD);
    m_lblTelemHeader->setText("TELEMETRY (CHAI3D / HAPTICS)");
    m_lblTelemHeader->m_fontColor.set(0.3f, 0.9f, 1.0f, 1.0f);
    m_camera->m_frontLayer->addChild(m_lblTelemHeader);

    m_lblTelemDevice = new cLabel(m_fontHUD);
    m_lblTelemDevice->m_fontColor.set(0.8f, 0.9f, 0.95f, 1.0f);
    m_camera->m_frontLayer->addChild(m_lblTelemDevice);

    m_lblTelemForce = new cLabel(m_fontHUD);
    m_lblTelemForce->m_fontColor.set(1.0f, 0.85f, 0.3f, 1.0f);
    m_camera->m_frontLayer->addChild(m_lblTelemForce);

    m_lblTelemAngles = new cLabel(m_fontHUD);
    m_lblTelemAngles->m_fontColor.set(0.4f, 1.0f, 0.6f, 1.0f);
    m_camera->m_frontLayer->addChild(m_lblTelemAngles);

    m_lblTelemDistance = new cLabel(m_fontHUD);
    m_lblTelemDistance->m_fontColor.set(0.9f, 0.7f, 1.0f, 1.0f);
    m_camera->m_frontLayer->addChild(m_lblTelemDistance);

    m_lblTelemMuzzle = new cLabel(m_fontHUD);
    m_lblTelemMuzzle->m_fontColor.set(0.7f, 0.85f, 1.0f, 1.0f);
    m_camera->m_frontLayer->addChild(m_lblTelemMuzzle);
}

void Game::randomizePlacement() {
    // 1. Turret Base centered close to first-person view: (0, -7, 0)
    m_turretPos.set(0.0, -7.0, 0.0);
    m_turret.setPosition(m_turretPos);

    m_destinationPos.set(0.0, -7.0, 0.0);

    // 2. Monster Spawn Point randomly across the opposite side / top area: X in [-8, 8], Y in [11, 14]
    double mx = -8.0 + (rand() % 1600) / 100.0;
    double my = 11.0 + (rand() % 300) / 100.0;
    m_monsterSpawnPos.set(mx, my, 0.55);

    // Target position for monster moves straight towards turret at Y = -7.0 (past the gun)
    cVector3d monsterTarget(mx * 0.6, -7.0, 0.55);
    m_monster.spawn(m_monsterSpawnPos, monsterTarget, 0.10, 0.001);

    // Reset all 4 slender target panels at reflection circuit points
    cVector3d initPos[4] = {
        cVector3d( 6.0, 14.0, 0.60),
        cVector3d(-6.0, 14.0, 0.60),
        cVector3d(-6.0,  4.0, 0.60),
        cVector3d( 6.0,  4.0, 0.60)
    };

    for (size_t i = 0; i < m_targetPanels.size() && i < 4; ++i) {
        m_targetPanels[i].active = true;
        m_targetPanels[i].position = initPos[i];
        if (m_targetPanels[i].mesh) {
            m_targetPanels[i].mesh->setLocalPos(initPos[i]);
            m_targetPanels[i].mesh->setShowEnabled(true);
        }
    }

    // Default initial gun aiming direction: Point laser beam directly at Panel 0!
    cVector3d reflectorTarget(6.0, 14.0, 0.60);
    m_turret.aimAt(reflectorTarget);
}

void Game::updateLaserHitDot() {
    if (m_hitDotMesh) {
        m_hitDotMesh->setShowEnabled(false);
    }
}

void Game::randomizePlacementOnlyMonster() {
    // Monster Spawn Point randomly across the opposite side / top area: X in [-8, 8], Y in [11, 14]
    double mx = -8.0 + (rand() % 1600) / 100.0;
    double my = 11.0 + (rand() % 300) / 100.0;
    m_monsterSpawnPos.set(mx, my, 0.55);

    cVector3d monsterTarget(mx * 0.6, -7.0, 0.55);
    m_monster.spawn(m_monsterSpawnPos, monsterTarget, 0.10, 0.001);
}

void Game::fireBullet() {
    // Find an inactive bullet in pool
    for (auto b : m_bullets) {
        if (!b->isActive()) {
            cVector3d tipPos = m_turret.getBarrelTipPosition();
            cVector3d fireDir = m_turret.getFiringDirection();

            b->spawn(tipPos, fireDir, 32.0); // 32 units/s linear speed

            // Trigger recoil haptic feedback force
            m_recoilDir = -fireDir;
            m_recoilTimer = 0.15; // 0.15 seconds recoil force impulse
            break;
        }
    }
}

void Game::processInput(bool keyA, bool keyD, bool keyW, bool keyS, bool keySpace, bool keyR, bool keyP, bool keyN) {
    double dtKey = 0.016; // key step for smoothness

    // Press N key at any time to start a new game round immediately!
    if (keyN && !m_nKeyPrev) {
        resetGame();
    }

    if (m_state == STATE_PLAYING) {
        // Turret Base Yaw control: A = Left (+Yaw), D = Right (-Yaw) - Slower & silky smooth rotation
        if (keyA) m_turret.updateYaw( 0.45 * dtKey);
        if (keyD) m_turret.updateYaw(-0.45 * dtKey);

        // Turret Gun Pitch control: W = Up (+Pitch), S = Down (-Pitch) - Slower & silky smooth rotation
        if (keyW) m_turret.updatePitch( 0.35 * dtKey);
        if (keyS) m_turret.updatePitch(-0.35 * dtKey);

        // Shoot linear bullet with SPACE key
        if (keySpace && !m_spaceKeyPrev) {
            fireBullet();
        }
    }
    else if (m_state == STATE_GAME_OVER) {
        // Restart game with N key only
        if (keyN && !m_nKeyPrev) {
            resetGame();
        }
    }

    m_spaceKeyPrev = keySpace;
    m_rKeyPrev = keyR;
    m_pKeyPrev = keyP;
    m_nKeyPrev = keyN;
}

void Game::update(double dt) {
    if (dt > 0.1) dt = 0.1; // clamp delta time

    if (m_recoilTimer > 0.0) m_recoilTimer -= dt;
    if (m_hitVibrationTimer > 0.0) m_hitVibrationTimer -= dt;

    if (m_state == STATE_PLAYING) {
        // Match duration timer
        m_gameTimer += dt;

        // Update monster position & acceleration
        m_monster.update(dt);

        // Check if monster reached Green Line in front of gun
        if (m_monster.hasReachedDestination()) {
            m_state = STATE_GAME_OVER;
            m_endReason = "THUA CUOC! (Quai thu da toi Vach Xanh truoc sung)";
        }

        // Update active bullets linear movement
        for (auto b : m_bullets) {
            if (b->isActive()) {
                b->update(dt);
            }
        }

        checkCollisions();
    }

    // Dynamic tactical laser flickering effect update & laser target impact hit dot
    m_turret.updateLaserEffect(m_gameTimer);
    updateLaserHitDot();

    updateUI();
}

void Game::checkCollisions() {
    // 1. Check bullet collision with Target Cubes (1-Shot Instant Destruction!)
    for (auto& panel : m_targetPanels) {
        if (!panel.active || !panel.mesh) continue;

        cVector3d boxPos = panel.position;
        cMatrix3d R_inv = cTranspose(panel.rotation);

        for (auto b : m_bullets) {
            if (b->isActive()) {
                cVector3d p1 = b->getPrevPosition();
                cVector3d p2 = b->getPosition();

                bool hit = false;
                for (int step = 0; step <= 10; ++step) {
                    double t = step / 10.0;
                    cVector3d pSample = p1 + t * (p2 - p1);
                    cVector3d pLocal = R_inv * (pSample - boxPos);

                    // Cube half-width (0.70m) + Bullet radius (0.18m) + Margin (0.12m) = 1.00m
                    if (std::abs(pLocal.x()) <= 1.00 &&
                        std::abs(pLocal.y()) <= 1.00 &&
                        std::abs(pLocal.z()) <= 1.00)
                    {
                        hit = true;
                        break;
                    }
                }

                if (hit) {
                    // Bullet hits the square cube! Destroy cube ON 1 SINGLE SHOT!
                    b->deactivate();
                    panel.active = false;
                    panel.mesh->setShowEnabled(false);

                    m_score += 500; // Bonus +500 points!
                    m_hitVibrationTimer = 0.30;
                    break;
                }
            }
        }
    }

    // 2. Check bullet collision with Monster
    if (!m_monster.isActive()) return;

    cVector3d monsterPos = m_monster.getPosition();
    double monsterR = m_monster.getRadius();

    for (auto b : m_bullets) {
        if (b->isActive()) {
            cVector3d p1 = b->getPrevPosition();
            cVector3d p2 = b->getPosition();
            double bulletR = b->getRadius();

            // Distance from monster center to bullet trajectory line segment (p1 -> p2)
            cVector3d v = p2 - p1;
            cVector3d w = monsterPos - p1;
            double c1 = w.dot(v);
            double c2 = v.dot(v);
            double dist = 0.0;
            if (c1 <= 0) {
                dist = (monsterPos - p1).length();
            } else if (c2 <= c1) {
                dist = (monsterPos - p2).length();
            } else {
                double b_val = c1 / c2;
                cVector3d pb = p1 + v * b_val;
                dist = (monsterPos - pb).length();
            }

            double hitThreshold = monsterR + bulletR + 0.25; // 3D hit volume threshold

            if (dist <= hitThreshold) {
                // Bullet hit monster! Recycle bullet, deal 1 damage
                b->deactivate();
                m_score += 10;
                m_hitVibrationTimer = 0.15;

                bool killed = m_monster.takeDamage(1);
                if (killed) {
                    // Monster died on 10th hit! Match Victory!
                    m_score += 100;
                    m_monstersKilled++;
                    m_hitVibrationTimer = 0.35;

                    m_state = STATE_GAME_OVER;
                    m_endReason = "THANG CUOC! (Da tieu diet Quai thu sau 10 phat)";
                }
                break;
            }
        }
    }
}

void Game::updateUI() {
    int viewW = m_camera->getDisplayWidth();
    if (viewW <= 0) viewW = 800;

    if (m_state == STATE_PLACEMENT) {
        m_lblState->setText("GIAI DOAN 1: PLACEMENT (Nhan [N]/[R] Bat dau Combat)");
        m_lblState->m_fontColor.set(1.0f, 0.7f, 0.2f, 1.0f);
        m_lblGameOverTitle->setShowEnabled(false);
        m_lblGameOverScore->setShowEnabled(false);
        m_lblGameOverKills->setShowEnabled(false);
        m_lblGameOverRestart->setShowEnabled(false);
    }
    else if (m_state == STATE_PLAYING) {
        m_lblState->setText("COMBAT: Ban trung Quai thu 10 phat de THANG | Toi U sung thi THUA");
        m_lblState->m_fontColor.set(0.2f, 1.0f, 0.4f, 1.0f);
        m_lblGameOverTitle->setShowEnabled(false);
        m_lblGameOverScore->setShowEnabled(false);
        m_lblGameOverKills->setShowEnabled(false);
        m_lblGameOverRestart->setShowEnabled(false);
    }
    else if (m_state == STATE_GAME_OVER) {
        m_lblState->setText("TRANG THAI: GAME OVER");
        m_lblState->m_fontColor.set(1.0f, 0.3f, 0.3f, 1.0f);

        // Show Game Over Summary Labels directly on 2D front layer
        m_lblGameOverTitle->setShowEnabled(true);
        m_lblGameOverScore->setShowEnabled(true);
        m_lblGameOverKills->setShowEnabled(true);
        m_lblGameOverRestart->setShowEnabled(true);

        if (m_endReason.find("THANG") != string::npos) {
            m_lblGameOverTitle->setText("=== NGUOI CHOI THANG! (VICTORY) ===");
            m_lblGameOverTitle->m_fontColor.set(0.2f, 1.0f, 0.4f, 1.0f);

            m_lblGameOverScore->setText("Ban da ban trung 10 vien dan -> Quai thu da THUA!");
            m_lblGameOverScore->m_fontColor.set(1.0f, 1.0f, 1.0f, 1.0f);

            char statsBuf[128];
            snprintf(statsBuf, sizeof(statsBuf), "Tong diem so: %d | Thoi gian: %.1fs | Quai da diet: %d", m_score, m_gameTimer, m_monstersKilled);
            m_lblGameOverKills->setText(statsBuf);
            m_lblGameOverKills->m_fontColor.set(0.4f, 0.9f, 1.0f, 1.0f);

            m_lblGameOverRestart->setText("-> AN PHIM [N] DE BAT DAU LUOT CHOI MOI <-");
            m_lblGameOverRestart->m_fontColor.set(1.0f, 0.9f, 0.2f, 1.0f);
        } else {
            m_lblGameOverTitle->setText("=== NGUOI CHOI THUA! (DEFEAT) ===");
            m_lblGameOverTitle->m_fontColor.set(1.0f, 0.2f, 0.2f, 1.0f);

            m_lblGameOverScore->setText("Quai thu da di qua U sung -> Nguoi choi THUA!");
            m_lblGameOverScore->m_fontColor.set(1.0f, 0.6f, 0.2f, 1.0f);

            char statsBuf[128];
            snprintf(statsBuf, sizeof(statsBuf), "Tong diem so: %d | Thoi gian: %.1fs | Quai da diet: %d", m_score, m_gameTimer, m_monstersKilled);
            m_lblGameOverKills->setText(statsBuf);
            m_lblGameOverKills->m_fontColor.set(1.0f, 0.8f, 0.8f, 1.0f);

            m_lblGameOverRestart->setText("-> AN PHIM [N] DE BAT DAU LUOT CHOI MOI <-");
            m_lblGameOverRestart->m_fontColor.set(0.2f, 1.0f, 0.5f, 1.0f);
        }

        // Dynamically center each label string individually in exact middle of screen:
        int winW = m_camera->getDisplayWidth();
        if (winW <= 0) winW = 1280;

        int titleW   = (int)m_lblGameOverTitle->getTextWidth();
        int scoreW   = (int)m_lblGameOverScore->getTextWidth();
        int killsW   = (int)m_lblGameOverKills->getTextWidth();
        int restartW = (int)m_lblGameOverRestart->getTextWidth();

        int titleX   = (winW - titleW) / 2;
        int scoreX   = (winW - scoreW) / 2;
        int killsX   = (winW - killsW) / 2;
        int restartX = (winW - restartW) / 2;

        if (titleX < 20) titleX = 20;
        if (scoreX < 20) scoreX = 20;
        if (killsX < 20) killsX = 20;
        if (restartX < 20) restartX = 20;

        m_lblGameOverTitle->setLocalPos(titleX, 420);
        m_lblGameOverScore->setLocalPos(scoreX, 360);
        m_lblGameOverKills->setLocalPos(killsX, 310);
        m_lblGameOverRestart->setLocalPos(restartX, 250);
    }

    m_lblScore->setText("Diem so: " + to_string(m_score) + " | Quai thu HP: " + to_string(m_monster.getHealth()) + "/10 | Da diet: " + to_string(m_monstersKilled));
    
    char timerStr[64];
    snprintf(timerStr, sizeof(timerStr), "Thoi gian man choi: %.1f s", m_gameTimer);
    m_lblTimer->setText(timerStr);

    char turretStr[128];
    snprintf(turretStr, sizeof(turretStr), "Base Yaw: %.1f deg (L:120 deg) | Gun Pitch: %.1f deg (L:60 deg) | Speed: %.2f",
             m_turret.getYawDeg(), m_turret.getPitchDeg(), m_monster.getCurrentSpeed());
    m_lblTurretInfo->setText(turretStr);

    // Align all telemetry text labels neatly on the LEFT side of the screen at X = 20:
    int panelX = 20;

    m_lblTelemHeader->setLocalPos(panelX, 545);
    m_lblTelemDevice->setLocalPos(panelX, 515);
    m_lblTelemForce->setLocalPos(panelX, 485);
    m_lblTelemAngles->setLocalPos(panelX, 455);
    m_lblTelemDistance->setLocalPos(panelX, 425);
    m_lblTelemMuzzle->setLocalPos(panelX, 395);

    // 1. Device Info
    if (m_hapticDeviceName.empty()) {
        m_lblTelemDevice->setText("Thiet bi Haptic: Vir. Force Loop (~1000 Hz)");
    } else {
        m_lblTelemDevice->setText("Thiet bi Haptic: " + m_hapticDeviceName + " (~1000 Hz)");
    }

    // 2. Haptic Force Vector
    char forceStr[128];
    snprintf(forceStr, sizeof(forceStr), "Luc Haptic F: [%.1f, %.1f, %.1f] N (|F|=%.1f N)",
             m_currentHapticForce.x(), m_currentHapticForce.y(), m_currentHapticForce.z(),
             m_currentHapticForce.length());
    m_lblTelemForce->setText(forceStr);

    // 3. 2-DOF Turret Angles & Joint Limit Status
    char angleStr[128];
    string limitTag = "NORMAL";
    if (std::abs(m_turret.getYawDeg()) >= 59.0 || std::abs(m_turret.getPitchDeg()) >= 29.0) {
        limitTag = "SPRING LIMIT!";
    }
    snprintf(angleStr, sizeof(angleStr), "Goc 2-DOF: Yaw:%.1f deg | Pitch:%.1f deg (%s)",
             m_turret.getYawDeg(), m_turret.getPitchDeg(), limitTag.c_str());
    m_lblTelemAngles->setText(angleStr);

    // 4. Spatial Target Distance & Monster Velocity
    cVector3d tipPos = m_turret.getBarrelTipPosition();
    cVector3d monsterPos = m_monster.getPosition();
    double distToTarget = (monsterPos - tipPos).length();
    char distStr[128];
    snprintf(distStr, sizeof(distStr), "Khoang cach Quai: %.2f m | V_quai: %.2f m/s",
             distToTarget, m_monster.getCurrentSpeed());
    m_lblTelemDistance->setText(distStr);

    // 5. Muzzle 3D Coordinates
    char muzzleStr[128];
    snprintf(muzzleStr, sizeof(muzzleStr), "Toa do Muzzle 3D: [%.2f, %.2f, %.2f] m",
             tipPos.x(), tipPos.y(), tipPos.z());
    m_lblTelemMuzzle->setText(muzzleStr);
}

void Game::resetGame() {
    m_score = 0;
    m_monstersKilled = 0;
    m_gameTimer = 0.0;
    m_state = STATE_PLAYING; // Directly enter Combat mode!

    for (auto b : m_bullets) {
        b->deactivate();
    }

    randomizePlacement();
}

void Game::updateHaptics(cGenericHapticDevicePtr hapticDevice) {
    if (hapticDevice) {
        cVector3d force(0, 0, 0);

        // Recoil force feedback when shooting
        if (m_recoilTimer > 0.0) {
            force += m_recoilDir * 12.0 * (m_recoilTimer / 0.15);
        }

        // Hit vibration when killing/hitting monster
        if (m_hitVibrationTimer > 0.0) {
            double freq = 150.0;
            double vibe = std::sin(freq * m_hitVibrationTimer) * 8.0;
            force += cVector3d(vibe, vibe, vibe);
        }

        // Limit spring force when reaching Yaw / Pitch rotation limits
        double yaw = m_turret.getYawDeg();
        if (yaw <= -59.0) force += cVector3d( 8.0, 0, 0);
        if (yaw >=  59.0) force += cVector3d(-8.0, 0, 0);

        m_currentHapticForce = force;
        hapticDevice->setForce(force);

        cHapticDeviceInfo info = hapticDevice->getSpecifications();
        m_hapticDeviceName = info.m_modelName;
    } else {
        cVector3d force(0, 0, 0);
        if (m_recoilTimer > 0.0) {
            force += m_recoilDir * 12.0 * (m_recoilTimer / 0.15);
        }
        if (m_hitVibrationTimer > 0.0) {
            double freq = 150.0;
            double vibe = std::sin(freq * m_hitVibrationTimer) * 8.0;
            force += cVector3d(vibe, vibe, vibe);
        }
        m_currentHapticForce = force;
    }
}
