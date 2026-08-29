#ifndef GAME_H
#define GAME_H

#include "chai3d.h"
#include "Turret.h"
#include "Bullet.h"
#include "Monster.h"
#include <vector>
#include <string>

enum GameState {
    STATE_PLACEMENT,
    STATE_PLAYING,
    STATE_GAME_OVER
};

class Game {
public:
    Game();
    ~Game();

    void init(chai3d::cWorld* world, chai3d::cCamera* camera, const std::string& assetsDir);
    void update(double dt);
    void processInput(bool keyA, bool keyD, bool keyW, bool keyS, bool keySpace, bool keyR, bool keyP, bool keyN);
    void cleanup();

    // Haptics simulation callback
    void updateHaptics(chai3d::cGenericHapticDevicePtr hapticDevice);

    GameState getState() const { return m_state; }

private:
    void setupScene();
    void setupUI();
    void randomizePlacement();
    void randomizePlacementOnlyMonster();
    void fireBullet();
    void checkCollisions();
    void updateUI();
    void resetGame();

    chai3d::cWorld* m_world;
    chai3d::cCamera* m_camera;
    std::string m_assetsDir;

    Turret m_turret;
    Monster m_monster;
    std::vector<Bullet*> m_bullets;

    chai3d::cVector3d m_turretPos;
    chai3d::cVector3d m_monsterSpawnPos;
    chai3d::cVector3d m_destinationPos;

    // Ground plane & destination visual objects
    chai3d::cMesh* m_groundMesh;
    chai3d::cMesh* m_destinationLineMesh;

    // Faraway Slender Target Panels & Laser Impact Hit Dot
    struct TargetPanel {
        chai3d::cMesh* mesh;
        chai3d::cVector3d position;
        chai3d::cMatrix3d rotation;
        chai3d::cVector3d normal;
        bool active;
    };
    std::vector<TargetPanel> m_targetPanels;
    chai3d::cMesh* m_hitDotMesh;
    void updateLaserHitDot();

    // UI elements
    chai3d::cFontPtr m_fontTitle;
    chai3d::cFontPtr m_fontHUD;
    chai3d::cLabel* m_lblTitle;
    chai3d::cLabel* m_lblState;
    chai3d::cLabel* m_lblScore;
    chai3d::cLabel* m_lblTimer;
    chai3d::cLabel* m_lblTurretInfo;
    chai3d::cLabel* m_lblControls;

    // Game Over Overlay Panel & Labels
    chai3d::cPanel* m_pnlGameOver;
    chai3d::cLabel* m_lblGameOverTitle;
    chai3d::cLabel* m_lblGameOverScore;
    chai3d::cLabel* m_lblGameOverKills;
    chai3d::cLabel* m_lblGameOverRestart;

    // Bottom-Right Telemetry Panel & Labels (CHAI3D & Haptics)
    chai3d::cPanel* m_pnlTelemetry;
    chai3d::cLabel* m_lblTelemHeader;
    chai3d::cLabel* m_lblTelemDevice;
    chai3d::cLabel* m_lblTelemForce;
    chai3d::cLabel* m_lblTelemAngles;
    chai3d::cLabel* m_lblTelemDistance;
    chai3d::cLabel* m_lblTelemMuzzle;

    // Gameplay variables
    GameState m_state;
    double m_gameTimer;       // 60 seconds countdown
    int m_score;
    int m_monstersKilled;
    bool m_spaceKeyPrev;
    bool m_rKeyPrev;
    bool m_pKeyPrev;
    bool m_nKeyPrev;
    std::string m_endReason;

    // Haptics feedback & telemetry variables
    double m_recoilTimer;
    chai3d::cVector3d m_recoilDir;
    double m_hitVibrationTimer;
    chai3d::cVector3d m_currentHapticForce;
    std::string m_hapticDeviceName;
};

#endif // GAME_H
