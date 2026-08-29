#ifndef TURRET_H
#define TURRET_H

#include "chai3d.h"
#include <string>

class Turret {
public:
    Turret();
    ~Turret();

    void init(chai3d::cWorld* world, const std::string& assetsDir);
    void setPosition(const chai3d::cVector3d& pos);
    chai3d::cVector3d getPosition() const { return m_position; }

    void updateYaw(double deltaYaw);
    void updatePitch(double deltaPitch);

    void setAngles(double yawRad, double pitchRad);
    void aimAt(const chai3d::cVector3d& targetPos);
    void cleanup();

    double getYawDeg() const;
    double getPitchDeg() const;

    chai3d::cVector3d getBarrelTipPosition() const;
    chai3d::cVector3d getFiringDirection() const;

    void updateLaserEffect(double timeSec);

    chai3d::cMultiMesh* getBaseMesh() const { return m_baseMesh; }
    chai3d::cMultiMesh* getGunMesh() const { return m_gunMesh; }

private:
    void generateBaseModel(const std::string& filepath);
    void generateGunModel(const std::string& filepath);
    void applyKinematics();

    chai3d::cWorld* m_world;
    chai3d::cMultiMesh* m_baseMesh;
    chai3d::cMultiMesh* m_gunMesh;
    chai3d::cMesh* m_laserMesh;
    chai3d::cMesh* m_laserDotMesh;

    chai3d::cVector3d m_position;
    
    // Kinematic joint angles in radians
    double m_yawAngle;   // Yaw limit: 120 deg total (-60 to +60 deg)
    double m_pitchAngle; // Pitch limit: 60 deg total (-30 to +30 deg)

    // Constants (60 deg yaw = 1.0472 rad, 30 deg pitch = 0.5236 rad)
    static constexpr double YAW_MIN = -1.0471975511965976;
    static constexpr double YAW_MAX =  1.0471975511965976;
    static constexpr double PITCH_MIN = -0.5235987755982988;
    static constexpr double PITCH_MAX =  0.5235987755982988;
};

#endif // TURRET_H
