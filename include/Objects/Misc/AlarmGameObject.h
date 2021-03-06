/*
 * =====================================================================================
 *
 *       Filename:  AlarmGameObject.h
 *
 *    Description:  A GameObject interface for 'alram' events.
 *
 *        Created:  05/23/2009 08:46:32 PM
 *       Compiler:  gcc
 *
 *         Author:  Nikhilesh (nikki)
 *
 * =====================================================================================
 */

#ifndef __ALARMGAMEOBJECT_H__
#define __ALARMGAMEOBJECT_H__

#include <boost/serialization/map.hpp>

class AlarmGameObject
{
    protected:
        typedef unsigned int Alarm;

        typedef std::multimap<Ogre::Real, Alarm> AlarmMap;
        typedef std::pair<Ogre::Real, Alarm> AlarmPair;
        AlarmMap mAlarms;

        Ogre::Real mCurrTime;

    public:
        AlarmGameObject()
            : mCurrTime(0)
        {
        }

        //The alarm event.
        virtual void alarm(Alarm) {}

        //Set an alarm.
        void setAlarm(Ogre::Real time, Alarm alarm)
        {
            mAlarms.insert(AlarmPair(mCurrTime + time, alarm));
        }

        //Call per frame.
        void updateAlarms(Ogre::Real deltaTime)
        {
            mCurrTime += deltaTime;

            for (AlarmMap::iterator iter = mAlarms.begin(); 
                    iter != mAlarms.end() && iter->first < mCurrTime;)
            {
                alarm(iter->second);
                mAlarms.erase(iter++);
            }
        }
};

#define GRALL2_PY_ALARM_METHOD(func)                                                           \
    NGF_PY_METHOD_IMPL(func)                                                                   \
    {                                                                                          \
        setAlarm(py::extract<float>(args[0]), py::extract<Alarm>(args[1]));                    \
                                                                                               \
        return py::object();                                                                   \
    }

#define GRALL2_SERIALISE_ALARMS()                                                              \
    NGF_SERIALISE_STL_CONTAINER(mAlarms);                                                      \
    NGF_SERIALISE_OGRE(Real, mCurrTime);

#endif
