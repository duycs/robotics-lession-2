#ifndef MONSTER_H
#define MONSTER_H

#include "chai3d.h"

class Monster {
public:
    Monster();
    ~Monster();

    void init(chai3d::cWorld* world, bool isBlue = false);
    void spawn(const chai3d::cVector3d& spawnPos, const chai3d::cVector3d& targetPos, double initialSpeed = 0.15, double accel = 0.015);
    void update(double dt);
    bool takeDamage(int damage = 1);
    void deactivate();
    void cleanup();

    bool isActive() const { return m_active; }
    bool isBlue() const { return m_isBlue; }
    bool hasReachedDestination() const;
    chai3d::cVector3d getPosition() const { return m_position; }
    double getRadius() const { return m_radius; }
    double getCurrentSpeed() const { return m_currentSpeed; }
    int getHealth() const { return m_health; }
    int getMaxHealth() const { return MAX_HEALTH; }

private:
    chai3d::cWorld* m_world;
    chai3d::cMesh* m_mesh;
    chai3d::cVector3d m_position;
    chai3d::cVector3d m_spawnPos;
    chai3d::cVector3d m_targetPos;
    chai3d::cVector3d m_moveDir;
    
    double m_baseSpeed;
    double m_currentSpeed;
    double m_acceleration;
    double m_distanceTotal;
    double m_radius;
    double m_pulseTime;
    
    int m_health;
    static constexpr int MAX_HEALTH = 10;
    bool m_active;
    bool m_isBlue;
};

#endif // MONSTER_H
