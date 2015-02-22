#include <SFML/OpenGL.hpp>
#include "nuke.h"
#include "particleEffect.h"
#include "explosionEffect.h"

REGISTER_MULTIPLAYER_CLASS(Nuke, "Nuke");
Nuke::Nuke()
: MissileWeapon("Nuke", 400.0f, 50.0f, 12.0f, sf::Color(255, 100, 32))
{
}

void Nuke::hitObject(P<SpaceObject> object)
{
    DamageInfo info(DT_Kinetic, getPosition());
    SpaceObject::damageArea(getPosition(), blastRange, damageAtEdge, damageAtCenter, info, getRadius());

    P<ExplosionEffect> e = new ExplosionEffect();
    e->setSize(blastRange);
    e->setPosition(getPosition());
    destroy();
}