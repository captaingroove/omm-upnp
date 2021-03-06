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

#include <iostream>
#include <Poco/NumberFormatter.h>

#include <Omm/Gui/Application.h>
#include <Omm/Gui/Button.h>
#include <Omm/Gui/Cluster.h>


class ButtonClusterController : public Omm::Gui::ClusterController
{
    virtual void insertedView(Omm::Gui::View* pView)
    {
        std::cout << "button cluster inserted view: " + pView->getName() << std::endl;
    }

    virtual void movedView(Omm::Gui::View* pView)
    {
        std::cout << "button cluster moved view: " + pView->getName() << std::endl;
    }
};


class Application : public Omm::Gui::Application
{
    virtual Omm::Gui::View* createMainView()
    {
//        Omm::Gui::ClusterView* pCluster = new Omm::Gui::ClusterView(0, Omm::Gui::ClusterView::Native);
//        Omm::Gui::ClusterView* pCluster = new Omm::Gui::ClusterView(0, Omm::Gui::ClusterView::Generic);
        Omm::Gui::ClusterView* pCluster = new Omm::Gui::ClusterView(0, Omm::Gui::ClusterView::Column);
        // FIXME: core dump with default cluster type "" == Column and libommgui-qt
//        Omm::Gui::ClusterView* pCluster = new Omm::Gui::ClusterView;
        pCluster->attachController(new ButtonClusterController);

        const int subviewCount = 5;
        for (int i = 0; i < subviewCount; i++) {
            Omm::Gui::Button* pButton = new Omm::Gui::Button;
            std::string name = "V" + Poco::NumberFormatter::format(i);
            std::string label = "Button " + Poco::NumberFormatter::format(i);
            std::string handle = "B" + Poco::NumberFormatter::format(i);
            pButton->setName(name);
            pButton->setLabel(label);
            pButton->setSizeConstraint(150, 420, Omm::Gui::View::Pref);
            pCluster->insertView(pButton, handle, i);
        }
        std::vector<std::string> arguments;
        getArguments(arguments);
        if (arguments.size() > 1) {
            pCluster->setConfiguration(arguments[1]);
        }
        return pCluster;
    }

    virtual void presentedMainView()
    {
        resizeMainView(300, 120);
    }

    virtual void finishedEventLoop()
    {
        std::cout << "cluster configuration: " << static_cast<Omm::Gui::ClusterView*>(getMainView())->getConfiguration() << std::endl;
    }

};


int main(int argc, char** argv)
{
    Application app;
    return app.runEventLoop(argc, argv);
}

