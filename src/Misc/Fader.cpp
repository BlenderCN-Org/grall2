/*
=========================================
Fader.cpp
=========================================
*/

#include "Misc/Fader.h"

void Fader::tick(const Ogre::FrameEvent &evt)
{
    switch (mState)
    {
        case FM_IN:
            mImage->setAlpha(Util::clamp(mImage->getAlpha() + mRate * evt.timeSinceLastFrame, 0.0f, 1.0f));

            if (mImage->getAlpha() == 1)
                mState = FM_NONE;
            break;

        case FM_OUT:
            mImage->setAlpha(Util::clamp(mImage->getAlpha() - mRate * evt.timeSinceLastFrame, 0.0f, 1.0f));

            if (mImage->getAlpha() == 0)
                mState = FM_NONE;
            break;

        case FM_INOUT:
            if (mImage->getAlpha() < 1)
            {
                mImage->setAlpha(Util::clamp(mImage->getAlpha() + mRate * evt.timeSinceLastFrame, 0.0f, 1.0f));
            } else if (mPause > 0)
            {
                mPause -= evt.timeSinceLastFrame;
            }
            else
            {
                mState = FM_OUT;
                mRate = 1 / mFadeInOutOutTime;
            }
            break;

        default:
            return;
    }
    mImage->setCoord(MyGUI::IntCoord(0,0,GlbVar.ogreWindow->getWidth(),GlbVar.ogreWindow->getHeight()));
}
