#ifndef BULLET_H
#define BULLET_H

#include "chai3d.h"

class Bullet {
public:
    Bullet();
    ~Bullet();

    void init(chai3d::cWorld* world);
    void spawn(const chai3d::cVector3d& startPos, const chai3d::cVector3d& direction, double speed = 30.0);
    void update(double dt);
    void deactivate();
    void cleanup();

    bool isActive() const { return m_active; }
    bool isExpired() const { return m_age >= 10.0 || !m_active; }
    chai3d::cVector3d getPosition() const { return m_position; }
    chai3d::cVector3d getPrevPosition() const { return m_prevPosition; }
    double getRadius() const { return m_radius; }

private:
    chai3d::cWorld* m_world;
    chai3d::cMesh* m_mesh;
    chai3d::cVector3d m_position;
    chai3d::cVector3d m_prevPosition;
    chai3d::cVector3d m_velocity;
    double m_age;
    double m_radius;
    bool m_active;
};

#endif // BULLET_H
