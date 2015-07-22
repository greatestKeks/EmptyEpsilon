#include "spaceship.h"
#include "mesh.h"
#include "shipTemplate.h"
#include "playerInfo.h"
#include "spaceObjects/beamEffect.h"
#include "factionInfo.h"
#include "spaceObjects/explosionEffect.h"
#include "spaceObjects/EMPMissile.h"
#include "spaceObjects/homingMissile.h"
#include "particleEffect.h"
#include "spaceObjects/mine.h"
#include "spaceObjects/nuke.h"
#include "spaceObjects/warpJammer.h"
#include "gameGlobalInfo.h"

#include "scriptInterface.h"
REGISTER_SCRIPT_SUBCLASS_NO_CREATE(SpaceShip, SpaceObject)
{
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setShipTemplate);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setScanned);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, isDocked);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, getWeaponStorage);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, getWeaponStorageMax);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setWeaponStorage);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, getHull);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, getHullMax);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setHull);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setHullMax);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, getFrontShield);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, getFrontShieldMax);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setFrontShield);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setFrontShieldMax);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, getRearShield);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, getRearShieldMax);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setRearShield);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setRearShieldMax);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, getShieldsActive);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setShieldsActive);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, getSystemHealth);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setSystemHealth);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, hasJumpDrive);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setJumpDrive);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, hasWarpDrive);
    REGISTER_SCRIPT_CLASS_FUNCTION(SpaceShip, setWarpDrive);
}

/* Define script conversion function for the EMainScreenSetting enum. */
template<> void convert<EMainScreenSetting>::param(lua_State* L, int& idx, EMainScreenSetting& mss)
{
    string str = string(luaL_checkstring(L, idx++)).lower();
    if (str == "front")
        mss = MSS_Front;
    else if (str == "back")
        mss = MSS_Back;
    else if (str == "left")
        mss = MSS_Left;
    else if (str == "right")
        mss = MSS_Right;
    else if (str == "tactical")
        mss = MSS_Tactical;
    else if (str == "longrange")
        mss = MSS_LongRange;
    else
        mss = MSS_Front;
}

SpaceShip::SpaceShip(string multiplayerClassName, float multiplayer_significant_range)
: SpaceObject(50, multiplayerClassName, multiplayer_significant_range)
{
    setCollisionPhysics(true, false);

    target_rotation = getRotation();
    impulse_request = 0;
    current_impulse = 0;
    has_warp_drive = true;
    warp_request = 0.0;
    current_warp = 0.0;
    has_jump_drive = true;
    jump_distance = 0.0;
    jump_delay = 0.0;
    tube_load_time = 8.0;
    weapon_tubes = 0;
    turn_speed = 10.0;
    impulse_max_speed = 600.0;
    warp_speed_per_warp_level = 1000.0;
    combat_maneuver_charge = 1.0;
    combat_maneuver_boost_request = 0.0;
    combat_maneuver_boost_active = 0.0;
    combat_maneuver_strafe_request = 0.0;
    combat_maneuver_strafe_active = 0.0;
    target_id = -1;
    hull_strength = hull_max = 70;
    shields_active = false;
    front_shield = rear_shield = front_shield_max = rear_shield_max = 50;
    front_shield_hit_effect = rear_shield_hit_effect = 0;
    scanned_by_player = SS_NotScanned;
    beam_frequency = irandom(0, max_frequency);
    beam_system_target = SYS_None;
    shield_frequency = irandom(0, max_frequency);
    docking_state = DS_NotDocking;
    impulse_acceleration = 20.0;
    energy_level = 1000;

    registerMemberReplication(&ship_callsign);
    registerMemberReplication(&target_rotation, 1.5);
    registerMemberReplication(&impulse_request, 0.1);
    registerMemberReplication(&current_impulse, 0.5);
    registerMemberReplication(&has_warp_drive);
    registerMemberReplication(&warp_request, 0.1);
    registerMemberReplication(&current_warp, 0.1);
    registerMemberReplication(&has_jump_drive);
    registerMemberReplication(&jump_delay, 0.5);
    registerMemberReplication(&tube_load_time);
    registerMemberReplication(&weapon_tubes);
    registerMemberReplication(&target_id);
    registerMemberReplication(&turn_speed);
    registerMemberReplication(&impulse_max_speed);
    registerMemberReplication(&impulse_acceleration);
    registerMemberReplication(&warp_speed_per_warp_level);
    registerMemberReplication(&template_name);
    registerMemberReplication(&ship_type_name);
    registerMemberReplication(&front_shield, 1.0);
    registerMemberReplication(&rear_shield, 1.0);
    registerMemberReplication(&front_shield_max);
    registerMemberReplication(&rear_shield_max);
    registerMemberReplication(&shield_frequency);
    registerMemberReplication(&front_shield_hit_effect, 0.5);
    registerMemberReplication(&rear_shield_hit_effect, 0.5);
    registerMemberReplication(&scanned_by_player);
    registerMemberReplication(&docking_state);
    registerMemberReplication(&beam_frequency);
    registerMemberReplication(&combat_maneuver_charge, 0.5);
    registerMemberReplication(&combat_maneuver_boost_request);
    registerMemberReplication(&combat_maneuver_boost_active, 0.2);
    registerMemberReplication(&combat_maneuver_strafe_request);
    registerMemberReplication(&combat_maneuver_strafe_active, 0.2);

    for(int n=0; n<SYS_COUNT; n++)
    {
        systems[n].health = 1.0;
        systems[n].power_level = 1.0;
        systems[n].coolant_level = 0.0;
        systems[n].heat_level = 0.0;

        registerMemberReplication(&systems[n].health, 0.1);
    }

    for(int n=0; n<max_beam_weapons; n++)
    {
        beam_weapons[n].arc = 0;
        beam_weapons[n].direction = 0;
        beam_weapons[n].range = 0;
        beam_weapons[n].cycleTime = 6.0;
        beam_weapons[n].cooldown = 0.0;
        beam_weapons[n].damage = 1.0;

        registerMemberReplication(&beam_weapons[n].arc);
        registerMemberReplication(&beam_weapons[n].direction);
        registerMemberReplication(&beam_weapons[n].range);
        registerMemberReplication(&beam_weapons[n].cycleTime);
        registerMemberReplication(&beam_weapons[n].cooldown, 0.5);
    }
    for(int n=0; n<max_weapon_tubes; n++)
    {
        weaponTube[n].type_loaded = MW_None;
        weaponTube[n].state = WTS_Empty;
        weaponTube[n].delay = 0.0;

        registerMemberReplication(&weaponTube[n].type_loaded);
        registerMemberReplication(&weaponTube[n].state);
        registerMemberReplication(&weaponTube[n].delay, 0.5);
    }
    for(int n=0; n<MW_Count; n++)
    {
        weapon_storage[n] = 0;
        weapon_storage_max[n] = 0;
        registerMemberReplication(&weapon_storage[n]);
        registerMemberReplication(&weapon_storage_max[n]);
    }

    if (game_server)
        ship_callsign = gameGlobalInfo->getNextShipCallsign();
}

void SpaceShip::setShipTemplate(string template_name)
{
    P<ShipTemplate> new_ship_template = ShipTemplate::getTemplate(template_name);
    if (!new_ship_template)
    {
        LOG(ERROR) << "Failed to find ship template: " << template_name;
        return;
    }

    this->ship_template = new_ship_template;
    this->template_name = template_name;
    this->ship_type_name = template_name;

    for(int n=0; n<max_beam_weapons; n++)
    {
        beam_weapons[n].arc = ship_template->beams[n].arc;
        beam_weapons[n].direction = ship_template->beams[n].direction;
        beam_weapons[n].range = ship_template->beams[n].range;
        beam_weapons[n].cycleTime = ship_template->beams[n].cycle_time;
        beam_weapons[n].damage = ship_template->beams[n].damage;
    }
    weapon_tubes = ship_template->weapon_tubes;
    hull_strength = hull_max = ship_template->hull;
    front_shield = ship_template->front_shields;
    rear_shield = ship_template->rear_shields;
    front_shield_max = ship_template->front_shields;
    rear_shield_max = ship_template->rear_shields;
    impulse_max_speed = ship_template->impulse_speed;
    impulse_acceleration = ship_template->impulse_acceleration;
    turn_speed = ship_template->turn_speed;
    has_warp_drive = ship_template->warp_speed > 0.0;
    warp_speed_per_warp_level = ship_template->warp_speed;
    has_jump_drive = ship_template->has_jump_drive;
    tube_load_time = ship_template->tube_load_time;
    //shipTemplate->has_cloaking;
    for(int n=0; n<MW_Count; n++)
        weapon_storage[n] = weapon_storage_max[n] = ship_template->weapon_storage[n];

    ship_template->setCollisionData(this);
    model_info.setData(ship_template->model_data);
}

void SpaceShip::draw3DTransparent()
{
    if (!ship_template) return;

    if (front_shield_hit_effect > 0)
        model_info.renderFrontShield((front_shield / front_shield_max) * front_shield_hit_effect);
    if (rear_shield_hit_effect > 0)
        model_info.renderRearShield((rear_shield / rear_shield_max) * rear_shield_hit_effect);

    if (has_jump_drive && jump_delay > 0.0f)
    {
        float alpha = 1.0f - (jump_delay / 10.0f);
        model_info.renderOverlay(textureManager.getTexture("electric_sphere_texture.png"), alpha);
    }
}

void SpaceShip::drawOnRadar(sf::RenderTarget& window, sf::Vector2f position, float scale, bool long_range)
{
    if (!long_range && ((scanned_by_player == SS_FullScan) || !my_spaceship))
    {
        for(int n=0; n<max_beam_weapons; n++)
        {
            if (beam_weapons[n].range == 0.0) continue;
            sf::Color color = sf::Color::Red;
            if (beam_weapons[n].cooldown > 0)
                color = sf::Color(255, 255 * (beam_weapons[n].cooldown / beam_weapons[n].cycleTime), 0);

            sf::Vector2f beam_offset = sf::rotateVector(ship_template->model_data->getBeamPosition2D(n) * scale, getRotation());
            sf::VertexArray a(sf::LinesStrip, 3);
            a[0].color = color;
            a[1].color = color;
            a[2].color = sf::Color(color.r, color.g, color.b, 0);
            a[0].position = beam_offset + position;
            a[1].position = beam_offset + position + sf::vector2FromAngle(getRotation() + (beam_weapons[n].direction + beam_weapons[n].arc / 2.0f)) * beam_weapons[n].range * scale;
            a[2].position = beam_offset + position + sf::vector2FromAngle(getRotation() + (beam_weapons[n].direction + beam_weapons[n].arc / 2.0f)) * beam_weapons[n].range * scale * 1.3f;
            window.draw(a);
            a[1].position = beam_offset + position + sf::vector2FromAngle(getRotation() + (beam_weapons[n].direction - beam_weapons[n].arc / 2.0f)) * beam_weapons[n].range * scale;
            a[2].position = beam_offset + position + sf::vector2FromAngle(getRotation() + (beam_weapons[n].direction - beam_weapons[n].arc / 2.0f)) * beam_weapons[n].range * scale * 1.3f;
            window.draw(a);

            int arcPoints = int(beam_weapons[n].arc / 10) + 1;
            sf::VertexArray arc(sf::LinesStrip, arcPoints);
            for(int i=0; i<arcPoints; i++)
            {
                arc[i].color = color;
                arc[i].position = beam_offset + position + sf::vector2FromAngle(getRotation() + (beam_weapons[n].direction - beam_weapons[n].arc / 2.0f + 10 * i)) * beam_weapons[n].range * scale;
            }
            arc[arcPoints-1].position = beam_offset + position + sf::vector2FromAngle(getRotation() + (beam_weapons[n].direction + beam_weapons[n].arc / 2.0f)) * beam_weapons[n].range * scale;
            window.draw(arc);
        }
    }

    sf::Sprite objectSprite;
    textureManager.setTexture(objectSprite, "RadarArrow.png");
    objectSprite.setRotation(getRotation());
    objectSprite.setPosition(position);
    if (long_range)
    {
        objectSprite.setScale(0.7, 0.7);
    }
    if (my_spaceship == this)
    {
        objectSprite.setColor(sf::Color(192, 192, 255));
    }else if (my_spaceship)
    {
        if (scanned_by_player != SS_NotScanned)
        {
            if (isEnemy(my_spaceship))
                objectSprite.setColor(sf::Color::Red);
            else if (isFriendly(my_spaceship))
                objectSprite.setColor(sf::Color(128, 255, 128));
            else
                objectSprite.setColor(sf::Color(128, 128, 255));
        }else{
            objectSprite.setColor(sf::Color(192, 192, 192));
        }
    }else{
        objectSprite.setColor(factionInfo[getFactionId()]->gm_color);
    }
    window.draw(objectSprite);
}

void SpaceShip::drawOnGMRadar(sf::RenderTarget& window, sf::Vector2f position, float scale, bool long_range)
{
    if (!long_range)
    {
        sf::RectangleShape bar(sf::Vector2f(60, 10));
        
        bar.setPosition(position.x - 30, position.y - 40);
        bar.setSize(sf::Vector2f(60 * front_shield / front_shield_max, 5));
        bar.setFillColor(sf::Color(128, 128, 255, 128));
        window.draw(bar);
        bar.setPosition(position.x - 30, position.y - 35);
        bar.setSize(sf::Vector2f(60 * rear_shield / rear_shield_max, 5));
        bar.setFillColor(sf::Color(128, 128, 255, 128));
        window.draw(bar);
        bar.setPosition(position.x - 30, position.y - 30);
        bar.setSize(sf::Vector2f(60 * hull_strength / hull_max, 5));
        bar.setFillColor(sf::Color(128, 255, 128, 128));
        window.draw(bar);
    }
}

void SpaceShip::update(float delta)
{
    if (!ship_template || ship_template->getName() != template_name)
    {
        ship_template = ShipTemplate::getTemplate(template_name);
        if (!ship_template)
            return;
        ship_template->setCollisionData(this);
        model_info.setData(ship_template->model_data);
    }

    if (game_server)
    {
        if (docking_state == DS_Docking)
        {
            if (!docking_target)
                docking_state = DS_NotDocking;
            else
                target_rotation = sf::vector2ToAngle(getPosition() - docking_target->getPosition());
            if (fabs(sf::angleDifference(target_rotation, getRotation())) < 10.0)
                impulse_request = -1.0;
            else
                impulse_request = 0.0;
        }
        if (docking_state == DS_Docked)
        {
            if (!docking_target)
            {
                docking_state = DS_NotDocking;
            }else{
                setPosition(docking_target->getPosition() + sf::rotateVector(docking_offset, docking_target->getRotation()));
                target_rotation = sf::vector2ToAngle(getPosition() - docking_target->getPosition());
            }
            impulse_request = 0.0;
        }
    }

    if (front_shield < front_shield_max)
    {
        front_shield += delta * shield_recharge_rate * getSystemEffectiveness(SYS_FrontShield);
        if (docking_state == DS_Docked)
            front_shield += delta * shield_recharge_rate * getSystemEffectiveness(SYS_FrontShield) * 5.0;
        if (front_shield > front_shield_max)
            front_shield = front_shield_max;
    }
    if (rear_shield < front_shield_max)
    {
        rear_shield += delta * shield_recharge_rate * getSystemEffectiveness(SYS_RearShield);
        if (docking_state == DS_Docked)
            rear_shield += delta * shield_recharge_rate * getSystemEffectiveness(SYS_RearShield) * 5.0;
        if (rear_shield > rear_shield_max)
            rear_shield = rear_shield_max;
    }
    if (front_shield_hit_effect > 0)
        front_shield_hit_effect -= delta;
    if (rear_shield_hit_effect > 0)
        rear_shield_hit_effect -= delta;

    float rotationDiff = sf::angleDifference(getRotation(), target_rotation);

    if (rotationDiff > 1.0)
        setAngularVelocity(turn_speed * getSystemEffectiveness(SYS_Maneuver));
    else if (rotationDiff < -1.0)
        setAngularVelocity(-turn_speed * getSystemEffectiveness(SYS_Maneuver));
    else
        setAngularVelocity(rotationDiff * turn_speed * getSystemEffectiveness(SYS_Maneuver));

    if ((has_jump_drive && jump_delay > 0) || (has_warp_drive && warp_request > 0))
    {
        if (WarpJammer::isWarpJammed(getPosition()))
        {
            jump_delay = 0;
            warp_request = 0.0f;
        }
    }
    if (has_jump_drive && jump_delay > 0)
    {
        if (current_impulse > 0.0)
        {
            if (impulse_max_speed > 0)
                current_impulse -= delta * (impulse_acceleration / impulse_max_speed);
            if (current_impulse < 0.0)
                current_impulse = 0.0;
        }
        if (current_warp > 0.0)
        {
            current_warp -= delta;
            if (current_warp < 0.0)
                current_warp = 0.0;
        }
        jump_delay -= delta * getSystemEffectiveness(SYS_JumpDrive);
        if (jump_delay <= 0.0)
        {
            executeJump(jump_distance);
            jump_delay = 0.0;
        }
    }else if (has_warp_drive && (warp_request > 0 || current_warp > 0))
    {
        if (current_impulse < 1.0)
        {
            if (impulse_max_speed > 0)
                current_impulse += delta * (impulse_acceleration / impulse_max_speed);
            if (current_impulse > 1.0)
                current_impulse = 1.0;
        }else{
            if (current_warp < warp_request)
            {
                current_warp += delta;
                if (current_warp > warp_request)
                    current_warp = warp_request;
            }else if (current_warp > warp_request)
            {
                current_warp -= delta;
                if (current_warp < warp_request)
                    current_warp = warp_request;
            }
        }
    }else{
        current_warp = 0.0;
        if (impulse_request > 1.0)
            impulse_request = 1.0;
        if (impulse_request < -1.0)
            impulse_request = -1.0;
        if (current_impulse < impulse_request)
        {
            if (impulse_max_speed > 0)
                current_impulse += delta * (impulse_acceleration / impulse_max_speed);
            if (current_impulse > impulse_request)
                current_impulse = impulse_request;
        }else if (current_impulse > impulse_request)
        {
            if (impulse_max_speed > 0)
                current_impulse -= delta * (impulse_acceleration / impulse_max_speed);
            if (current_impulse < impulse_request)
                current_impulse = impulse_request;
        }
    }
    sf::Vector2f forward = sf::vector2FromAngle(getRotation());
    setVelocity(forward * (current_impulse * impulse_max_speed * getSystemEffectiveness(SYS_Impulse) + current_warp * warp_speed_per_warp_level * getSystemEffectiveness(SYS_Warp)));
    
    if (combat_maneuver_boost_active > combat_maneuver_boost_request)
    {
        combat_maneuver_boost_active -= delta;
        if (combat_maneuver_boost_active < combat_maneuver_boost_request)
            combat_maneuver_boost_active = combat_maneuver_boost_request;
    }
    if (combat_maneuver_boost_active < combat_maneuver_boost_request)
    {
        combat_maneuver_boost_active += delta;
        if (combat_maneuver_boost_active > combat_maneuver_boost_request)
            combat_maneuver_boost_active = combat_maneuver_boost_request;
    }
    if (combat_maneuver_strafe_active > combat_maneuver_strafe_request)
    {
        combat_maneuver_strafe_active -= delta;
        if (combat_maneuver_strafe_active < combat_maneuver_strafe_request)
            combat_maneuver_strafe_active = combat_maneuver_strafe_request;
    }
    if (combat_maneuver_strafe_active < combat_maneuver_strafe_request)
    {
        combat_maneuver_strafe_active += delta;
        if (combat_maneuver_strafe_active > combat_maneuver_strafe_request)
            combat_maneuver_strafe_active = combat_maneuver_strafe_request;
    }
    
    if (combat_maneuver_boost_active != 0.0)
    {
        combat_maneuver_charge -= combat_maneuver_boost_active * delta * 0.3;
        if (combat_maneuver_charge <= 0.0)
        {
            combat_maneuver_charge = 0.0;
            combat_maneuver_boost_request = 0.0;
        }else{
            setVelocity(getVelocity() + forward * impulse_max_speed * 5.0f * combat_maneuver_boost_active);
        }
    }else if (combat_maneuver_strafe_active != 0.0)
    {
        combat_maneuver_charge -= fabs(combat_maneuver_strafe_active) * delta * 0.3;
        if (combat_maneuver_charge <= 0.0)
        {
            combat_maneuver_charge = 0.0;
            combat_maneuver_strafe_request = 0.0;
        }else{
            setVelocity(getVelocity() + sf::vector2FromAngle(getRotation() + 90) * impulse_max_speed * 3.0f * combat_maneuver_strafe_active);
        }
    }else if (combat_maneuver_charge < 1.0)
    {
        combat_maneuver_charge += (delta / combat_maneuver_charge_time) * getSystemEffectiveness(SYS_Maneuver);
        if (combat_maneuver_charge > 1.0)
            combat_maneuver_charge = 1.0;
    }

    for(int n=0; n<max_beam_weapons; n++)
    {
        if (beam_weapons[n].cooldown > 0.0)
            beam_weapons[n].cooldown -= delta * getSystemEffectiveness(SYS_BeamWeapons);
    }

    P<SpaceObject> target = getTarget();
    if (game_server && target && delta > 0 && current_warp == 0.0 && docking_state == DS_NotDocking) // Only fire beam weapons if we are on the server, have a target, and are not paused.
    {
        for(int n=0; n<max_beam_weapons; n++)
        {
            if (beam_weapons[n].range <= 0.0)
                continue;
            if (target && isEnemy(target) && beam_weapons[n].cooldown <= 0.0)
            {
                sf::Vector2f diff = target->getPosition() - (getPosition() + sf::rotateVector(ship_template->model_data->getBeamPosition2D(n), getRotation()));
                float distance = sf::length(diff) - target->getRadius() / 2.0;
                float angle = sf::vector2ToAngle(diff);

                if (distance < beam_weapons[n].range)
                {
                    float angleDiff = sf::angleDifference(beam_weapons[n].direction + getRotation(), angle);
                    if (abs(angleDiff) < beam_weapons[n].arc / 2.0)
                    {
                        fireBeamWeapon(n, target);
                    }
                }
            }
        }
    }

    for(int n=0; n<max_weapon_tubes; n++)
    {
        if (weaponTube[n].delay > 0.0)
        {
            weaponTube[n].delay -= delta * getSystemEffectiveness(SYS_MissileSystem);
        }else{
            switch(weaponTube[n].state)
            {
            case WTS_Loading:
                weaponTube[n].state = WTS_Loaded;
                break;
            case WTS_Unloading:
                weaponTube[n].state = WTS_Empty;
                if (weapon_storage[weaponTube[n].type_loaded] < weapon_storage_max[weaponTube[n].type_loaded])
                    weapon_storage[weaponTube[n].type_loaded] ++;
                weaponTube[n].type_loaded = MW_None;
                break;
            default:
                break;
            }
        }
    }

    model_info.engine_scale = std::min(1.0, std::max(fabs(getAngularVelocity() / turn_speed), fabs(current_impulse)));
    if (has_jump_drive && jump_delay > 0.0f)
        model_info.warp_scale = (10.0f - jump_delay) / 10.0f;
    else
        model_info.warp_scale = 0.0;
}

P<SpaceObject> SpaceShip::getTarget()
{
    if (game_server)
        return game_server->getObjectById(target_id);
    return game_client->getObjectById(target_id);
}

void SpaceShip::executeJump(float distance)
{
    float f = systems[SYS_JumpDrive].health;
    if (f <= 0.0)
        return;
    distance = (distance * f) + (distance * (1.0 - f) * random(0.5, 1.5));
    sf::Vector2f target_position = getPosition() + sf::vector2FromAngle(getRotation()) * distance * 1000.0f;
    if (WarpJammer::isWarpJammed(target_position))
        target_position = WarpJammer::getFirstNoneJammedPosition(getPosition(), target_position);
    setPosition(target_position);
}

void SpaceShip::fireBeamWeapon(int index, P<SpaceObject> target)
{
    if (scanned_by_player == SS_NotScanned)
    {
        P<SpaceShip> ship = target;
        if (!ship || ship->scanned_by_player != SS_NotScanned)
            scanned_by_player = SS_FriendOrFoeIdentified;
    }

    sf::Vector2f hitLocation = target->getPosition() - sf::normalize(target->getPosition() - getPosition()) * target->getRadius();

    beam_weapons[index].cooldown = beam_weapons[index].cycleTime;
    P<BeamEffect> effect = new BeamEffect();
    effect->setSource(this, ship_template->model_data->getBeamPosition(index));
    effect->setTarget(target, hitLocation);

    DamageInfo info(this, DT_Energy, hitLocation);
    info.frequency = beam_frequency;
    info.system_target = beam_system_target;
    target->takeDamage(beam_weapons[index].damage, info);
}

bool SpaceShip::canBeDockedBy(P<SpaceObject> obj)
{
    if (isEnemy(obj) || !ship_template)
        return false;
    P<SpaceShip> ship = obj;
    if (!ship || !ship->ship_template)
        return false;
    return ship_template->size_class > ship->ship_template->size_class;
}

void SpaceShip::collide(Collisionable* other)
{
    if (docking_state == DS_Docking)
    {
        P<SpaceObject> dock_object = P<Collisionable>(other);
        if (dock_object == docking_target)
        {
            docking_state = DS_Docked;
            docking_offset = sf::rotateVector(getPosition() - other->getPosition(), -other->getRotation());
            float length = sf::length(docking_offset);
            docking_offset = docking_offset / length * (length + 2.0f);
        }
    }
}

void SpaceShip::loadTube(int tubeNr, EMissileWeapons type)
{
    if (tubeNr >= 0 && tubeNr < max_weapon_tubes && type > MW_None && type < MW_Count)
    {
        if (weaponTube[tubeNr].state == WTS_Empty && weapon_storage[type] > 0)
        {
            weaponTube[tubeNr].state = WTS_Loading;
            weaponTube[tubeNr].delay = tube_load_time;
            weaponTube[tubeNr].type_loaded = type;
            weapon_storage[type]--;
        }
    }
}

void SpaceShip::fireTube(int tubeNr, float target_angle)
{
    if (scanned_by_player == SS_NotScanned)
    {
        P<SpaceShip> ship = getTarget();
        if (getTarget() && (!ship || ship->scanned_by_player != SS_NotScanned))
            scanned_by_player = SS_FriendOrFoeIdentified;
    }

    if (docking_state != DS_NotDocking) return;
    if (current_warp > 0.0) return;
    if (tubeNr < 0 || tubeNr >= max_weapon_tubes) return;
    if (weaponTube[tubeNr].state != WTS_Loaded) return;

    sf::Vector2f fireLocation = getPosition() + sf::rotateVector(ship_template->model_data->getTubePosition2D(tubeNr), getRotation());
    switch(weaponTube[tubeNr].type_loaded)
    {
    case MW_Homing:
        {
            P<HomingMissile> missile = new HomingMissile();
            missile->owner = this;
            missile->setFactionId(getFactionId());
            missile->target_id = target_id;
            missile->setPosition(fireLocation);
            missile->setRotation(getRotation());
            missile->target_angle = target_angle;
        }
        break;
    case MW_Nuke:
        {
            P<Nuke> missile = new Nuke();
            missile->owner = this;
            missile->setFactionId(getFactionId());
            missile->target_id = target_id;
            missile->setPosition(fireLocation);
            missile->setRotation(getRotation());
            missile->target_angle = target_angle;
        }
        break;
    case MW_Mine:
        {
            P<Mine> missile = new Mine();
            missile->owner = this;
            missile->setFactionId(getFactionId());
            missile->setPosition(fireLocation);
            missile->setRotation(getRotation());
            missile->eject();
        }
        break;
    case MW_EMP:
        {
            P<EMPMissile> missile = new EMPMissile();
            missile->owner = this;
            missile->setFactionId(getFactionId());
            missile->target_id = target_id;
            missile->setPosition(fireLocation);
            missile->setRotation(getRotation());
            missile->target_angle = target_angle;
        }
        break;
    default:
        break;
    }
    weaponTube[tubeNr].state = WTS_Empty;
    weaponTube[tubeNr].type_loaded = MW_None;
}

void SpaceShip::initializeJump(float distance)
{
    if (docking_state != DS_NotDocking) return;
    if (jump_delay <= 0.0)
    {
        jump_distance = distance;
        jump_delay = 10.0;
    }
}

void SpaceShip::requestDock(P<SpaceObject> target)
{
    if (!target || docking_state != DS_NotDocking || !target->canBeDockedBy(this))
        return;
    if (sf::length(getPosition() - target->getPosition()) > 1000 + target->getRadius())
        return;
    if (has_jump_drive && jump_delay > 0.0f)
        return;

    docking_state = DS_Docking;
    docking_target = target;
}
void SpaceShip::requestUndock()
{
    if (docking_state == DS_Docked)
    {
        docking_state = DS_NotDocking;
        impulse_request = 0.5;
    }
}

void SpaceShip::takeDamage(float damage_amount, DamageInfo& info)
{
    if (shields_active)
    {
        float frequency_damage_factor = 1.0;
        if (info.type == DT_Energy && gameGlobalInfo->use_beam_shield_frequencies)
            frequency_damage_factor = frequencyVsFrequencyDamageFactor(info.frequency, shield_frequency);
        float angle = sf::angleDifference(getRotation(), sf::vector2ToAngle(getPosition() - info.location));
        ESystem system = SYS_FrontShield;
        bool front_hit = !(angle > -90 && angle < 90);
        float* shield = &front_shield;
        float* shield_hit_effect = &front_shield_hit_effect;
        if (!front_hit)
        {
            shield = &rear_shield;
            shield_hit_effect = &rear_shield_hit_effect;
            system = SYS_RearShield;
        }
        float shield_damage_factor = 1.25 - getSystemEffectiveness(system) * 0.25;

        *shield -= damage_amount * frequency_damage_factor * shield_damage_factor;

        if (*shield < 0)
        {
            takeHullDamage(-(*shield) / frequency_damage_factor, info);
            *shield = 0.0;
        }else{
            *shield_hit_effect = 1.0;
        }
    }else{
        takeHullDamage(damage_amount, info);
    }
}

void SpaceShip::takeHullDamage(float damage_amount, DamageInfo& info)
{
    if (info.type == DT_EMP)
        return;

    if (gameGlobalInfo->use_system_damage)
    {
        if (info.system_target != SYS_None)
        {
            //Target specific system
            float system_damage = (damage_amount / hull_max) * 1.0;
            if (info.type == DT_Energy)
                system_damage *= 3.0;   //Beam weapons do more system damage, as they penetrate the hull easier.
            systems[info.system_target].health -= system_damage;
            if (systems[info.system_target].health < -1.0)
                systems[info.system_target].health = -1.0;

            for(int n=0; n<2; n++)
            {
                ESystem random_system = ESystem(irandom(0, SYS_COUNT - 1));
                //Damage the system compared to the amount of hull damage you would do. If we have less hull strength you get more system damage.
                float system_damage = (damage_amount / hull_max) * 1.0;
                systems[random_system].health -= system_damage;
                if (systems[random_system].health < -1.0)
                    systems[random_system].health = -1.0;
            }

            if (info.type == DT_Energy)
                damage_amount *= 0.02;
            else
                damage_amount *= 0.5;
        }else{
            for(int n=0; n<5; n++)
            {
                ESystem random_system = ESystem(irandom(0, SYS_COUNT - 1));
                //Damage the system compared to the amount of hull damage you would do. If we have less hull strength you get more system damage.
                float system_damage = (damage_amount / hull_max) * 1.0;
                if (info.type == DT_Energy)
                    system_damage *= 3.0;   //Beam weapons do more system damage, as they penetrate the hull easier.
                systems[random_system].health -= system_damage;
                if (systems[random_system].health < -1.0)
                    systems[random_system].health = -1.0;
            }
        }
    }

    hull_strength -= damage_amount;
    if (hull_strength <= 0.0)
    {
        ExplosionEffect* e = new ExplosionEffect();
        e->setSize(getRadius() * 1.5);
        e->setPosition(getPosition());

        if (info.instigator)
        {
            if (isEnemy(info.instigator))
                info.instigator->addReputationPoints((hull_max + front_shield_max + rear_shield_max) * 0.1);
            else
                info.instigator->removeReputationPoints((hull_max + front_shield_max + rear_shield_max) * 0.1);
        }
        destroy();
    }
}

bool SpaceShip::hasSystem(ESystem system)
{
    switch(system)
    {
    case SYS_None:
    case SYS_COUNT:
        return false;
    case SYS_Warp:
        return has_warp_drive;
    case SYS_JumpDrive:
        return has_jump_drive;
    case SYS_MissileSystem:
        return weapon_tubes > 0;
    case SYS_FrontShield:
        return front_shield_max > 0;
    case SYS_RearShield:
        return rear_shield_max > 0;
    case SYS_Reactor:
        return true;
    case SYS_BeamWeapons:
        return true;
    case SYS_Maneuver:
        return turn_speed > 0.0;
    case SYS_Impulse:
        return impulse_max_speed > 0.0;
    }
    return true;
}

float SpaceShip::getSystemEffectiveness(ESystem system)
{
    float power = systems[system].power_level;
    if (energy_level < 10.0)
        power = std::max(0.1f, power);
    if (gameGlobalInfo->use_system_damage)
        return std::max(0.0f, power * systems[system].health);
    return std::max(0.0f, power * (1.0f - systems[system].heat_level));
}

string SpaceShip::getCallSign()
{
    return ship_callsign;
}

std::unordered_map<string, string> SpaceShip::getGMInfo()
{
    std::unordered_map<string, string> ret;
    ret["Type"] = ship_type_name;
    ret["Hull"] = string(int(hull_strength)) + "/" + string(int(hull_max));
    ret["FrontShield"] = string(int(front_shield)) + "/" + string(int(front_shield_max));
    ret["RearShield"] = string(int(rear_shield)) + "/" + string(int(rear_shield_max));
    return ret;
}

string getMissileWeaponName(EMissileWeapons missile)
{
    switch(missile)
    {
    case MW_None:
        return "-";
    case MW_Homing:
        return "Homing";
    case MW_Nuke:
        return "Nuke";
    case MW_Mine:
        return "Mine";
    case MW_EMP:
        return "EMP";
    default:
        return "UNK: " + string(int(missile));
    }
}

float frequencyVsFrequencyDamageFactor(int beam_frequency, int shield_frequency)
{
    if (beam_frequency < 0 || shield_frequency < 0)
        return 1.0;

    float diff = abs(beam_frequency - shield_frequency);
    float f1 = sinf(Tween<float>::linear(diff, 0, SpaceShip::max_frequency, 0, M_PI * (1.2 + shield_frequency * 0.05)) + M_PI / 2);
    f1 = f1 * Tween<float>::easeInCubic(diff, 0, SpaceShip::max_frequency, 1.0, 0.1);
    f1 = Tween<float>::linear(f1, 1.0, -1.0, 0.5, 1.5);
    return f1;
}

string frequencyToString(int frequency)
{
    return string(400 + (frequency * 20)) + "THz";
}
