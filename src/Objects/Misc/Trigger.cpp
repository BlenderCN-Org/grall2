/*
=========================================
Trigger.cpp
=========================================
*/

#define __TRIGGER_CPP__

#include "Objects/Misc/Trigger.h"

//--- NGF events ----------------------------------------------------------------
Trigger::Trigger(Ogre::Vector3 pos, Ogre::Quaternion rot, NGF::ID id, NGF::PropertyList properties, Ogre::String name)
    : NGF::GameObject(pos, rot, id , properties, name),
      mTouched(false)
{
    addFlag("Trigger");

    //Python init event.
    NGF_PY_CALL_EVENT(init);
    NGF_PY_SAVE_EVENT(touchPlayer);

    //Create the Ogre stuff.
    mNode = GlbVar.ogreSmgr->getRootSceneNode()->createChildSceneNode(mOgreName, pos, rot);

    //Create the Physics stuff. Entity is temporary, only for trigger mesh conversion.
    Ogre::String mesh = mProperties.getValue("brushMeshFile", 0, "Player.mesh");
    Ogre::Entity *ent = GlbVar.ogreSmgr->createEntity(mOgreName, mesh);
    BtOgre::StaticMeshToShapeConverter converter(ent);

    mShape = converter.createTrimesh();
    GlbVar.ogreSmgr->destroyEntity(ent->getName());

    BtOgre::RigidBodyState *state = new BtOgre::RigidBodyState(mNode);
    mBody = new btRigidBody(0, state, mShape, btVector3(0,0,0));
    initBody( DimensionManager::NO_DIM_CHECK 
            | DimensionManager::NO_CRATE_CHECK
            | DimensionManager::NO_MOVING_CHECK
            | DimensionManager::NO_BULLET_HIT
            | DimensionManager::INVISIBLE
            );
    mBody->setCollisionFlags(mBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
}
//-------------------------------------------------------------------------------
void Trigger::postLoad()
{
    //Python create event.
    NGF_PY_CALL_EVENT(create);
}
//-------------------------------------------------------------------------------
Trigger::~Trigger()
{
    //Python destroy event.
    NGF_PY_CALL_EVENT(destroy);

    //We only clear up stuff that we did.
    destroyBody();
    delete mShape;
}
//-------------------------------------------------------------------------------
void Trigger::unpausedTick(const Ogre::FrameEvent &evt)
{
    GraLL2GameObject::unpausedTick(evt);
    
    //Python utick event.
    NGF_PY_CALL_EVENT(utick, evt.timeSinceLastFrame);
}
//-------------------------------------------------------------------------------
void Trigger::pausedTick(const Ogre::FrameEvent &evt)
{
    //Python ptick event.
    NGF_PY_CALL_EVENT(ptick, evt.timeSinceLastFrame);
}
//-------------------------------------------------------------------------------
NGF::MessageReply Trigger::receiveMessage(NGF::Message msg)
{
    return GraLL2GameObject::receiveMessage(msg);
}
//-------------------------------------------------------------------------------
void Trigger::collide(GameObject *other, btCollisionObject *otherPhysicsObject, btManifoldPoint &contact)
{
    if (!other)
        return;

    if (!mTouched && other->hasFlag("Player"))
    {
        NGF_PY_CALL_EVENT(touchPlayer);
        mTouched = true;
    }

    //Python collide event.
    NGF::Python::PythonGameObject *oth = dynamic_cast<NGF::Python::PythonGameObject*>(other);
    if (oth)
        NGF_PY_CALL_EVENT(collide, oth->getConnector());
}
//-------------------------------------------------------------------------------

//--- Python interface implementation -------------------------------------------
/*
NGF_PY_BEGIN_IMPL(Trigger)
{
}
NGF_PY_END_IMPL_BASE(GraLL2GameObject)
*/
