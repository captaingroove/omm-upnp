/***************************************************************************|
|  OMM - Open Multimedia                                                    |
|                                                                           |
|  Copyright (C) 2011                                                       |
|  Jörg Bakker (jb'at'open-multimedia.org)                                  |
|                                                                           |
|  This file is part of OMM.                                                |
|                                                                           |
|  OMM is free software: you can redistribute it and/or modify              |
|  it under the terms of the GNU General Public License as published by     |
|  the Free Software Foundation version 3 of the License.                   |
|                                                                           |
|  OMM is distributed in the hope that it will be useful,                   |
|  but WITHOUT ANY WARRANTY; without even the implied warranty of           |
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            |
|  GNU General Public License for more details.                             |
|                                                                           |
|  You should have received a copy of the GNU General Public License        |
|  along with this program.  If not, see <http://www.gnu.org/licenses/>.    |
 ***************************************************************************/

#include <iostream>

#include <Poco/NotificationCenter.h>
#include <Poco/NObserver.h>
#include <Poco/UUIDGenerator.h>
#include <Poco/StreamCopier.h>
#include <Poco/NamedMutex.h>
#include <Poco/Util/Application.h>
#include <Poco/NumberFormatter.h>


#include "Util.h"
#include "UpnpAv.h"
#include "UpnpAvCtlServer.h"

#include "Gui/GuiLogger.h"
#include "Gui/View.h"
#include "Gui/Controller.h"

#include "UpnpGui/ActivityIndicator.h"
#include "UpnpGui/DeviceGroup.h"
#include "UpnpGui/GuiVisual.h"
#include "UpnpGui/MediaObject.h"
#include "UpnpGui/MediaRenderer.h"
#include "UpnpGui/MediaServer.h"
#include "UpnpGui/Playlist.h"
#include "UpnpGui/GuiSetup.h"
#include "UpnpGui/GuiUpnpApplication.h"
#include "UpnpGui/ControllerWidget.h"

#include "MediaImages.h"


namespace Omm {


class MediaServerGroupController : public Omm::Gui::NavigatorController
{
    friend class ControllerWidget;

    MediaServerGroupController(ControllerWidget* pController) : _pController(pController) {}

    virtual void poppedToRoot()
    {
        LOGNS(Gui, gui, debug, "media server group popped to root");
        if (_pController->_pPlaylistEditorView && _pController->_pPlaylistEditorView->isVisible()) {
            _pController->_pPlaylistEditorView->hide(false);
            _pController->_pMediaServerGroupWidget->finishEditPlaylist();
        }
        if (_pController->_pMediaServerGroupWidget) {
            _pController->_pMediaServerGroupWidget->showSearchBox(false);
            _pController->_pMediaServerGroupWidget->clearSearchText();
        }
    }

    virtual void poppedToView(Gui::View* pView)
    {
        LOGNS(Gui, gui, debug, "media server group popped to view: " + (pView ? pView->getName() : ""));
        if (_pController->_pPlaylistEditorView && !_pController->_pPlaylistEditorView->isVisible()) {
            _pController->_pMediaServerGroupWidget->showRightButton(false);
        }
        _pController->_pMediaServerGroupWidget->showSearchBox(true);
    }

    ControllerWidget* _pController;
};


class ControllerWidgetClusterController : public Omm::Gui::ClusterController
{
    friend class ControllerWidget;

    ControllerWidgetClusterController(ControllerWidget* pControllerWidget) : _pControllerWidget(pControllerWidget) {}

    virtual void movedView(Omm::Gui::View* pView)
    {
        LOGNS(Gui, gui, debug, "controller widget moved view: " + pView->getName());

        GuiVisual* pVisual = dynamic_cast<GuiVisual*>(pView);
        if (pVisual && _pControllerWidget->getLocalRenderer() && _pControllerWidget->getLocalRenderer()->getEngine()) {
            _pControllerWidget->getLocalRenderer()->getEngine()->setVisual(pVisual);
        }
    }

    ControllerWidget* _pControllerWidget;
};


ControllerWidget::ControllerWidget(GuiUpnpApplication* pApplication) :
//ClusterView(0, Gui::ClusterView::Generic),
ClusterView(0, Gui::ClusterView::Column),
_pApplication(pApplication),
_featureShowNetworkActivity(true)
{
    LOGNS(Gui, gui, debug, "controller widget ctor ...");

    if (!_pApplication->getIgnoreConfigFile() && !Poco::Util::Application::instance().config().getBool("application.fullscreen", false)) {
        LOGNS(Gui, gui, debug, "controller widget create setup view ...");
        _pSetup = new GuiSetup(_pApplication);
        _pSetup->setName("Setup");
        insertView(_pSetup, "Setup");
    }

    if (!_pApplication->getIgnoreConfigFile()) {
        LOGNS(Gui, gui, debug, "controller widget create visual ...");
        _pVisual = new GuiVisual;
        insertView(_pVisual, "Video");
    }

    LOGNS(Gui, gui, debug, "controller widget create renderer device group ...");
    _pMediaRendererGroupWidget = new MediaRendererGroupWidget(this);
    registerDeviceGroup(_pMediaRendererGroupWidget);
    if (!Poco::Util::Application::instance().config().getBool("application.fullscreen", false)) {
        insertView(_pMediaRendererGroupWidget, "Player");
    }

    LOGNS(Gui, gui, debug, "controller widget create server browser ...");
    Omm::Gui::SplitterView* pSplitterView = new Omm::Gui::SplitterView(0, Omm::Gui::View::Vertical);
    pSplitterView->setName("Media");
    _pMediaServerGroupWidget = new MediaServerGroupWidget(this);
    _pMediaServerGroupWidget->attachController(new MediaServerGroupController(this));
    registerDeviceGroup(_pMediaServerGroupWidget);
    pSplitterView->insertView(_pMediaServerGroupWidget, 0);

    _pPlaylistEditor = new PlaylistEditor(this);
    _pPlaylistEditorView = new PlaylistEditorView(_pPlaylistEditor);
    _pPlaylistEditorView->hide();
    pSplitterView->insertView(_pPlaylistEditorView, 1);
    Poco::NotificationCenter::defaultCenter().addObserver(Poco::Observer<ControllerWidget,
    PlaylistNotification>(*this, &ControllerWidget::playlistNotification));

    insertView(pSplitterView, "Media");
    std::vector<float> splitterSizes;
    splitterSizes.push_back(0.8);
    splitterSizes.push_back(0.2);
    pSplitterView->setSizes(splitterSizes);

#ifdef __IPHONE__
    _pControlPanel = new MediaRendererView(true);
#else
    _pControlPanel = new MediaRendererView(false);
#endif
    _pActivityIndicator = new ActivityIndicator;
    setCurrentViewIndex(getIndexFromView(_pMediaServerGroupWidget));
    setLayoutConfiguration(Poco::Util::Application::instance().config().getBool("application.fullscreen", false));

//    Poco::NotificationCenter::defaultCenter().addObserver(Poco::Observer<ControllerWidget, TransportStateNotification>(*this, &ControllerWidget::newTransportState));
//    Poco::NotificationCenter::defaultCenter().addObserver(Poco::Observer<ControllerWidget, TrackNotification>(*this, &ControllerWidget::newTrack));
//    Poco::NotificationCenter::defaultCenter().addObserver(Poco::Observer<ControllerWidget, PlaylistNotification>(*this, &ControllerWidget::newPlaylist));
    attachController(new KeyController(this));
    attachController(new ControllerWidgetClusterController(this));

    LOGNS(Gui, gui, debug, "controller widget ctor finished.");
}


void
ControllerWidget::setState(State newState)
{
    LOGNS(Gui, gui, debug, "controller widget state change: " + _state + " -> " + newState);
    if (_state == newState) {
        LOG(upnp, debug, "new state equal to old state, ignoring");
        return;
    }

    Controller::setState(newState);

    if (newState == Stopped) {

        // don't stop selected renderer, it should continue playing though the controller is stopped
//        MediaRendererDevice* pRenderer = static_cast<MediaRendererDevice*>(_pMediaRendererGroupWidget->getSelectedDevice());
//        if (pRenderer) {
//            pRenderer->stopPressed();
//        }
        // stop position info timer for all renderer devices that controller knows of (renderers continue to play)
        for (int r = 0; r < _pMediaRendererGroupWidget->getDeviceCount(); r++) {
            MediaRendererDevice* pRenderer = static_cast<MediaRendererDevice*>(_pMediaRendererGroupWidget->getDevice(r));
            // TODO: deactivated position timer for now, should fix it.
            pRenderer->startPositionTimer(false);
        }
        // clear device views and remove controller's device containers
        _pMediaServerGroupWidget->popToRoot();
        _pMediaServerGroupWidget->clearDevices();
//        _pMediaServerGroupWidget->getVisibleView()->syncView();
        _pMediaServerGroupWidget->getDeviceGroupView().syncView();
        _pMediaRendererGroupWidget->clearDevices();
//        _pMediaRendererGroupWidget->getVisibleView()->syncView();
        _pMediaRendererGroupWidget->getDeviceGroupView().syncView();
    }
    LOGNS(Gui, gui, debug, "controller widget state change finished");
}


Av::MediaRenderer*
ControllerWidget::getLocalRenderer()
{
    return _pApplication->getLocalRenderer();
}


GuiVisual*
ControllerWidget::getLocalRendererVisual()
{
    return _pVisual;
}


Gui::View*
ControllerWidget::getControlPanel()
{
//    View* pView = new View;
//    pView->setBackgroundColor(Gui::Color("blue"));
//    pView->resize(100, 20);
//    return pView;
//    _pControlPanel->resize(800, 80);
    return _pControlPanel;
}


Gui::View*
ControllerWidget::getStatusBar()
{
    return _pActivityIndicator;
}


void
ControllerWidget::setDefaultRenderer(Omm::Av::MediaRenderer* pRenderer)
{
    _localRendererUuid = pRenderer->getUuid();
    _pMediaRendererGroupWidget->setDefaultDevice(pRenderer);
}


void
ControllerWidget::newStreamType(const Poco::AutoPtr<Av::StreamTypeNotification>& pNotification)
{
    LOGNS(Gui, gui, debug, "controller widget stream type notification, instance id: "
        + Poco::NumberFormatter::format(pNotification->_instanceId) + ", transport state: " + pNotification->_transportState + ", stream type: " + pNotification->_streamType);
    if (pNotification->_streamType == Av::Engine::StreamTypeVideo && pNotification->_transportState == Av::AvTransportArgument::TRANSPORT_STATE_PLAYING) {
        setRendererVisualVisible(true);
        setCurrentViewIndex(getIndexFromView(_pVisual));
    }
    else if (pNotification->_transportState == Av::AvTransportArgument::TRANSPORT_STATE_STOPPED) {
        setRendererVisualVisible(false);
        showMainMenu();
    }
}


//void
//ControllerWidget::newTransportState(TransportStateNotification* pNotification)
//{
//    LOGNS(Gui, gui, debug, "controller widget device: " + pNotification->_uuid + " got new transport state: " + pNotification->_transportState);
//    LOGNS(Gui, gui, debug, "new transport state, local renderer has uuid: " + _localRendererUuid);
//    if (pNotification->_uuid == _localRendererUuid && pNotification->_transportState == Av::AvTransportArgument::TRANSPORT_STATE_STOPPED) {
//        showMainMenu();
//    }
//}


//void
//ControllerWidget::newTrack(TrackNotification* pNotification)
//{
//    if (pNotification->_uuid == _localRendererUuid) {
//        LOGNS(Gui, gui, debug, "new track on local renderer");
//        Av::CtlMediaRenderer* pRenderer = static_cast<Av::CtlMediaRenderer*>(_pMediaRendererGroupWidget->getDevice(pNotification->_uuid));
//        if (pRenderer) {
//            LOGNS(Gui, gui, debug, "new track on local renderer, playing object: " + pNotification->_title + ", class: " + pNotification->_objectClass);
//            if (Av::AvClass::matchClass(pNotification->_objectClass, Av::AvClass::ITEM, Av::AvClass::VIDEO_ITEM)
//                || Av::AvClass::matchClass(pNotification->_objectClass, Av::AvClass::ITEM, Av::AvClass::VIDEO_BROADCAST)
//                || Av::AvClass::matchClass(pNotification->_objectClass, Av::AvClass::CONTAINER, Av::AvClass::PLAYLIST_CONTAINER)) {
//                setCurrentView(_pVisual);
//            }
//        }
//    }
//}


//void
//ControllerWidget::newPlaylist(PlaylistNotification* pNotification)
//{
//    // write resource of playlist container
//    // TODO: move this to playlist editor, assign a playlist container to playlist editor.
////    pModel->writeResource(getControllerHttpUri());
//}


void
ControllerWidget::showMainMenu()
{
    setCurrentViewIndex(getIndexFromView(_pMediaServerGroupWidget));
}


//void
//ControllerWidget::showOnlyBasicDeviceGroups(bool show)
//{
//    insertView(_pMediaRendererGroupWidget, "Player");
//    insertView(_pPlaylistEditor, "List");
//    insertView(_pConfigBrowser, "Setup");
//}


void
ControllerWidget::setRendererVisualVisible(bool show)
{
    if (show) {
        showViewAtIndex(_pVisual, 1);
    }
    else {
        hideView(_pVisual);
    }
}


void
ControllerWidget::navigateListWithKey(Gui::Controller::KeyCode key)
{
    LOGNS(Gui, gui, debug, "navigate list with key: " + Poco::NumberFormatter::format(key));
    Gui::ListView* pListView = dynamic_cast<Gui::ListView*>(_pMediaServerGroupWidget->getVisibleView());
    if (pListView) {
        LOGNS(Gui, gui, debug, "triggerKeyEvent: " + Poco::NumberFormatter::format(key) + ", on view: " + _pMediaServerGroupWidget->getVisibleView()->getName());
//        _pMediaServerGroupWidget->getVisibleView()->triggerKeyEvent(key, Gui::Controller::NoModifier, false);
        switch (key) {
            case Gui::Controller::KeyUp:
                pListView->highlightUp();
                break;
            case Gui::Controller::KeyDown:
                pListView->highlightDown();
                break;
            case Gui::Controller::KeyReturn:
                pListView->activateHighlighted();
                break;
        }
    }
}


void
ControllerWidget::back()
{
    _pMediaServerGroupWidget->pop();
}


void
ControllerWidget::signalNetworkActivity(bool on)
{
    if (_featureShowNetworkActivity) {
        on ? _pActivityIndicator->startActivity() : _pActivityIndicator->stopActivity();
    }
}


std::stringstream*
ControllerWidget::getPlaylistResource()
{
    return _pPlaylistEditor->getPlaylistResource();
}


MediaServerDevice*
ControllerWidget::getServer(const std::string& uuid)
{
    return static_cast<MediaServerDevice*>(_pMediaServerGroupWidget->getDevice(uuid));
}


Av::CtlMediaRenderer*
ControllerWidget::getSelectedRenderer()
{
    return static_cast<Av::CtlMediaRenderer*>(_pMediaRendererGroupWidget->getSelectedDevice());
}


void
ControllerWidget::showPlaylistEditor(bool show)
{
    if (show) {
        _pPlaylistEditorView->show();
    }
    else {
        _pPlaylistEditorView->hide();
    }
}


void
ControllerWidget::setFeatureShowNetworkActivity(bool on)
{
    _featureShowNetworkActivity = on;
}


void
ControllerWidget::setLayoutConfiguration(bool fullscreen)
{
//    setConfiguration("col [0,0] Media,Video {800;480} {" + Poco::NumberFormatter::format(1.00) + "} {" + Poco::NumberFormatter::format(1.00) + "}");

    if (fullscreen) {
        // simple fullscreen configuration with media browser and video view, only
        setConfiguration("col [0,0] Media,Video {800;480} {" + Poco::NumberFormatter::format(1.00) + "} {" + Poco::NumberFormatter::format(1.00) + "}");
    }
    else {
        // controller only, without local renderer and video view
        if (_pApplication->getIgnoreConfigFile()) {
            setConfiguration("col [0,0] Media [1,0] Player {800;480} {" +\
                    Poco::NumberFormatter::format(0.5) + ";" + Poco::NumberFormatter::format(0.5) + "} {" + Poco::NumberFormatter::format(1.0) + "} {" +\
                    Poco::NumberFormatter::format(1.0) + "}");
        }
        else {
            // omm application with full configuration
            setConfiguration(Poco::Util::Application::instance().config().getString("application.cluster", "col [0,0] Media [1,0] Player [1,1] Setup,Video {800;480} {" +\
                    Poco::NumberFormatter::format(0.56) + ";" + Poco::NumberFormatter::format(0.43) + "} {" + Poco::NumberFormatter::format(1.00) + "} {" +\
                    Poco::NumberFormatter::format(0.50) + ";" + Poco::NumberFormatter::format(0.50) + "}"));
        }
    }
}


void
ControllerWidget::playlistNotification(PlaylistNotification* pNotification)
{
    MediaObjectModel* pModel = pNotification->_pMediaObject;
    if (pModel->isContainer()) {
        if (Av::AvClass::matchClass(pModel->getClass(), Av::AvClass::CONTAINER, Av::AvClass::PLAYLIST_CONTAINER)) {
//            _pPlaylistEditorView->show();
            _pMediaServerGroupWidget->showRightButton();
            _pMediaServerGroupWidget->showSearchBox(false);
//            if (pModel->getResource() && pModel->getResource()->getAttributeValue(Av::AvProperty::IMPORT_URI) != "") {
//                LOGNS(Gui, gui, debug, "playlist editor load playlist: " + pModel->getTitle());
//                setPlaylistContainer(pModel);
//                Av::AbstractMediaObject* pObject = pModel->getChildForRow(0);
//                LOGNS(Gui, gui, debug, "media object playlist button pushed, container with count children: " + Poco::NumberFormatter::format(pModel->getChildCount()));
//                for (int r = 0; r < pModel->getChildCount(); r++) {
//                    LOGNS(Gui, gui, debug, "title: " + pModel->getChildForRow(r)->getTitle());
//                    _playlistItems.push_back(new MediaObjectModel(*static_cast<MediaObjectModel*>(pModel->getChildForRow(r))));
//                }
//            }
        }
    }
//    else if (_pPlaylistContainer) {
//        LOGNS(Gui, gui, debug, "media object playlist add item with title: " + pModel->getTitle());
//        _playlistItems.push_back(new MediaObjectModel(*pModel));
//        writePlaylistResource();
//
//        // FIXME: why does this crash?
////        _pPlaylistContainer->writeResource(getPlaylistResourceUri());
//    }
//    syncViewImpl();
}


void
KeyController::keyPressed(KeyCode key, Modifiers mod, bool& propagate)
{
    LOGNS(Gui, gui, debug, "key controller, key pressed: " + Poco::NumberFormatter::format(key) + ", modifiers: 0x" + Poco::NumberFormatter::formatHex(mod));
    Av::CtlMediaRenderer* pRenderer = _pControllerWidget->getSelectedRenderer();
    bool fullscreen = false;

    if (mod == Gui::Controller::NoModifier) {
        switch (key) {
            case Gui::Controller::KeyMenu:
                _pControllerWidget->showMainMenu();
                break;
            case Gui::Controller::KeyLeft:
                if (_pControllerWidget->getCurrentViewIndex()) {
                    _pControllerWidget->setCurrentViewIndex(_pControllerWidget->getCurrentViewIndex() - 1);
                }
                break;
            case Gui::Controller::KeyRight:
                if (_pControllerWidget->getCurrentViewIndex() < _pControllerWidget->getViewCount() - 1) {
                    _pControllerWidget->setCurrentViewIndex(_pControllerWidget->getCurrentViewIndex() + 1);
                }
                break;
            case Gui::Controller::KeyUp:
            case Gui::Controller::KeyDown:
            case Gui::Controller::KeyReturn:
                _pControllerWidget->navigateListWithKey(key);
                break;
            case Gui::Controller::KeyBack:
                _pControllerWidget->back();
                break;
            case Gui::Controller::KeyPlay:
                if (pRenderer) {
                    pRenderer->playPressed();
                }
                break;
            case Gui::Controller::KeyStop:
                if (pRenderer) {
                    pRenderer->stopPressed();
                }
                break;
            case Gui::Controller::KeyMute:
                if (pRenderer) {
                    pRenderer->setMute(!pRenderer->getMute());
                }
                break;
            case Gui::Controller::KeyVolUp:
                if (pRenderer) {
                    ui2 vol = pRenderer->getVolume();
                    if (vol + _volStep <= 100) {
                        pRenderer->volumeChanged(pRenderer->getVolume() + _volStep);
                    }
                }
                break;
            case Gui::Controller::KeyVolDown:
                if (pRenderer) {
                    ui2 vol = pRenderer->getVolume();
                    if (vol - _volStep >= 0) {
                        pRenderer->volumeChanged(vol - _volStep);
                    }
                }
                break;
        }
    }

    if (mod == Gui::Controller::ControlModifier) {
        switch (key) {
            case Gui::Controller::KeyQ:
                propagate = false;
                _pControllerWidget->_pApplication->quit();
                break;
            case Gui::Controller::KeyS:
                propagate = false;
                if (_pControllerWidget->getState() == DeviceManager::Stopped) {
                    _pControllerWidget->_pApplication->start();
                }
                else {
                    _pControllerWidget->_pApplication->stop();
                }
                break;
            case Gui::Controller::KeyF:
                fullscreen = !Poco::Util::Application::instance().config().getBool("application.fullscreen", false);
                _pControllerWidget->_pApplication->setFullscreen(fullscreen);
                Poco::Util::Application::instance().config().setBool("application.fullscreen", fullscreen);
                _pControllerWidget->_pApplication->getFileConfiguration()->setBool("application.fullscreen", fullscreen);
                break;
            case Gui::Controller::KeyM:
                _pControllerWidget->showMainMenu();
                break;
//            case Gui::Controller::KeyP:
//                if (pRenderer) {
//                    pRenderer->playPressed();
//                }
//                break;
//            case Gui::Controller::KeyS:
//            case Gui::Controller::KeyStop:
//                if (pRenderer) {
//                    pRenderer->stopPressed();
//                }
//                break;
            case Gui::Controller::KeyH:
                if (pRenderer) {
                    pRenderer->setMute(!pRenderer->getMute());
                }
                break;
            case Gui::Controller::KeyU:
                if (pRenderer) {
                    ui2 vol = pRenderer->getVolume();
                    if (vol + _volStep <= 100) {
                        pRenderer->volumeChanged(pRenderer->getVolume() + _volStep);
                    }
                }
                break;
            case Gui::Controller::KeyD:
                if (pRenderer) {
                    ui2 vol = pRenderer->getVolume();
                    if (vol - _volStep >= 0) {
                        pRenderer->volumeChanged(vol - _volStep);
                    }
                }
                break;
        }
    }

    if (mod == Gui::Controller::ShiftModifier) {
        switch (key) {
            case Gui::Controller::KeyP:
                propagate = false;
                if (pRenderer) {
                    pRenderer->playPressed();
                }
                break;
            case Gui::Controller::KeyS:
                propagate = false;
                if (pRenderer) {
                    pRenderer->stopPressed();
                }
                break;
        }
    }
}


} // namespace Omm
