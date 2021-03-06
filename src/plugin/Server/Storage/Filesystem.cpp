/***************************************************************************|
|  OMM - Open Multimedia                                                    |
|                                                                           |
|  Copyright (C) 2009, 2010                                                 |
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
#include <Poco/ClassLibrary.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/NumberFormatter.h>

#include <Omm/UpnpAvObject.h>
#include <Omm/AvStream.h>
#include <Omm/Util.h>
#include <Poco/NumberParser.h>

#include "Filesystem.h"


FileModel::FileModel() :
_pTagger(0)
{
    loadTagger();
}


FileModel::~FileModel()
{
    delete _pTagger;
}


std::string
FileModel::getModelClass()
{
    return "FileModel";
}


Poco::UInt64
FileModel::getSystemUpdateId(bool checkMod)
{
    Poco::File baseDir(getBasePath());
    if (!checkMod && _directories.size()) {
        return checkDirectories();
    }
    else {
        return getUpdateId(baseDir, checkMod);
    }
}


Poco::UInt64
FileModel::getUpdateId(const std::string& path)
{
    Poco::File file(Poco::Path(getBasePath(), path));
    return file.getLastModified().epochTime();
}


Omm::Av::CsvList
FileModel::getQueryProperties()
{
    return Omm::Av::CsvList(Omm::Av::AvProperty::ARTIST, Omm::Av::AvProperty::ALBUM, Omm::Av::AvProperty::ORIGINAL_TRACK_NUMBER);
}


void
FileModel::scan()
{
    LOGNS(Omm::Av, upnpav, debug, "scanning " + getBasePath() + " ...");
    Poco::File baseDir(getBasePath());
    scanDirectory(baseDir);
    LOGNS(Omm::Av, upnpav, debug, "scanning " + getBasePath() + " finished.");
}


std::string
FileModel::getParentPath(const std::string& path)
{
    std::string::size_type pos = path.rfind("/");
    if (pos == std::string::npos) {
        LOGNS(Omm::Av, upnpav, debug, "file data model path: " + path + ", parent path: \"\"");
        return "";
    }
    else {
        LOGNS(Omm::Av, upnpav, debug, "file data model path: " + path + ", parent path: " + path.substr(0, pos));
        return path.substr(0, pos);
    }
}


Omm::Av::ServerObject*
FileModel::getMediaObject(const std::string& path)
{
    Poco::Path fullPath(getBasePath(), path);
    if (Poco::File(fullPath).isDirectory()) {
        LOGNS(Omm::Av, upnpav, debug, "file data model creating container for: " + fullPath.toString());
        Omm::Av::ServerContainer* pContainer = getServerContainer()->createMediaContainer();
        pContainer->setTitle(fullPath.getFileName());
        return pContainer;
    }
    LOGNS(Omm::Av, upnpav, debug, "file data model tagging: " + fullPath.toString());
    Omm::AvStream::Meta* pMeta = _pTagger->tag(fullPath.toString());
    if (pMeta) {
        Omm::Av::ServerItem* pItem = getServerContainer()->createMediaItem();

        setClass(pItem, pMeta->getContainerFormat());

        std::string title = pMeta->getTag(Omm::AvStream::Meta::TK_TITLE);
        if (title == "") {
            title = fullPath.getFileName();
        }
        pItem->setTitle(title);
        pItem->setUniqueProperty(Omm::Av::AvProperty::ARTIST, pMeta->getTag(Omm::AvStream::Meta::TK_ARTIST));
        pItem->setUniqueProperty(Omm::Av::AvProperty::ALBUM, pMeta->getTag(Omm::AvStream::Meta::TK_ALBUM));
        pItem->setUniqueProperty(Omm::Av::AvProperty::ORIGINAL_TRACK_NUMBER, pMeta->getTag(Omm::AvStream::Meta::TK_TRACK));
        pItem->setUniqueProperty(Omm::Av::AvProperty::GENRE, pMeta->getTag(Omm::AvStream::Meta::TK_GENRE));

        Omm::Av::ServerObjectResource* pResource = pItem->createResource();
        pResource->setSize(getSize(path));
        try {
            Omm::r8 duration = Poco::NumberParser::parse64(pMeta->getTag(Omm::AvStream::Meta::TK_DURATION)) / 1000.0;
            pResource->setAttribute(Omm::Av::AvProperty::DURATION, Omm::Av::AvTypeConverter::writeDuration(duration));
        }
        catch (Poco::Exception& e) {
            LOGNS(Omm::Av, upnpav, warning, "file data model could not get duration: " + e.displayText());
        }
        // FIXME: add some parts of protinfo in server container / media server.
        pResource->setProtInfo("http-get:*:" + pMeta->getMime() + ":*");
        pItem->addResource(pResource);
        // TODO: add icon property
//        pItem->_icon = pItem->_path;

        delete pMeta;
        return pItem;
    }
    return 0;
}


std::streamsize
FileModel::getSize(const std::string& path)
{
    std::streamsize res;
    Poco::Path fullPath(getBasePath(), path);
    try {
        res = Poco::File(fullPath).getSize();
    }
    catch (Poco::Exception& e) {
        LOGNS(Omm::Av, upnpav, error, "could not get size of file: " + fullPath.toString());
        res = 0;
    }
    return res;
}


bool
FileModel::isSeekable(const std::string& path, const std::string& resourcePath)
{
    return true;
}


std::istream*
FileModel::getStream(const std::string& path, const std::string& resourcePath)
{
    Poco::Path fullPath(getBasePath(), path);
    std::istream* pRes = new std::ifstream(fullPath.toString().c_str());
    if (!*pRes) {
        LOGNS(Omm::Av, upnpav, error, "could not open file for streaming: " + fullPath.toString());
        return 0;
    }
    return pRes;
}


void
FileModel::freeStream(std::istream* pIstream)
{
    LOGNS(Omm::Av, upnpav, debug, "deleting file stream");
    delete pIstream;
}


void
FileModel::setClass(Omm::Av::ServerItem* pItem, Omm::AvStream::Meta::ContainerFormat format)
{
    switch (format) {
        case Omm::AvStream::Meta::CF_UNKNOWN:
            pItem->setClass(Omm::Av::AvClass::OBJECT);
            break;
        case Omm::AvStream::Meta::CF_AUDIO:
            pItem->setClass(Omm::Av::AvClass::className(Omm::Av::AvClass::ITEM, Omm::Av::AvClass::AUDIO_ITEM));
            break;
        case Omm::AvStream::Meta::CF_VIDEO:
            pItem->setClass(Omm::Av::AvClass::className(Omm::Av::AvClass::ITEM, Omm::Av::AvClass::VIDEO_ITEM));
            break;
        case Omm::AvStream::Meta::CF_IMAGE:
            pItem->setClass(Omm::Av::AvClass::className(Omm::Av::AvClass::ITEM, Omm::Av::AvClass::IMAGE_ITEM));
            break;
        case Omm::AvStream::Meta::CF_PLAYLIST:
            pItem->setClass(Omm::Av::AvClass::className(Omm::Av::AvClass::ITEM, Omm::Av::AvClass::PLAYLIST_ITEM));
            break;
    }
}


Omm::ui4
FileModel::getUpdateId(Poco::File& directory, bool checkMod)
{
    Omm::ui4 res = directory.getLastModified().epochTime();
    _directories[directory.path()] = res;

    Poco::DirectoryIterator dir(directory);
    Poco::DirectoryIterator end;
    while(dir != end) {
        try {
            if (dir->isDirectory()) {
                res = std::max(res, getUpdateId(*dir, checkMod));
            }
            else if (checkMod) {
                res = std::max(res, (Omm::ui4)(dir->getLastModified().epochTime()));
            }
        }
        catch(...) {
            LOGNS(Omm::Av, upnpav, warning, dir->path() + " not found while scanning directory, ignoring.");
        }
        ++dir;
    }
    return res;
}


Omm::ui4
FileModel::checkDirectories()
{
    std::time_t res = 0;
    bool dirty = false;
    for (std::map<std::string, std::time_t>::iterator it = _directories.begin(); it != _directories.end(); ++it) {
        std::time_t mod = Poco::File(it->first).getLastModified().epochTime();
        if (mod != it->second) {
            mod = it->second;
            dirty = true;
        }
        res = std::max(res, mod);
    }
    if (dirty) {
        _directories.clear();
    }
    return res;
}


void
FileModel::scanDirectory(Poco::File& directory)
{
    Poco::DirectoryIterator dir(directory);
    Poco::DirectoryIterator end;
    while(dir != end) {
        try {
            addPath(dir->path().substr(getBasePath().length()));
            if (dir->isDirectory()) {
                scanDirectory(*dir);
            }
        }
        catch(...) {
            LOGNS(Omm::Av, upnpav, warning, dir->path() + " not found while scanning directory, ignoring.");
        }
        ++dir;
    }
}


void
FileModel::loadTagger()
{
    std::string taggerPlugin("tagger-ffmpeg");
    Omm::Util::PluginLoader<Omm::AvStream::Tagger> taggerPluginLoader;
    try {
        _pTagger = taggerPluginLoader.load(taggerPlugin, "Tagger", "FFmpeg");
    }
    catch(Poco::NotFoundException) {
        LOGNS(Omm::Av, upnpav, warning, "could not find avstream tagger plugin: " + taggerPlugin + " loading vlc tagger ...");
        try {
            _pTagger = taggerPluginLoader.load("tagger-vlc");
        }
        catch(Poco::NotFoundException) {
            LOGNS(Omm::Av, upnpav, warning, "could not find avstream tagger plugin: tagger-vlc loading simple tagger ...");
            _pTagger = taggerPluginLoader.load("tagger-simple");
        }
    }
}


#ifdef OMMPLUGIN
POCO_BEGIN_MANIFEST(Omm::Av::AbstractDataModel)
POCO_EXPORT_CLASS(FileModel)
POCO_END_MANIFEST
#endif
