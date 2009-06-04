/*
=========================================
MovingBrush.cpp
=========================================
*/

#define __MOVINGBRUSH_CPP__

#include "Objects/Obstacles/MovingBrush.h"

//--- NGF events ----------------------------------------------------------------
MovingBrush::MovingBrush(Ogre::Vector3 pos, Ogre::Quaternion rot, NGF::ID id, NGF::PropertyList properties, Ogre::String name)
    : NGF::GameObject(pos, rot, id , properties, name),
      mTimer(-1),
      mLastFrameTime(0.1)
{
    addFlag("MovingBrush");

    //Python init event.
    NGF_PY_CALL_EVENT(init);

    //Get properties.
    mEnabled = Ogre::StringConverter::parseBool(properties.getValue("enabled", 0, "yes"));
    mVelocity = rot * Ogre::Vector3(0,0,-Ogre::StringConverter::parseReal(properties.getValue("speed", 0, "2")));

    //Create the Ogre stuff.
    mEntity = createBrushEntity();
    mNode = GlbVar.ogreSmgr->getRootSceneNode()->createChildSceneNode(mOgreName, pos, rot);
    mNode->attachObject(mEntity);

    //Create the Physics stuff.
    BtOgre::StaticMeshToShapeConverter converter(mEntity);
    mShape = converter.createConvex();
    mShape->setMargin(0); //Bad, but we gotta.

    BtOgre::RigidBodyState *state = new BtOgre::RigidBodyState(mNode);
    mBody = new btRigidBody(0, state, mShape);
    mBody->setCollisionFlags(mBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    mBody->setActivationState(DISABLE_DEACTIVATION);
    initBody();

    //Make smaller shape for cast.
    mCastShape = new btConvexHullShape(*mShape);
    mCastShape->setLocalScaling(btVector3(0.85,0.85,0.85));
}
//-------------------------------------------------------------------------------
void MovingBrush::postLoad()
{
    //Python create event.
    NGF_PY_CALL_EVENT(create);
}
//-------------------------------------------------------------------------------
MovingBrush::~MovingBrush()
{
    //Python destroy event.
    NGF_PY_CALL_EVENT(destroy);

    //We only clear up stuff that we did.
    delete mCastShape;
    destroyBody();
    delete mShape;

    mNode->detachAllObjects();
    GlbVar.ogreSmgr->destroyEntity(mEntity->getName());
}
//-------------------------------------------------------------------------------
void MovingBrush::unpausedTick(const Ogre::FrameEvent &evt)
{
    GraLL2GameObject::unpausedTick(evt);

    if (mEnabled)
    {
        //Timer.
        if (mTimer >= 0)
        {
            mTimer -= evt.timeSinceLastFrame;
        }

        //Get old place (current place actually).
        btTransform oldTrans;
        mBody->getMotionState()->getWorldTransform(oldTrans);
        btVector3 prevPos = oldTrans.getOrigin();

        //If we're near the next point on our list then we take it off our list and start moving to the next one (if it exists).
        //This way, you don't _need_ a point-list. You could do it using Directors, or even bouncing between walls, but when they're
        //there, it nicely blends in.
        if (!mPoints.empty())
        {
            Ogre::Vector3 currPoint = mPoints.front();
            Ogre::Real sqDist = (currPoint - BtOgre::Convert::toOgre(prevPos)).squaredLength();
            if (sqDist < 0.0001)
            {
                mBody->getMotionState()->setWorldTransform(btTransform(oldTrans.getRotation(), BtOgre::Convert::toBullet(currPoint)));
                mPoints.pop_front();
                if (!mPoints.empty())
                    mVelocity = (mPoints.front() - currPoint).normalisedCopy() * mVelocity.length();
            }
        }

        //Calculate new position.
        btVector3 currVel = BtOgre::Convert::toBullet(mVelocity) * evt.timeSinceLastFrame;
        btVector3 newPos = prevPos + currVel;

        //The cast result callback.
        struct MovingBrushCheckResult : public btDynamicsWorld::ConvexResultCallback
        {
            btCollisionObject *mIgnore;
            int mDimension;
            bool mHit;

            MovingBrushCheckResult(btCollisionObject *ignore, int dimension)
                : mIgnore(ignore),
                  mDimension(dimension),
                  mHit(false)
            {
            }

            btScalar addSingleResult(btDynamicsWorld::LocalConvexResult &convexResult, bool)
            {
                btCollisionObject *obj = convexResult.m_hitCollisionObject;
                //Don't record non-physics objects (Trigger etc.).
                mHit = !(obj->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE);

                return convexResult.m_hitFraction;
            }

            bool needsCollision(btBroadphaseProxy* proxy0) const
            {
                //If it's us, is the Player, or isn't in our dimension, we don't care.
                return ((btCollisionObject*) proxy0->m_clientObject != mIgnore) 
                    && (proxy0->m_collisionFilterGroup & mDimension);
            }
        };

        //Where to cast from, where to cast to, etc.
        btVector3 pos1 = prevPos;
        btVector3 castDisp = currVel + (currVel.normalized() * 0.15);
        castDisp.setY(0);
        btVector3 pos2 = prevPos + castDisp;
        btQuaternion rot = mBody->getWorldTransform().getRotation();
        btTransform trans1(rot, pos1);
        btTransform trans2(rot, pos2);

        //Do the cast.
        MovingBrushCheckResult res(mBody, mDimensions);
        GlbVar.phyWorld->convexSweepTest(mCastShape, trans1, trans2, res);

        //If free, continue, otherwise turn.
        if (res.mHit)
        {
            mVelocity = -mVelocity;
            currVel = -currVel;
            newPos = prevPos + currVel;
        }

        //Do the actual movement.
        oldTrans.setOrigin(newPos);
        mBody->getMotionState()->setWorldTransform(oldTrans);
        mNode->translate(mVelocity * evt.timeSinceLastFrame);
    }

    //Save frame time.
    mLastFrameTime = evt.timeSinceLastFrame;

    //Python utick event.
    NGF_PY_CALL_EVENT(utick, evt.timeSinceLastFrame);
}
//-------------------------------------------------------------------------------
void MovingBrush::pausedTick(const Ogre::FrameEvent &evt)
{
    //Python ptick event.
    NGF_PY_CALL_EVENT(ptick, evt.timeSinceLastFrame);
}
//-------------------------------------------------------------------------------
NGF::MessageReply MovingBrush::receiveMessage(NGF::Message msg)
{
    return GraLL2GameObject::receiveMessage(msg);
}
//-------------------------------------------------------------------------------
void MovingBrush::collide(NGF::GameObject *other, btCollisionObject *otherPhysicsObject, btManifoldPoint &contact)
{
    //If Director, get directed.
    if ((mTimer < 0) && other->hasFlag("Director"))
    {
        Ogre::Vector3 otherPos = GlbVar.goMgr->sendMessageWithReply<Ogre::Vector3>(other, NGF_MESSAGE(MSG_GETPOSITION));
        Ogre::Real sqDist = (otherPos - BtOgre::Convert::toOgre(mBody->getWorldTransform().getOrigin())).squaredLength();
        if (sqDist < 0.0001)
        {
            mBody->getMotionState()->setWorldTransform(btTransform(mBody->getWorldTransform().getRotation(), BtOgre::Convert::toBullet(otherPos)));
            mVelocity = GlbVar.goMgr->sendMessageWithReply<Ogre::Vector3>(other, NGF_MESSAGE(MSG_GETVELOCITY));

            //Wait for next time, to avoid sticking to this one.
            mTimer = 1/(mVelocity.length());
        }
    }

    //Python collide event.
    NGF::Python::PythonGameObject *oth = dynamic_cast<NGF::Python::PythonGameObject*>(other);
    if (oth)
        NGF_PY_CALL_EVENT(collide, oth->getConnector());
}
//-------------------------------------------------------------------------------

//--- Python interface implementation -------------------------------------------
NGF_PY_BEGIN_IMPL(MovingBrush)
{
    //setPosition
    NGF_PY_METHOD_IMPL(setPosition)
    {
        Ogre::Vector3 vec = py::extract<Ogre::Vector3>(args[0]);

        btTransform oldTrans;
        mBody->getMotionState()->getWorldTransform(oldTrans);
        mBody->getMotionState()->setWorldTransform(btTransform(oldTrans.getRotation(), BtOgre::Convert::toBullet(vec)));

        NGF_PY_RETURN();
    }
    //setOrientation
    NGF_PY_METHOD_IMPL(setOrientation)
    {
        Ogre::Quaternion rot = py::extract<Ogre::Quaternion>(args[0]);

        btTransform oldTrans;
        mBody->getMotionState()->getWorldTransform(oldTrans);
        mBody->getMotionState()->setWorldTransform(btTransform(BtOgre::Convert::toBullet(rot), oldTrans.getOrigin()));

        NGF_PY_RETURN();
    }
    NGF_PY_METHOD_IMPL(addPoint)
    {
        Ogre::Vector3 pos = py::extract<Ogre::Vector3>(args[0]);
        mPoints.push_back(pos);

        NGF_PY_RETURN();
    }

    NGF_PY_PROPERTY_IMPL(enabled, mEnabled, bool);
    NGF_PY_PROPERTY_IMPL(velocity, mVelocity, Ogre::Vector3);
}
NGF_PY_END_IMPL_BASE(GraLL2GameObject)