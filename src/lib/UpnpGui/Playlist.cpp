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

#include "UpnpGui/UpnpApplication.h"
#include "UpnpGui/MediaObject.h"
#include "UpnpGui/ControllerWidget.h"
#include "UpnpGui/Playlist.h"

#include "Util.h"
#include "Gui/GuiLogger.h"
#include "UpnpAv.h"
#include "UpnpAvCtlServer.h"

#include "MediaImages.h"



namespace Omm {

PlaylistNotification::PlaylistNotification(MediaObjectModel* pMediaObject) :
_pMediaObject(pMediaObject)
{
}


PlaylistEditor::PlaylistEditor(ControllerWidget* pControllerWidget) :
_pControllerWidget(pControllerWidget),
_pPlaylistContainer(0)
{
    Poco::NotificationCenter::defaultCenter().addObserver(Poco::Observer<PlaylistEditor,
            PlaylistNotification>(*this, &PlaylistEditor::playlistNotification));
    View::setName("List");
    setDragMode(Omm::Gui::ListView::DragSource | Omm::Gui::ListView::DragTarget);
    setModel(this);
    attachController(this);
}


int
PlaylistEditor::totalItemCount()
{
    LOGNS(Gui, gui, debug, "playlist editor total item count: " + Poco::NumberFormatter::format(_playlistItems.size()));
    return _playlistItems.size();
}


Gui::View*
PlaylistEditor::createItemView()
{
//    return new Gui::ListItemView;
    return new PlaylistEditorObjectView(this);
}


Gui::Model*
PlaylistEditor::getItemModel(int row)
{
    LOGNS(Gui, gui, debug, "playlist editor get item model in row: " + Poco::NumberFormatter::format(row));
    if (row >= 0 && row < _playlistItems.size()) {
        LOGNS(Gui, gui, debug, "playlist editor get item with title: " + _playlistItems[row]->getTitle());
        return _playlistItems[row];
    }
    else {
        return 0;
    }
}


void
PlaylistEditor::draggedItem(int row)
{
    _playlistItems.erase(_playlistItems.begin() + row);
    writePlaylistResource();
}


void
PlaylistEditor::droppedItem(Gui::Model* pModel, int row)
{
    _playlistItems.insert(_playlistItems.begin() + row, static_cast<MediaObjectModel*>(pModel));
    writePlaylistResource();
}


void
PlaylistEditor::playlistNotification(PlaylistNotification* pNotification)
{
    MediaObjectModel* pModel = pNotification->_pMediaObject;
    if (pModel->isContainer()) {
        if (Av::AvClass::matchClass(pModel->getClass(), Av::AvClass::CONTAINER, Av::AvClass::PLAYLIST_CONTAINER)) {
            if (pModel->getResource() && pModel->getResource()->getAttributeValue(Av::AvProperty::IMPORT_URI) != "") {
                LOGNS(Gui, gui, debug, "playlist editor load playlist: " + pModel->getTitle());
                setPlaylistContainer(pModel);
                Av::AbstractMediaObject* pObject = pModel->getChildForRow(0);
                LOGNS(Gui, gui, debug, "media object playlist button pushed, container with count children: " + Poco::NumberFormatter::format(pModel->getChildCount()));
                for (int r = 0; r < pModel->getChildCount(); r++) {
                    LOGNS(Gui, gui, debug, "title: " + pModel->getChildForRow(r)->getTitle());
                    _playlistItems.push_back(new MediaObjectModel(*static_cast<MediaObjectModel*>(pModel->getChildForRow(r))));
                }
            }
        }
    }
    else if (_pPlaylistContainer) {
        LOGNS(Gui, gui, debug, "media object playlist add item with title: " + pModel->getTitle());
        _playlistItems.push_back(new MediaObjectModel(*pModel));
        writePlaylistResource();

        // FIXME: why does this crash?
//        _pPlaylistContainer->writeResource(getPlaylistResourceUri());
    }
    syncViewImpl();
}


void
PlaylistEditor::setPlaylistContainer(MediaObjectModel* pPlaylistContainer)
{
    LOGNS(Gui, gui, debug, "set playlist container with title: " + pPlaylistContainer->getTitle());
    _pPlaylistContainer = new MediaObjectModel(*pPlaylistContainer);
}


std::stringstream*
PlaylistEditor::getPlaylistResource()
{
    std::stringstream* pPlaylistResource = new std::stringstream;
    for (std::vector<MediaObjectModel*>::iterator it = _playlistItems.begin(); it != _playlistItems.end(); ++it) {
        *pPlaylistResource << (*it)->getResource()->getUri() << std::endl;
    }
    return pPlaylistResource;
}


std::string
PlaylistEditor::getPlaylistResourceUri()
{
    _pControllerWidget->_pApplication->getAppHttpUri() + UpnpApplication::PLAYLIST_URI;
}


void
PlaylistEditor::writePlaylistResource()
{
    _pPlaylistContainer->writeResource(_pControllerWidget->_pApplication->getAppHttpUri() + UpnpApplication::PLAYLIST_URI);
}


void
PlaylistEditor::deleteItem(MediaObjectModel* pModel)
{
    LOGNS(Gui, gui, debug, "delete media object from playlist with title: " + pModel->getTitle());
    std::vector<MediaObjectModel*>::iterator pos = std::find(_playlistItems.begin(), _playlistItems.end(), pModel);
    if (pos != _playlistItems.end()) {
        _playlistItems.erase(pos);
        writePlaylistResource();
        delete pModel;
        syncViewImpl();
    }
}


PlaylistEditorDeleteObjectController::PlaylistEditorDeleteObjectController(PlaylistEditorObjectView* pPlaylistEditorObjectView) :
_pPlaylistEditorObjectView(pPlaylistEditorObjectView)
{
}


void
PlaylistEditorDeleteObjectController::pushed()
{
    LOGNS(Gui, gui, debug, "playlist editor delete button pushed.");
    MediaObjectModel* pModel = static_cast<MediaObjectModel*>(_pPlaylistEditorObjectView->getModel());
    _pPlaylistEditorObjectView->_pPlaylistEditor->deleteItem(pModel);
}


PlaylistEditorObjectView::PlaylistEditorObjectView(PlaylistEditor* pPlaylistEditor, View* pParent) :
_pPlaylistEditor(pPlaylistEditor)
{
    _pDeleteButton = new Gui::Button(this);
    _pDeleteButton->setLabel("D");
    _pDeleteButton->setBackgroundColor(Gui::Color("blue"));
    _pDeleteButton->setStretchFactor(-1.0);
    _pDeleteButton->resize(20, 15);
    _pDeleteButton->attachController(new PlaylistEditorDeleteObjectController(this));
}

} // namespace Omm
