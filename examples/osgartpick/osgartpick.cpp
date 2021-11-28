/* -*-c++-*- 
 * 
 * osgART - ARToolKit for OpenSceneGraph
 * Copyright (C) 2005-2008 Human Interface Technology Laboratory New Zealand
 * 
 * This file is part of osgART 2.0
 *
 * osgART 2.0 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * osgART 2.0 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with osgART 2.0.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// A simple example to demonstrate picking with osgART

#include <osgART/Foundation>
#include <osgART/VideoLayer>
#include <osgART/PluginManager>
#include <osgART/VideoGeode>
#include <osgART/Utils>
#include <osgART/GeometryUtils>
#include <osgART/MarkerCallback>
#include <osgART/TransformFilterCallback>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgGA/GUIEventHandler>
#include <osgGA/GUIEventAdapter>

osg::Group* createImageBackground(osg::Image* video) {
	osgART::VideoLayer* _layer = new osgART::VideoLayer();
	_layer->setSize(*video);
	osgART::VideoGeode* _geode = new osgART::VideoGeode(osgART::VideoGeode::USE_TEXTURE_2D, video);
	addTexturedQuad(*_geode,video->s(),video->t());
	_layer->addChild(_geode);
	return _layer;
}

class PickEventHandler : public osgGA::GUIEventHandler {

public:
	PickEventHandler() : osgGA::GUIEventHandler() { }                                                       

	virtual bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa, osg::Object* obj, osg::NodeVisitor* nv) { 

		switch (ea.getEventType()) {

			case osgGA::GUIEventAdapter::PUSH:

				osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);
				osgUtil::LineSegmentIntersector::Intersections intersections;
				
				if (view && view->computeIntersections(ea.getX(), ea.getY(), intersections)) {				
					for (osgUtil::LineSegmentIntersector::Intersections::iterator iter = intersections.begin(); iter != intersections.end(); iter++) {							
						if (iter->nodePath.back()->getName() == "target") {
							std::cout << "HIT!" << std::endl;	
							return true;
						}
					}
				}
			
				break;
		}
		return false;
	}
};

int main(int argc, char* argv[])  {

	// create a root node
	osg::ref_ptr<osg::Group> root = new osg::Group;

	osgViewer::Viewer viewer;
	viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

	viewer.addEventHandler(new PickEventHandler());
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);
	viewer.addEventHandler(new osgViewer::StatsHandler);

	viewer.setSceneData(root.get());


	// preload the video and tracker
	int _video_id = osgART::PluginManager::instance()->load("osgart_video_artoolkit2");
	int _tracker_id = osgART::PluginManager::instance()->load("osgart_tracker_artoolkit2");

	// Load a video plugin.
	osg::ref_ptr<osgART::Video> video = 
		dynamic_cast<osgART::Video*>(osgART::PluginManager::instance()->get(_video_id));

	// check if an instance of the video stream could be started
	if (!video.valid()) 
	{   
		// Without video an AR application can not work. Quit if none found.
		osg::notify(osg::FATAL) << "Could not initialize video plugin!" << std::endl;
		exit(-1);
	}

	// found video - configure now
	osgART::VideoConfiguration* _config = video->getVideoConfiguration();

	// if the configuration is existing
	if (_config) 
	{
		// it is possible to configure the plugin before opening it

		//_config->deviceconfig = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		//	"<dsvl_input><avi_file use_reference_clock=\"true\" file_name=\"Data\\MyVideo.avi\" loop_avi=\"true\" render_secondary=\"true\">"
		//	"<pixel_format><RGB32/></pixel_format></avi_file></dsvl_input>";
	}
	

	// Open the video. This will not yet start the video stream but will
	// get information about the format of the video which is essential
	// for the connected tracker
	video->open();

	osg::ref_ptr<osgART::Tracker> tracker = 
		dynamic_cast<osgART::Tracker*>(osgART::PluginManager::instance()->get(_tracker_id));

	if (!tracker.valid()) 
	{
		// Without tracker an AR application can not work. Quit if none found.
		osg::notify(osg::FATAL) << "Could not initialize tracker plugin!" << std::endl;
		exit(-1);
	}

	// get the tracker calibration object
	osg::ref_ptr<osgART::Calibration> calibration = tracker->getOrCreateCalibration();

	// load a calibration file
	if (!calibration->load("data/camera_para.dat")) 
	{

		// the calibration file was non-existing or couldnt be loaded
		osg::notify(osg::FATAL) << "Non existing or incompatible calibration file" << std::endl;
		exit(-1);
	}

	tracker->setImage(video.get());

	osgART::TrackerCallback::addOrSet(root.get(), tracker.get());

	osg::ref_ptr<osgART::Marker> marker = tracker->addMarker("single;data/patt.hiro;80;0;0");
	if (!marker.valid()) 
	{
		// Without marker an AR application can not work. Quit if none found.
		osg::notify(osg::FATAL) << "Could not add marker!" << std::endl;
		exit(-1);
	}

	marker->setActive(true);

	osg::ref_ptr<osg::MatrixTransform> arTransform = new osg::MatrixTransform();
	arTransform->setUpdateCallback(new osgART::MarkerTransformCallback(marker.get()));
	arTransform->getUpdateCallback()->setNestedCallback(new osgART::MarkerVisibilityCallback(marker.get()));
	arTransform->getUpdateCallback()->getNestedCallback()->setNestedCallback(new osgART::TransformFilterCallback());
	
	osg::Geode* cube = osgART::testCube();
	cube->setName("target");
	arTransform->addChild(cube);

	arTransform->getOrCreateStateSet()->setRenderBinDetails(100, "RenderBin");

	osg::ref_ptr<osg::Group> videoBackground = createImageBackground(video.get());
	videoBackground->getOrCreateStateSet()->setRenderBinDetails(0, "RenderBin");

	osg::ref_ptr<osg::Camera> cam = calibration->createCamera();

	cam->addChild(arTransform.get());
	cam->addChild(videoBackground.get());
	root->addChild(cam.get());

	video->start();
	return viewer.run();
	
}
