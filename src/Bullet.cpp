#include "Bullet.h"

using namespace chai3d;

Bullet::Bullet() :
    m_world(nullptr),
    m_mesh(nullptr),
    m_position(0, -999, -999),
    m_prevPosition(0, -999, -999),
    m_velocity(0, 0, 0),
    m_age(0.0),
    m_radius(0.18),
    m_active(false)
{
}

Bullet::~Bullet() {
    cleanup();
}

void Bullet::cleanup() {
    if (m_mesh) {
        if (m_world) {
            m_world->removeChild(m_mesh);
        }
        delete m_mesh;
        m_mesh = nullptr;
    }
}

void Bullet::init(cWorld* world) {
    m_world = world;
    m_mesh = new cMesh();
    cCreateSphere(m_mesh, m_radius, 16, 16);

    // Glowing yellow bullet material
    cMaterial mat;
    mat.setYellowGold();
    mat.setShininess(100);
    mat.m_emission.set(1.0f, 0.8f, 0.2f, 1.0f);
    m_mesh->setMaterial(mat);

    m_position.set(0, -999, -999);
    m_prevPosition = m_position;
    m_mesh->setLocalPos(m_position);
    m_mesh->setEnabled(false);
    m_mesh->setShowEnabled(false);
    m_world->addChild(m_mesh);
}

void Bullet::spawn(const cVector3d& startPos, const cVector3d& direction, double speed) {
    m_position = startPos;
    m_prevPosition = startPos;
    cVector3d normDir = direction;
    normDir.normalize();
    m_velocity = normDir * speed;
    m_age = 0.0;
    m_active = true;

    if (m_mesh) {
        m_mesh->setLocalPos(m_position);
        m_mesh->setEnabled(true);
        m_mesh->setShowEnabled(true);
    }
}

void Bullet::update(double dt) {
    if (!m_active) return;

    m_age += dt;
    if (m_age >= 10.0) { // Requirement: 10s lifetime
        deactivate();
        return;
    }

    m_prevPosition = m_position;
    m_position += m_velocity * dt;
    if (m_mesh) {
        m_mesh->setLocalPos(m_position);
    }
}

void Bullet::deactivate() {
    m_active = false;
    m_position.set(0, -999, -999);
    m_prevPosition = m_position;
    if (m_mesh) {
        m_mesh->setLocalPos(m_position);
        m_mesh->setEnabled(false);
        m_mesh->setShowEnabled(false);
    }
}
