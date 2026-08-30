#include "Monster.h"
#include <cmath>

using namespace chai3d;

Monster::Monster() :
    m_world(nullptr),
    m_mesh(nullptr),
    m_position(0, 0, 0.55),
    m_spawnPos(0, 0, 0.55),
    m_targetPos(0, 0, 0.55),
    m_moveDir(0, 0, 0),
    m_baseSpeed(0.10),
    m_currentSpeed(0.10),
    m_acceleration(0.001),
    m_distanceTotal(1.0),
    m_radius(0.55),
    m_pulseTime(0.0),
    m_health(MAX_HEALTH),
    m_active(false),
    m_isBlue(false)
{
}

Monster::~Monster() {
    cleanup();
}

void Monster::cleanup() {
    if (m_mesh) {
        if (m_world) {
            m_world->removeChild(m_mesh);
        }
        delete m_mesh;
        m_mesh = nullptr;
    }
}

void Monster::init(cWorld* world, bool isBlue) {
    m_world = world;
    m_isBlue = isBlue;
    m_mesh = new cMesh();
    cCreateSphere(m_mesh, m_radius, 24, 24);

    cMaterial mat;
    if (m_isBlue) {
        mat.m_ambient.set(0.0f, 0.3f, 0.6f, 1.0f);
        mat.m_diffuse.set(0.1f, 0.6f, 1.0f, 1.0f);
        mat.m_specular.set(0.8f, 0.9f, 1.0f, 1.0f);
        mat.m_emission.set(0.1f, 0.4f, 0.8f, 1.0f);
    } else {
        mat.setRedCrimson();
        mat.m_specular.set(0.9f, 0.9f, 0.9f, 1.0f);
    }
    mat.setShininess(80);
    m_mesh->setMaterial(mat);

    m_mesh->setShowEnabled(false);
    m_world->addChild(m_mesh);
}

void Monster::spawn(const cVector3d& spawnPos, const cVector3d& targetPos, double initialSpeed, double accel) {
    // Elevate monster center to Z = 0.55 to match gun barrel height
    m_spawnPos.set(spawnPos.x(), spawnPos.y(), 0.55);
    m_targetPos.set(targetPos.x(), targetPos.y(), 0.55);
    m_position = m_spawnPos;
    
    m_baseSpeed = initialSpeed;
    m_currentSpeed = initialSpeed;
    m_acceleration = accel;
    m_health = MAX_HEALTH; // 10 hits requirement
    
    m_moveDir = m_targetPos - m_spawnPos;
    m_distanceTotal = m_moveDir.length();
    if (m_distanceTotal > 1e-4) {
        m_moveDir.normalize();
    } else {
        m_moveDir.set(0, -1, 0);
    }
    
    m_pulseTime = 0.0;
    m_active = true;

    if (m_mesh) {
        m_mesh->setLocalPos(m_position);
        m_mesh->setShowEnabled(true);
    }
}

bool Monster::takeDamage(int damage) {
    if (!m_active) return false;

    m_health -= damage;
    if (m_health <= 0) {
        m_health = 0;
        deactivate();
        return true; // Monster died!
    }
    return false; // Still alive
}

void Monster::update(double dt) {
    if (!m_active) return;

    // Smooth time step for 3D sweeping & vertical movement
    m_pulseTime += dt * 0.27;

    // Calculate distance traveled from top spawn Y down to current Y
    double distanceTraveled = std::abs(m_spawnPos.y() - m_position.y());
    if (distanceTraveled < 0.0) distanceTraveled = 0.0;

    // Progressive gentle acceleration towards target
    m_currentSpeed = m_baseSpeed + m_acceleration * distanceTraveled;

    // Steady, smooth progression step directly towards turret destination line (-Y)
    double newY = m_position.y() - m_currentSpeed * dt;

    // Smooth sinusoidal horizontal sweeping: Red weaves Left->Right, Blue weaves Right->Left
    double wideAmplitude = 4.0;
    double newX = m_isBlue ? (-wideAmplitude * std::sin(m_pulseTime)) : (wideAmplitude * std::sin(m_pulseTime));

    // Dynamic 3D vertical up-and-down motion
    double newZ = m_isBlue ? (1.90 - 1.60 * std::cos(m_pulseTime * 1.8)) : (1.90 + 1.60 * std::cos(m_pulseTime * 1.8));

    m_position.set(newX, newY, newZ);

    if (m_mesh) {
        m_mesh->setLocalPos(m_position);

        // Health ratio: 1.0 (full health = 10) down to 0.1 (1 hit left)
        double healthRatio = (double)m_health / (double)MAX_HEALTH;
        if (healthRatio < 0.0) healthRatio = 0.0;
        double dmgRatio = 1.0 - healthRatio; // 0.0 (fresh) -> 0.9 (pale gray)

        cMaterial mat;
        if (m_isBlue) {
            // Blend from vibrant electric cyan/blue to pale light blue/gray as monster takes hits
            float redComp   = (float)(0.10 * healthRatio + 0.50 * dmgRatio);
            float greenComp = (float)(0.60 * healthRatio + 0.50 * dmgRatio);
            float blueComp  = (float)(1.00 * healthRatio + 0.55 * dmgRatio);

            mat.m_ambient.set(redComp * 0.3f, greenComp * 0.3f, blueComp * 0.3f, 1.0f);
            mat.m_diffuse.set(redComp, greenComp, blueComp, 1.0f);
            mat.m_specular.set(0.3f * (float)healthRatio + 0.3f * (float)dmgRatio,
                               0.8f * (float)healthRatio + 0.3f * (float)dmgRatio,
                               1.0f * (float)healthRatio + 0.3f * (float)dmgRatio, 1.0f);
            mat.m_emission.set(0.1f * (float)healthRatio, 0.4f * (float)healthRatio, 0.8f * (float)healthRatio, 1.0f);
        } else {
            // Blend from vibrant red/crimson to pale gray as monster takes hits
            float redComp   = (float)(0.90 * healthRatio + 0.50 * dmgRatio);
            float greenComp = (float)(0.10 * healthRatio + 0.50 * dmgRatio);
            float blueComp  = (float)(0.10 * healthRatio + 0.52 * dmgRatio);

            mat.m_ambient.set(redComp * 0.3f, greenComp * 0.3f, blueComp * 0.3f, 1.0f);
            mat.m_diffuse.set(redComp, greenComp, blueComp, 1.0f);
            mat.m_specular.set(0.8f * (float)healthRatio + 0.3f * (float)dmgRatio,
                               0.2f * (float)healthRatio + 0.3f * (float)dmgRatio,
                               0.2f * (float)healthRatio + 0.3f * (float)dmgRatio, 1.0f);
        }
        mat.setShininess((int)(20 + 70 * healthRatio));
        m_mesh->setMaterial(mat);
    }
}

bool Monster::hasReachedDestination() const {
    if (!m_active) return false;
    
    // Defeat trigger: Monster gets past the gun/turret at Y <= -7.0
    return (m_position.y() <= -7.0);
}

void Monster::deactivate() {
    m_active = false;
    if (m_mesh) {
        m_mesh->setShowEnabled(false);
    }
}
