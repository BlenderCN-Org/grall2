/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  The main source file for GraLL 2.
 *
 *        Created:  04/26/2009 07:35:28 PM
 *       Compiler:  gcc
 *
 *         Author:  Nikhilesh (nikki)
 *
 * =====================================================================================
 */

/* 
 * Testing shortcut keys
 *
 *    F1, F2   - Next world, previous world.
 *    F3, F4   - Toggle console visibility, run code
 *    F5, F6   - Start recording video, stop recording video (frames in usr/Video)
 *    F7, F8   - Dump game state, load game state
 *    F9       - Screenshot 
 *    F10      - Show physics collision shapes
 *    F12      - Exit
 *    Ctrl+X   - Pick object (stored as 'clicked' in Python)
 *    Ctrl+O   - Show Options dialog
 *    Ctrl+P   - Pause game
 *
 */

#include "Globals.h"

#if (OGRE_PLATFORM == OGRE_PLATFORM_WIN32)
#define USE_OWN_PY_STDLIB
#endif

#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "MyGUI_OgrePlatform.h"

#if (OGRE_VERSION < ((1 << 16) | (8 << 8)))
template<> Globals* Ogre::Singleton<Globals>::ms_Singleton = 0;
#else
template<> Globals* Ogre::Singleton<Globals>::msSingleton = 0;
#endif

//--------------------------------------------------------------------------------------
class WindowListener : 
    public Ogre::WindowEventListener
{
    public:
        void windowResized(Ogre::RenderWindow *win)
        {
            //Update viewports
            unsigned int vps = win->getNumViewports();
            for (unsigned int j = 0; j < vps; ++j)
            {
                Ogre::Viewport *vp = win->getViewport(j);

                Ogre::Camera *cam = vp->getCamera();
                if (cam)
                    cam->setAspectRatio(((float) vp->getActualWidth()) / (vp->getActualHeight()));

                if (Ogre::CompositorManager::getSingleton().hasCompositorChain(vp))
                {
                    Ogre::CompositorChain *chain = 
                        Ogre::CompositorManager::getSingleton().getCompositorChain(vp);
                    unsigned int n = chain->getNumCompositors();
                    for (int i = 0; i < n; ++i)
                        if (chain->getCompositor(i)->getEnabled())
                        {
                            chain->setCompositorEnabled(i, false);
                            chain->setCompositorEnabled(i, true);
                        }
                }
            }
        }
};

//--------------------------------------------------------------------------------------
class MaterialListener : 
    public Ogre::MaterialManager::Listener
{
    protected:
        Ogre::MaterialPtr mBlackMat;
    public:
        MaterialListener()
        {
            mBlackMat = Ogre::MaterialManager::getSingleton().create("mBlack", "Internal");
            mBlackMat->getTechnique(0)->getPass(0)->setDiffuse(0,0,0,0);
            mBlackMat->getTechnique(0)->getPass(0)->setSpecular(0,0,0,0);
            mBlackMat->getTechnique(0)->getPass(0)->setAmbient(0,0,0);
            mBlackMat->getTechnique(0)->getPass(0)->setSelfIllumination(0,0,0);
        }

        Ogre::Technique *handleSchemeNotFound(unsigned short, const Ogre::String& schemeName, 
                Ogre::Material*, unsigned short, const Ogre::Renderable*)
        {
            return mBlackMat->getTechnique(0);
        }
}; 

//--------------------------------------------------------------------------------------
class GameListener :
    public Ogre::FrameListener,
    public OIS::KeyListener,
    public OIS::MouseListener
{
    protected:
        static OIS::KeyCode mCurrKey; //Stores keycode to broadcast to GameObjects.

        bool mLightingPrev;

    public:
        GameListener()
            : mLightingPrev(GlbVar.settings.graphics.lighting)
        {
        }

        void updateLightingSetting()
        {
            mLightingPrev = GlbVar.settings.graphics.lighting;
            if (GlbVar.settings.graphics.lighting)
                GlbVar.ogreSmgr->setAmbientLight(Ogre::ColourValue(0.3,0.3,0.3));
            else
                GlbVar.ogreSmgr->setAmbientLight(Ogre::ColourValue(0.49, 0.49, 0.49));
            GlbVar.sun->setVisible(GlbVar.settings.graphics.lighting);
        }

        bool frameStarted(const Ogre::FrameEvent &evt)
        {
            //GUI update.
            //MyGUI::InputManager::getInstance().injectFrameEntered(evt.timeSinceLastFrame);

            //Handle lighting options changes.
            if (GlbVar.settings.graphics.lighting != mLightingPrev)
                updateLightingSetting();
            if (GlbVar.settings.graphics.lighting)
                GlbVar.sun->setCastShadows(GlbVar.settings.graphics.shadows);

            //Physics update.
            if (!GlbVar.paused)
                GlbVar.phyWorld->stepSimulation(evt.timeSinceLastFrame, 7);
            GlbVar.phyWorld->debugDrawWorld();

            //Shows debug if F10 key down.
            GlbVar.phyDebug->setDebugMode(GlbVar.keyboard->isKeyDown(OIS::KC_F10));
            GlbVar.phyDebug->step();

            //Update helpers.
            GlbVar.dimMgr->tick();
            GlbVar.fader->tick(evt);
            GlbVar.musicMgr->tick(evt);
            GlbVar.videoRec->tick(evt);
            GlbVar.optionsDialog->tick();
            GlbVar.objectClicker->tick();
            GlbVar.hud->tick(evt);

            //Log FPS
            //Ogre::LogManager::getSingleton().logMessage(FORMAT("FPS: %1%", GlbVar.ogreWindow->getLastFPS()));

            //NGF update.
            GlbVar.goMgr->tick(GlbVar.paused, evt);
            return GlbVar.woMgr->tick(evt);
        }

        //--- Send keypress events to GameObjects, and all events to MyGUI -------------
        static void sendKeyPressMessage(NGF::GameObject *obj)
        {
            GlbVar.goMgr->sendMessage(obj, NGF_MESSAGE(MSG_KEYPRESSED, mCurrKey));
        }
        bool keyPressed(const OIS::KeyEvent &arg)
        {
            mCurrKey = arg.key;

            //Tell helpers.
            GlbVar.console->keyPressed(mCurrKey);
            GlbVar.optionsDialog->keyPressed(mCurrKey);

            //Tell MyGUI.
            MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Enum(arg.key), arg.text);

            //Tell all GameObjects.
            if (!GlbVar.console->isVisible())
                GlbVar.goMgr->forEachGameObject(GameListener::sendKeyPressMessage);

            //Some debug keys. TODO: Remove these in final version.
            switch (mCurrKey)
            {
                case OIS::KC_F1:
                    GlbVar.woMgr->previousWorld();
                    break;
                case OIS::KC_F2:
                    GlbVar.woMgr->nextWorld();
                    break;

                case OIS::KC_F5:
                    GlbVar.videoRec->startRecording(0.1);
                    break;
                case OIS::KC_F6:
                    GlbVar.videoRec->stopRecording();
                    break;

                case OIS::KC_F7:
                    Util::serialise("TestSave");
                    break;
                case OIS::KC_F8:
                    Util::clearLevel();
                    Util::deserialise("TestSave");
                    break;

                case OIS::KC_F9:
                    //Util::highResScreenshot(GlbVar.ogreWindow, GlbVar.ogreCamera, 3, "HiResScreenshot", ".png", true);
                    Util::screenshot("Screenshot", ".png");
                    GlbVar.ogreSmgr->getShadowTexture(0)->getBuffer()->getRenderTarget()->writeContentsToFile("shadow.png");
                    break;

                case OIS::KC_X:
                    if (GlbVar.keyboard->isKeyDown(OIS::KC_LCONTROL))
                        GlbVar.objectClicker->click();
                    break;

                case OIS::KC_O:
                    if (GlbVar.keyboard->isKeyDown(OIS::KC_LCONTROL))
                        GlbVar.optionsDialog->setVisible(!(GlbVar.console->isVisible()));
                    break;

                case OIS::KC_P:
                    if (GlbVar.keyboard->isKeyDown(OIS::KC_LCONTROL))
                        GlbVar.paused = !GlbVar.paused;
                    break;

                case OIS::KC_R:
                    if (GlbVar.keyboard->isKeyDown(OIS::KC_LCONTROL))
                        Util::reloadMaterials();
                    break;
            }

            return true;
        }
        bool keyReleased(const OIS::KeyEvent &arg)
        {
            MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Enum(arg.key));
            return true;
        }

        bool mouseMoved(const OIS::MouseEvent &arg)
        {
            MyGUI::InputManager::getInstance().injectMouseMove(arg.state.X.abs, arg.state.Y.abs, arg.state.Z.abs);
            return true;
        }
        bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
        {
            MyGUI::InputManager::getInstance().injectMousePress(arg.state.X.abs, arg.state.Y.abs, MyGUI::MouseButton::Enum(id));
            return true;
        }
        bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
        {
            MyGUI::InputManager::getInstance().injectMouseRelease(arg.state.X.abs, arg.state.Y.abs, MyGUI::MouseButton::Enum(id));
            return true;
        }
};
OIS::KeyCode GameListener::mCurrKey = OIS::KC_UNASSIGNED;

//--------------------------------------------------------------------------------------
class Game
{
    protected:
        OIS::InputManager *mInputManager;

        GameListener *mGameListener;
        MaterialListener *mMaterialListener;
        WindowListener *mWindowListener;

        btAxisSweep3 *mBroadphase;
        btDefaultCollisionConfiguration *mCollisionConfig;
        btCollisionDispatcher *mDispatcher;
        btSequentialImpulseConstraintSolver *mSolver;

    public:
        bool init(char *progname)
        {
            //--- Globals, settings ----------------------------------------------------
            new Globals();
            GlbVar.sun = NULL;
            GlbVar.keyMap = new KeyMap();

            //We don't 'require' Settings.ini because if it doesn't exist we use the one
            //in DATA_PREFIX.
            Util::requireDirectory(USER_PREFIX);
            Util::requireFile(USER_PREFIX "Record");
            Util::requireDirectory(USER_PREFIX "Content");
            Util::requireDirectory(USER_PREFIX "Saves");
            Util::requireDirectory(USER_PREFIX "Screenshots");
            Util::requireDirectory(USER_PREFIX "Video");

            loadSettings();

            Util::writeShaderConfig();

            //--- Ogre (Graphics) ------------------------------------------------------
            //Root.
            GlbVar.ogreRoot = new Ogre::Root("", "", USER_PREFIX "main.log");

            //Plugins.
            switch (GlbVar.settings.ogre.renderer)
            {
                case Globals::Settings::OgreSettings::DIRECT3D:
                    GlbVar.ogreRoot->loadPlugin(GlbVar.settings.ogre.pluginDirectory + "/RenderSystem_Direct3D9");
                    break;

                case Globals::Settings::OgreSettings::OPENGL:
                    GlbVar.ogreRoot->loadPlugin(GlbVar.settings.ogre.pluginDirectory + "/RenderSystem_GL");
                    break;
            }

            for (Ogre::StringVector::iterator iter = GlbVar.settings.ogre.plugins.begin();
                    iter != GlbVar.settings.ogre.plugins.end(); ++iter)
                GlbVar.ogreRoot->loadPlugin(GlbVar.settings.ogre.pluginDirectory + "/" + (*iter));

            //Resources.
            Util::addResourceLocationRecursive(DATA_PREFIX, "General");
            Util::addResourceLocationRecursive(USER_PREFIX "Content", "General");

            //Renderer. Just choose the first available one, we just load only one plugin (either
            //Direct3D or OpenGL) anyway.
            const Ogre::RenderSystemList &renderers = GlbVar.ogreRoot->getAvailableRenderers();
            Ogre::RenderSystem *renderer = *(renderers.begin());
            GlbVar.ogreRoot->setRenderSystem(renderer);

            //Window.
            GlbVar.ogreRoot->initialise(false);

            Ogre::NameValuePairList winParams;
            winParams["FSAA"] = GlbVar.settings.ogre.FSAA;
            winParams["vsync"] = GlbVar.settings.ogre.vsync;

            GlbVar.ogreWindow = GlbVar.ogreRoot->createRenderWindow(
                    "GraLL 2",
                    GlbVar.settings.ogre.winWidth, 
                    GlbVar.settings.ogre.winHeight,
                    GlbVar.settings.ogre.winFullscreen,
                    &winParams
                    );
            GlbVar.ogreWindow->setActive(true);
            GlbVar.ogreWindow->setAutoUpdated(true);

            //SceneManager.
            GlbVar.ogreSmgr = GlbVar.ogreRoot->createSceneManager(Ogre::ST_GENERIC);

            //Camera, Viewport.
            GlbVar.ogreCamera = GlbVar.ogreSmgr->createCamera("mainCamera");
            Ogre::Viewport *viewport = GlbVar.ogreWindow->addViewport(GlbVar.ogreCamera);

            viewport->setDimensions(0,0,1,1);
            GlbVar.ogreCamera->setAspectRatio((float)viewport->getActualWidth() / (float) viewport->getActualHeight());
            GlbVar.ogreCamera->setFarClipDistance(1000.0);
            GlbVar.ogreCamera->setNearClipDistance(0.1);

            GlbVar.camNode = GlbVar.ogreSmgr->getRootSceneNode()->createChildSceneNode("camNode");
            GlbVar.camNode->attachObject(GlbVar.ogreCamera);

            //--- OIS (Input) ----------------------------------------------------------
            OIS::ParamList inputParams;
            size_t windowHnd = 0;

            GlbVar.ogreWindow->getCustomAttribute("WINDOW", &windowHnd);
            inputParams.insert(std::make_pair(Ogre::String("WINDOW"), Ogre::StringConverter::toString(windowHnd)));
            //inputParams.insert(std::make_pair(Ogre::String("x11_mouse_grab"), Ogre::String("false")));
            inputParams.insert(std::make_pair(Ogre::String("x11_keyboard_grab"), Ogre::String("false")));

            mInputManager = OIS::InputManager::createInputSystem(inputParams);
            GlbVar.keyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject(OIS::OISKeyboard, true));
            GlbVar.mouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject(OIS::OISMouse, true));

            unsigned int width, height, depth; int top, left;
            GlbVar.ogreWindow->getMetrics(width, height, depth, left, top);
            const OIS::MouseState &ms = GlbVar.mouse->getMouseState();
            ms.width = width;
            ms.height = height;

            //--- Listeners ------------------------------------------------------------
            mGameListener = new GameListener();
            GlbVar.ogreRoot->addFrameListener(mGameListener);
            GlbVar.keyboard->setEventCallback(mGameListener);
            GlbVar.mouse->setEventCallback(mGameListener);

            mMaterialListener = new MaterialListener();
            Ogre::MaterialManager::getSingleton().addListener(mMaterialListener);

            mWindowListener = new WindowListener();
            Ogre::WindowEventUtilities::addWindowEventListener(GlbVar.ogreWindow, mWindowListener);

            //--- Bullet (Physics) -----------------------------------------------------
            mBroadphase = new btAxisSweep3(btVector3(-10000,-10000,-10000), btVector3(10000,10000,10000), 1024);
            mBroadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
            mCollisionConfig = new btDefaultCollisionConfiguration();
            mDispatcher = new btCollisionDispatcher(mCollisionConfig);
            mSolver = new btSequentialImpulseConstraintSolver();

            GlbVar.phyWorld = new btDiscreteDynamicsWorld(mDispatcher, mBroadphase, mSolver, mCollisionConfig);

            GlbVar.phyDebug = new BtOgre::DebugDrawer(GlbVar.ogreSmgr->getRootSceneNode(), GlbVar.phyWorld);
            GlbVar.phyWorld->setDebugDrawer(GlbVar.phyDebug);

            //--- MyGUI (GUI) ----------------------------------------------------------
            MyGUI::OgrePlatform *guiPlatform = new MyGUI::OgrePlatform();
            guiPlatform->initialise(GlbVar.ogreWindow, GlbVar.ogreSmgr, "General", USER_PREFIX "gui.log");
            GlbVar.gui = new MyGUI::Gui();
            GlbVar.gui->initialise("MyGUI_Core.xml");
            MyGUI::ResourceManager::getInstance().load("MessageBoxResources.xml");

            //--- OgreAL (Sound) -------------------------------------------------------
            GlbVar.soundMgr = new OgreAL::SoundManager();
            GlbVar.camNode->attachObject(GlbVar.soundMgr->getListener());

            //Make some 'persistent' sounds (usually played on destruction so not able to manage).
            //Yes, hax. :/
            GlbVar.playerExplosionSound = GlbVar.soundMgr->createSound("PlayerExplosion", "PlayerExplosion.wav", false, false);
            GlbVar.playerExplosionSound->setReferenceDistance(1.2);
            GlbVar.playerExplosionSound->setGain(0.42);

            GlbVar.bombExplosionSound = GlbVar.soundMgr->createSound("BombExplosion", "BombExplosion.wav", false, false);
            GlbVar.bombExplosionSound->setReferenceDistance(1.2);
            //GlbVar.playerExplosionSound->setGain(0.42);

            //--- NGF (Game architecture framework) ------------------------------------
            //Usual stuff.
            GlbVar.goMgr = new NGF::GameObjectManager();
            GlbVar.woMgr = new NGF::WorldManager();
            GlbVar.lvlLoader = new NGF::Loading::Loader();

            //Python, GraLL2 python bindings, python console. Tell Python to use packaged stdlib if needed.
#ifdef USE_OWN_PY_STDLIB
            Py_SetProgramName(progname);
            Py_SetPath(DATA_PREFIX "Python/python27.zip");
            Py_SetPythonHome(DATA_PREFIX "Python");
#endif
            Py_Initialize();
            GlbVar.console = new Console();
            new NGF::Python::PythonManager(fastdelegate::MakeDelegate(GlbVar.console, &Console::print));
            initPythonBinds();

            //--- Init resources and other stuff ---------------------------------------
            //Initialise the ResourceGroups.
            Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

            //Shadows.
            initShadows();

            //Compositor chain.
            Ogre::CompositorManager::getSingleton().addCompositor(viewport, "Compositor/Glow");
            Ogre::CompositorManager::getSingleton().addCompositor(viewport, "Compositor/Dimension2");
            Ogre::CompositorManager::getSingleton().setCompositorEnabled(viewport, "Compositor/Glow", true);

            //Preloader.
            Ogre::SceneNode *preloadNode = GlbVar.ogreSmgr->getRootSceneNode()->createChildSceneNode();
            GlbVar.preloadEntity = GlbVar.ogreSmgr->createEntity(Ogre::SceneManager::PT_PLANE);
            preloadNode->attachObject(GlbVar.preloadEntity);
            preloadNode->setVisible(false);
            Util::preloadMaterial("ParticleFX/Flare");

            return true;
        }

        void start(char *ngfName = NULL)
        {
            //Not paused in the beginning.
            GlbVar.paused = false;

            //Create helpers.
            GlbVar.dimMgr = new DimensionManager();
            GlbVar.fader = new Fader();
            GlbVar.musicMgr = new MusicManager();
            GlbVar.videoRec = new VideoRecorder();
            GlbVar.optionsDialog = new OptionsDialog();
            GlbVar.objectClicker = new ObjectClicker();
            GlbVar.hud = new HUD();
            GlbVar.gravMgr = new GravityManager();

            //The persistent Controller GameObject.
            GlbVar.controller = 0;//GlbVar.goMgr->createObject<Controller>(Ogre::Vector3::ZERO, Ogre::Quaternion::ZERO);

            //Initialise other Global variables.
            GlbVar.player = 0;
            GlbVar.currCameraHandler = 0;
            GlbVar.currMessageBox = 0;
            GlbVar.currSlideshow = 0;
            GlbVar.worldSwitch = -1;
            GlbVar.loadGame = true;
            GlbVar.bonusTime = 0;
            GlbVar.commandLine = false;

            //Add Worlds, register GameObjects.
            addWorlds();
            registerGameObjectTypes();

            //Load user record (highest level, times, scores).
            loadRecord();

            //List of user levels.
            std::vector<Ogre::String> levels = GlbVar.lvlLoader->getLevels();
            for (std::vector<Ogre::String>::iterator iter = levels.begin(); iter != levels.end(); ++iter)
                if (std::find(GlbVar.ngfNames.begin(), GlbVar.ngfNames.end(), *iter) == GlbVar.ngfNames.end())
                    GlbVar.userNgfNames.push_back(*iter);

            //Run the startup script (we do it here so that all managers and stuff are created and initialised).
            runPythonStartupScript();

            //Main lighting.
            Util::reloadSun();
            mGameListener->updateLightingSetting();

            //Start running the Worlds.
            if (ngfName)
            {
                GlbVar.commandLine = true;
                Util::loadUserLevel(ngfName); //Load NGF script
            }
            else
                GlbVar.woMgr->start(0); //Normal game
        }

        void loop()
        {
            while (7)
            {
                //Window message pumping, events etc.
                Ogre::WindowEventUtilities::messagePump();

                //Update input.
                GlbVar.keyboard->capture();
                GlbVar.mouse->capture();

                //If we need to switch Worlds, do so, but not again.
                if (GlbVar.worldSwitch != -1)
                {
                    //If 'special' index, go forward or backward, else just jump.
                    if (GlbVar.worldSwitch < 0)
                        if (GlbVar.worldSwitch == PREV_WORLD)
                            GlbVar.woMgr->previousWorld();
                        else
                            GlbVar.woMgr->nextWorld();
                    else
                        GlbVar.woMgr->gotoWorld(GlbVar.worldSwitch);
                    GlbVar.worldSwitch = -1;
                }

                //Exit if F12 pressed.
                if (GlbVar.keyboard->isKeyDown(OIS::KC_F12) || GlbVar.ogreWindow->isClosed())
                    GlbVar.woMgr->shutdown();

                //Update Ogre.
                if(!GlbVar.ogreRoot->renderOneFrame())
                    break;
            }
        }

        void shutdown()
        {
            //Controller can't truly live forever. :P
            if (GlbVar.controller)
                GlbVar.goMgr->destroyObject(GlbVar.controller->getID());

            //Save settings, user record.
            saveRecord();
            saveSettings();
            Util::writeShaderConfig();

            //Helpers.
            delete GlbVar.gravMgr;
            delete GlbVar.hud;
            delete GlbVar.objectClicker;
            delete GlbVar.optionsDialog;
            delete GlbVar.videoRec;
            delete GlbVar.musicMgr;
            delete GlbVar.fader;
            delete GlbVar.dimMgr;
            delete GlbVar.console;
            delete GlbVar.keyMap;

            //NGF.
            Py_Finalize();
            delete NGF::Python::PythonManager::getSingletonPtr();
            delete GlbVar.lvlLoader;
            delete GlbVar.woMgr;
            delete GlbVar.goMgr;

            //Sound.
            delete GlbVar.soundMgr;

            //Physics.
            delete GlbVar.phyDebug;
            delete GlbVar.phyWorld;
            delete mSolver;
            delete mDispatcher;
            delete mCollisionConfig;
            delete mBroadphase;

            //Listeners.
            delete mMaterialListener;
            delete mGameListener;
            delete mWindowListener;

            //Input.
            mInputManager->destroyInputObject(GlbVar.mouse);
            mInputManager->destroyInputObject(GlbVar.keyboard);
            OIS::InputManager::destroyInputSystem(mInputManager);

            //Graphics.
            delete GlbVar.ogreRoot;

            //Globals.
            delete Globals::getSingletonPtr();
        }
};

//--------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    Game game;

    try
    {
        //Init.
        if(!(game.init(argv[0])))
            return 0;   
        Ogre::LogManager::getSingleton().logMessage("*********** Systems Intialised *************\n");

        if (argc > 1)
            game.start(argv[1]); //Load NGF script given on command line
        else
            game.start();

        //Loop.
        Ogre::LogManager::getSingleton().logMessage("*********** Entering Mainloop **************\n");
        game.loop();
        Ogre::LogManager::getSingleton().logMessage("*********** Mainloop Exited ****************\n");

        //Shutdown.
        Ogre::LogManager::getSingleton().logMessage("*********** Shutting Down Systems **********\n");
        game.shutdown();
    }
    catch(OIS::Exception &e)
    {
        Ogre::StringUtil::StrStreamType desc;

        desc << 
            "An exception has been thrown!\n"
            "\n"
            "-----------------------------------\n"
            "Details:\n"
            "-----------------------------------\n"
            "Error #: " << e.eType
            << "\nDescription: " << e.eText 
            << ". "
            << "\nFile: " << e.eFile
            << "\nLine: " << e.eLine;

        std::cerr << desc.str();
        Ogre::LogManager::getSingleton().logMessage(desc.str());
    }
    catch (std::exception &e)
    {
        Ogre::StringUtil::StrStreamType desc;

        desc << 
            "An exception has been thrown!\n"
            "\n"
            "-----------------------------------\n"
            "Details:\n"
            "-----------------------------------\n"
            << "\nDescription: " << e.what();

        std::cerr << desc.str();
        Ogre::LogManager::getSingleton().logMessage(desc.str());
    }
    catch (py::error_already_set)
    {
        PyErr_Print();
    }
    catch (...)
    {
        std::cerr << "Uknown exception thrown!";
    }

    return 0;
}
