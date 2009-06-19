/*
 * =====================================================================================
 *
 *       Filename:  TestWorld.h
 *
 *    Description:  Has the World and GameObject 'lists', for registration.
 *
 *        Created:  04/28/2009 03:59:06 PM
 *       Compiler:  gcc
 *
 *         Author:  Nikhilesh (nikki)
 *
 * =====================================================================================
 */

#include "Globals.h"

//GameObject headers.
#include "Objects/Main/Player.h"
#include "Objects/Main/StaticBrush.h"
#include "Objects/Misc/Light.h"
#include "Objects/Misc/CameraHandler.h"
#include "Objects/Misc/Point.h"
#include "Objects/Misc/Trigger.h"
#include "Objects/Misc/RigidBody.h"
#include "Objects/Obstacles/MovingBrush.h"
#include "Objects/Misc/Director.h"
#include "Objects/Obstacles/Crate.h"
#include "Objects/Misc/MessageBox.h"
#include "Objects/Obstacles/Bomb.h"
#include "Objects/Misc/ParticleEffect.h"
#include "Objects/Obstacles/Ice.h"
#include "Objects/Obstacles/SlidingBrush.h"

//World headers.
#include "Worlds/TestWorld.h"

//Register GameObject types.
void registerGameObjectTypes()
{
    NGF_REGISTER_OBJECT_TYPE(Player);
    NGF_REGISTER_OBJECT_TYPE(StaticBrush);
    NGF_REGISTER_OBJECT_TYPE(Light);
    NGF_REGISTER_OBJECT_TYPE(CameraHandler);
    NGF_REGISTER_OBJECT_TYPE(Point);
    NGF_REGISTER_OBJECT_TYPE(Trigger);
    NGF_REGISTER_OBJECT_TYPE(RigidBody);
    NGF_REGISTER_OBJECT_TYPE(MovingBrush);
    NGF_REGISTER_OBJECT_TYPE(Director);
    NGF_REGISTER_OBJECT_TYPE(Crate);
    NGF_REGISTER_OBJECT_TYPE(MessageBox);
    NGF_REGISTER_OBJECT_TYPE(Bomb);
    NGF_REGISTER_OBJECT_TYPE(ParticleEffect);
    NGF_REGISTER_OBJECT_TYPE(Ice);
    NGF_REGISTER_OBJECT_TYPE(SlidingBrush);
}

//Register worlds.
void addWorlds()
{
    GlbVar.woMgr->addWorld(new TestWorld());
}
