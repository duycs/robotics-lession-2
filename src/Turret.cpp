#include "Turret.h"
#include "files/CFileModelOBJ.h"
#include <fstream>
#include <iostream>
#include <cmath>

using namespace chai3d;
using namespace std;

Turret::Turret() :
    m_world(nullptr),
    m_baseMesh(nullptr),
    m_gunMesh(nullptr),
    m_position(0, 0, 0),
    m_yawAngle(0.0),
    m_pitchAngle(0.0)
{
}

Turret::~Turret() {
    cleanup();
}

void Turret::cleanup() {
    if (m_baseMesh) {
        if (m_world) {
            m_world->removeChild(m_baseMesh);
        }
        delete m_baseMesh; // deletes m_baseMesh and its child m_gunMesh
        m_baseMesh = nullptr;
        m_gunMesh = nullptr;
    }
}

void Turret::init(cWorld* world, const string& assetsDir) {
    m_world = world;
    m_baseMesh = new cMultiMesh();
    m_gunMesh = new cMultiMesh();

    string baseFilepath = assetsDir + "/base.obj";
    string gunFilepath = assetsDir + "/gun.obj";

    // Generate 3D procedural meshes directly in CHAI3D memory and export base.obj / gun.obj
    generateBaseModel(baseFilepath);
    generateGunModel(gunFilepath);

    // Material setup
    cMaterial baseMat;
    baseMat.m_ambient.set(0.15f, 0.2f, 0.25f, 1.0f);
    baseMat.m_diffuse.set(0.3f, 0.35f, 0.4f, 1.0f);
    baseMat.m_specular.set(0.8f, 0.8f, 0.8f, 1.0f);
    baseMat.setShininess(90);
    m_baseMesh->setMaterial(baseMat);

    cMaterial gunMat;
    gunMat.m_ambient.set(0.3f, 0.35f, 0.4f, 1.0f);
    gunMat.m_diffuse.set(0.7f, 0.75f, 0.8f, 1.0f);
    gunMat.m_specular.set(1.0f, 1.0f, 1.0f, 1.0f);
    gunMat.setShininess(120);
    m_gunMesh->setMaterial(gunMat);

    // Parent-Child hierarchy setup: base is parent, gun is child
    m_baseMesh->addChild(m_gunMesh);

    // Gun local offset relative to base (placed on top of turret base ring)
    m_gunMesh->setLocalPos(0.0, 0.0, 0.6);

    m_world->addChild(m_baseMesh);

    setPosition(cVector3d(0, 0, 0));
    applyKinematics();
}

void Turret::setPosition(const cVector3d& pos) {
    m_position = pos;
    if (m_baseMesh) {
        m_baseMesh->setLocalPos(m_position);
    }
}

void Turret::updateYaw(double deltaYaw) {
    m_yawAngle += deltaYaw;
    // Limit base Yaw to 120 degrees (-60 to +60 degrees)
    if (m_yawAngle < YAW_MIN) m_yawAngle = YAW_MIN;
    if (m_yawAngle > YAW_MAX) m_yawAngle = YAW_MAX;
    applyKinematics();
}

void Turret::updatePitch(double deltaPitch) {
    m_pitchAngle += deltaPitch;
    // Limit gun Pitch to 60 degrees (-30 to +30 degrees)
    if (m_pitchAngle < PITCH_MIN) m_pitchAngle = PITCH_MIN;
    if (m_pitchAngle > PITCH_MAX) m_pitchAngle = PITCH_MAX;
    applyKinematics();
}

void Turret::setAngles(double yawRad, double pitchRad) {
    m_yawAngle = yawRad;
    if (m_yawAngle < YAW_MIN) m_yawAngle = YAW_MIN;
    if (m_yawAngle > YAW_MAX) m_yawAngle = YAW_MAX;

    m_pitchAngle = pitchRad;
    if (m_pitchAngle < PITCH_MIN) m_pitchAngle = PITCH_MIN;
    if (m_pitchAngle > PITCH_MAX) m_pitchAngle = PITCH_MAX;

    applyKinematics();
}

void Turret::aimAt(const cVector3d& targetPos) {
    cVector3d barrelBasePos = m_position + cVector3d(0, 0, 0.6);
    cVector3d delta = targetPos - barrelBasePos;

    // Yaw angle around vertical Z axis:
    double yaw = std::atan2(-delta.x(), delta.y());

    // Pitch elevation angle relative to horizontal plane
    double distXY = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
    double pitch = std::atan2(delta.z(), distXY);

    setAngles(yaw, pitch);
}

void Turret::applyKinematics() {
    if (!m_baseMesh || !m_gunMesh) return;

    // Base rotation around vertical Z axis (Yaw)
    cMatrix3d R_base;
    R_base.setExtrinsicEulerRotationDeg(0, 0, cRadToDeg(m_yawAngle), C_EULER_ORDER_XYZ);
    m_baseMesh->setLocalRot(R_base);

    // Gun rotation around local horizontal X axis (Pitch) inside parent base frame
    cMatrix3d R_gun;
    R_gun.setExtrinsicEulerRotationDeg(cRadToDeg(m_pitchAngle), 0, 0, C_EULER_ORDER_XYZ);
    m_gunMesh->setLocalRot(R_gun);
}

double Turret::getYawDeg() const {
    return cRadToDeg(m_yawAngle);
}

double Turret::getPitchDeg() const {
    return cRadToDeg(m_pitchAngle);
}

cVector3d Turret::getBarrelTipPosition() const {
    if (!m_gunMesh) return m_position;

    // Ensure global transform of child mesh is updated from root world
    m_gunMesh->computeGlobalPositionsFromRoot();

    // Local barrel tip offset in gun coordinate frame (forward along +Y axis, height +Z)
    cVector3d localTip(0.0, 1.4, 0.15);
    return m_gunMesh->getGlobalTransform() * localTip;
}

cVector3d Turret::getFiringDirection() const {
    if (!m_gunMesh) return cVector3d(0, 1, 0);

    // Ensure global rotation of child mesh is updated from root world
    m_gunMesh->computeGlobalPositionsFromRoot();

    // Gun forward direction (+Y axis in gun local frame) transformed to world frame
    cMatrix3d R_global = m_gunMesh->getGlobalRot();
    return R_global * cVector3d(0, 1, 0);
}

void Turret::generateBaseModel(const string& filepath) {
    if (!m_baseMesh) return;

    // Build compound 3D base mesh using CHAI3D primitives
    cMesh* m1 = m_baseMesh->newMesh();
    cCreateCylinder(m1, 0.4, 0.9, 36, 1, 1, true, true, cVector3d(0, 0, 0));

    cMesh* m2 = m_baseMesh->newMesh();
    cCreateCylinder(m2, 0.15, 1.1, 36, 1, 1, true, true, cVector3d(0, 0, 0.4));

    cMesh* m3 = m_baseMesh->newMesh();
    cCreateBox(m3, 1.0, 1.0, 0.5, cVector3d(0, 0, 0.55));

    // Save generated 3D OBJ file
    cSaveFileOBJ(m_baseMesh, filepath);
}

void Turret::generateGunModel(const string& filepath) {
    if (!m_gunMesh) return;

    // Build compound 3D gun mesh using CHAI3D primitives
    cMesh* m1 = m_gunMesh->newMesh();
    cCreateBox(m1, 0.6, 0.8, 0.5, cVector3d(0, 0.2, 0));

    cMatrix3d rotB;
    rotB.setExtrinsicEulerRotationDeg(-90, 0, 0, C_EULER_ORDER_XYZ);

    cMesh* m2 = m_gunMesh->newMesh();
    cCreateCylinder(m2, 1.4, 0.1, 24, 1, 1, true, true, cVector3d(-0.18, 0.0, 0.15), rotB);

    cMesh* m3 = m_gunMesh->newMesh();
    cCreateCylinder(m3, 1.4, 0.1, 24, 1, 1, true, true, cVector3d(0.18, 0.0, 0.15), rotB);

    cMesh* m4 = m_gunMesh->newMesh();
    cCreateCylinder(m4, 0.15, 0.14, 24, 1, 1, true, true, cVector3d(-0.18, 1.35, 0.15), rotB);

    cMesh* m5 = m_gunMesh->newMesh();
    cCreateCylinder(m5, 0.15, 0.14, 24, 1, 1, true, true, cVector3d(0.18, 1.35, 0.15), rotB);

    // Glowing Neon Red 3D Laser Sight Beam (35m long razor-thin 4mm laser beam attached to gun muzzle tip)
    m_laserMesh = m_gunMesh->newMesh();
    cCreateCylinder(m_laserMesh, 35.0, 0.004, 16, 1, 1, true, true, cVector3d(0.0, 1.4, 0.15), rotB);

    cMaterial laserMat;
    laserMat.m_ambient.set(1.0f, 0.0f, 0.0f, 1.0f);
    laserMat.m_diffuse.set(1.0f, 0.1f, 0.1f, 1.0f);
    laserMat.m_specular.set(1.0f, 0.6f, 0.6f, 1.0f);
    laserMat.m_emission.set(1.0f, 0.2f, 0.2f, 1.0f);
    m_laserMesh->setMaterial(laserMat);

    // Glowing Laser Target Point Dot Sphere (End dot at 35m)
    m_laserDotMesh = m_gunMesh->newMesh();
    cCreateSphere(m_laserDotMesh, 0.04, 16, 16, cVector3d(0.0, 36.4, 0.15));
    m_laserDotMesh->setMaterial(laserMat);

    // Save generated 3D OBJ file
    cSaveFileOBJ(m_gunMesh, filepath);
}

void Turret::updateLaserEffect(double timeSec) {
    if (!m_laserMesh) return;

    // High-frequency tactical laser strobe & flicker intensity
    double flicker = 0.60 + 0.40 * std::sin(timeSec * 36.0);
    int tick = (int)(timeSec * 22.0);
    if (tick % 7 == 0) {
        flicker *= 0.25; // micro dropout flicker
    }

    cMaterial mat;
    mat.m_ambient.set(1.0f * flicker, 0.0f, 0.0f, 1.0f);
    mat.m_diffuse.set(1.0f * flicker, 0.15f * flicker, 0.15f * flicker, 1.0f);
    mat.m_specular.set(1.0f * flicker, 0.6f * flicker, 0.6f * flicker, 1.0f);
    mat.m_emission.set(1.0f * flicker, 0.35f * flicker, 0.35f * flicker, 1.0f);

    m_laserMesh->setMaterial(mat);
    if (m_laserDotMesh) {
        m_laserDotMesh->setMaterial(mat);
    }
}
