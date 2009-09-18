/*
=========================================
Turret.cpp
=========================================
*/

#define __TURRET_CPP__

#include "Objects/Bots/Turret.h"

#include "Objects/Bots/Bullet.h"

#define TOP_MOVE_DISTANCE 0.19f
#define SHOOT_OFFSET (Ogre::Vector3(0,0.9,0))
#define TOP_MOVE_TIME 1.0f
#define FIRST_BULLET_TIME 1
#define BULLET_TIME 0.2

//--- NGF events ----------------------------------------------------------------
Turret::Turret(Ogre::Vector3 pos, Ogre::Quaternion rot, NGF::ID id, NGF::PropertyList properties, Ogre::String name)
    : NGF::GameObject(pos, rot, id , properties, name),
      mBulletTimer(FIRST_BULLET_TIME),
      mStateTimer(-1),
      mState(TS_REST)
{
    addFlag("Turret");

    //Python init event.
    NGF_PY_CALL_EVENT(init);

    //Read properties.
    mFireTime = Ogre::StringConverter::parseReal(mProperties.getValue("fireTime", 0, "7"));
    mRestTime = Ogre::StringConverter::parseReal(mProperties.getValue("restTime", 0, "5"));

    //Create the Ogre stuff.
    mBaseEntity = GlbVar.ogreSmgr->createEntity(mOgreName + "_base", "Mesh_TurretBase.mesh");
    mTopEntity = GlbVar.ogreSmgr->createEntity(mOgreName + "_top", "Mesh_TurretTop.mesh");
    mBaseEntity->setMaterialName("Objects/Crate1");
    mTopEntity->setMaterialName("Objects/Crate1");

    mNode = GlbVar.ogreSmgr->getRootSceneNode()->createChildSceneNode(mOgreName, pos, rot);
    mBaseNode = mNode->createChildSceneNode(mOgreName + "_base");
    mTopNode = mNode->createChildSceneNode(mOgreName + "_top", Ogre::Vector3(0,-TOP_MOVE_DISTANCE,0));

    mBaseNode->attachObject(mBaseEntity);
    mTopNode->attachObject(mTopEntity);

    //Create the Physics stuff.
    Ogre::Entity *colMesh = GlbVar.ogreSmgr->createEntity(mOgreName + "_collision", "Collision_Turret.mesh");
    BtOgre::StaticMeshToShapeConverter converter(colMesh);
    mShape = converter.createTrimesh();
    GlbVar.ogreSmgr->destroyEntity(colMesh);

    BtOgre::RigidBodyState *state = new BtOgre::RigidBodyState(mNode);
    mBody = new btRigidBody(0, state, mShape);

    initBody( DimensionManager::NO_BULLET_HIT
            );
    setBulletObject(mBody);

    //Fire!
    startFiring();
}
//-------------------------------------------------------------------------------
void Turret::postLoad()
{
    //Python create event.
    NGF_PY_CALL_EVENT(create);
}
//-------------------------------------------------------------------------------
Turret::~Turret()
{
    //Python destroy event.
    NGF_PY_CALL_EVENT(destroy);

    //We only clear up stuff that we did.
    destroyBody();
    delete mShape;

    mNode->detachAllObjects();
    mTopNode->detachAllObjects();
    mBaseNode->detachAllObjects();
    GlbVar.ogreSmgr->destroyEntity(mTopEntity->getName());
    GlbVar.ogreSmgr->destroyEntity(mBaseEntity->getName());
    GlbVar.ogreSmgr->destroySceneNode(mBaseNode);
    GlbVar.ogreSmgr->destroySceneNode(mTopNode);
}
//-------------------------------------------------------------------------------
void Turret::unpausedTick(const Ogre::FrameEvent &evt)
{
    GraLL2GameObject::unpausedTick(evt);

    //Ze ol' switch-case FSM. :D
    switch (mState)
    {
        case TS_RESTTOFIRE:
            {
                Ogre::Real speed = (TOP_MOVE_DISTANCE / TOP_MOVE_TIME) * evt.timeSinceLastFrame;

                if (-(mTopNode->getPosition().y) < speed)
                {
                    mTopNode->setPosition(Ogre::Vector3::ZERO);
                    fire();
                    break;
                }
                else
                {
                    mTopNode->translate(Ogre::Vector3(0,speed,0));
                }
            }
            break;
        case TS_FIRETOREST:
            {
                Ogre::Real speed = (TOP_MOVE_DISTANCE / TOP_MOVE_TIME) * evt.timeSinceLastFrame;

                if (mTopNode->getPosition().y + TOP_MOVE_DISTANCE < speed)
                {
                    mTopNode->setPosition(Ogre::Vector3(0,-TOP_MOVE_DISTANCE,0));
                    rest();
                    break;
                }
                else
                {
                    mTopNode->translate(Ogre::Vector3(0,-speed,0));
                }
            }
            break;

        case TS_FIRE:
            {
                mStateTimer -= evt.timeSinceLastFrame;

                if (mStateTimer > 0)
                {
                    mBulletTimer -= evt.timeSinceLastFrame;

                    if (mBulletTimer < 0)
                    {
                        fireSingleBullet();
                        mBulletTimer = BULLET_TIME + Ogre::Math::RangeRandom(-0.07,0.07);
                    }
                }
                else
                    stopFiring();
            }
            break;
        case TS_REST:
            {
                mStateTimer -= evt.timeSinceLastFrame;
                
                if (mStateTimer < 0)
                    startFiring();
            }
            break;
    }
    
    //Python utick event.
    NGF_PY_CALL_EVENT(utick, evt.timeSinceLastFrame);
}
//-------------------------------------------------------------------------------
void Turret::pausedTick(const Ogre::FrameEvent &evt)
{
    //Python ptick event.
    NGF_PY_CALL_EVENT(ptick, evt.timeSinceLastFrame);
}
//-------------------------------------------------------------------------------
NGF::MessageReply Turret::receiveMessage(NGF::Message msg)
{
    return GraLL2GameObject::receiveMessage(msg);
}
//-------------------------------------------------------------------------------
void Turret::collide(GameObject *other, btCollisionObject *otherPhysicsObject, btManifoldPoint &contact)
{
    if (!other)
        return;

    //Python collide event.
    NGF::Python::PythonGameObject *oth = dynamic_cast<NGF::Python::PythonGameObject*>(other);
    if (oth)
        NGF_PY_CALL_EVENT(collide, oth->getConnector());
}
//-------------------------------------------------------------------------------

//--- Non-NGF -------------------------------------------------------------------
void Turret::startFiring()
{
    if (mState != TS_FIRE)
        mState = TS_RESTTOFIRE;
}
//-------------------------------------------------------------------------------
void Turret::stopFiring()
{
    if (mState != TS_REST)
        mState = TS_FIRETOREST;
}
//-------------------------------------------------------------------------------
void Turret::fire()
{
    mState = TS_FIRE;
    mStateTimer = mFireTime;
    mBulletTimer = FIRST_BULLET_TIME;
}
//-------------------------------------------------------------------------------
void Turret::rest()
{
    mState = TS_REST;
    mStateTimer = mRestTime;
}
//-------------------------------------------------------------------------------
void Turret::fireSingleBullet()
{
    if (GlbVar.player)
    {
        Ogre::Vector3 playerPos = GlbVar.goMgr->sendMessageWithReply<Ogre::Vector3>(GlbVar.player, NGF_MESSAGE(MSG_GETPOSITION));
        Ogre::Vector3 shootPos = mNode->getPosition() + SHOOT_OFFSET;
        Ogre::Vector3 dir = (playerPos - shootPos).normalisedCopy().randomDeviant(Ogre::Radian(Ogre::Math::UnitRandom() * 0.034));
        Ogre::Quaternion shootRot = Ogre::Vector3::NEGATIVE_UNIT_Z.getRotationTo(dir);
        Ogre::Vector3 bulletPos = shootPos + (shootRot * Ogre::Vector3(0,0,-0.25)); //We make the bullet a little bit in it's direction.

        GlbVar.goMgr->createObject<Bullet>(bulletPos, shootRot, NGF::PropertyList::create
                ("dimensions", Ogre::StringConverter::toString(GlbVar.dimMgr->getCurrentDimension()))
                );
    }
    else
    {
        //Our work is done. ;-)
        stopFiring();
    }
}
//-------------------------------------------------------------------------------

//--- Python interface implementation -------------------------------------------
NGF_PY_BEGIN_IMPL(Turret)
{
    NGF_PY_METHOD_IMPL(startFiring)
    {
        startFiring();
        NGF_PY_RETURN();
    }
    NGF_PY_METHOD_IMPL(stopFiring)
    {
        stopFiring();
        NGF_PY_RETURN();
    }
}
NGF_PY_END_IMPL_BASE(GraLL2GameObject)