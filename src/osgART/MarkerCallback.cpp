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


#include "osgART/MarkerCallback"

#include <osg/Switch>
#include <osg/Notify>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>

namespace osgART {


	void addEventCallback(osg::Node* node, osg::NodeCallback* cb)
	{
		/* paranoia check */
		if (!node) return;

		/* add initial callback */
		if (!node->getEventCallback())
		{		
			node->setEventCallback(cb);

		} else if (cb)
		{
			node->getEventCallback()->addNestedCallback(cb);
		}
	}
	

	void attachDefaultEventCallbacks(osg::Node* node, Marker* marker)
	{
		if (!node) {
			osg::notify() << "attachDefaultEventCallbacks: Can't attach callbacks to NULL node" << std::endl;
			return;
		}
		
		if (!marker) {
			osg::notify() << "attachDefaultEventCallbacks: Can't attach callbacks with NULL marker" << std::endl;
			return;
		}

		addEventCallback(node, new MarkerTransformCallback(marker));
		addEventCallback(node, new MarkerVisibilityCallback(marker));
	}


	SingleMarkerCallback::SingleMarkerCallback(Marker* marker) : 
		osg::NodeCallback(),
		m_marker(marker)
	{
	}

	DoubleMarkerCallback::DoubleMarkerCallback(Marker* markerA, Marker* markerB) : 
		osg::NodeCallback(),
		m_markerA(markerA),
		m_markerB(markerB)
	{
	}


	MarkerTransformCallback::MarkerTransformCallback(Marker* marker) : 
		SingleMarkerCallback(marker)
	{
	}


	/*virtual*/ 
	void MarkerTransformCallback::operator()(osg::Node* node, osg::NodeVisitor* nv) {

		// Handler for osg::MatrixTransforms
		if (osg::MatrixTransform* mt = dynamic_cast<osg::MatrixTransform*>(node)) {
			mt->setMatrix(m_marker->getTransform());
		}
		
		// Handler for osg::PositionAttitudeTransforms
		// TODO: check correct translation/rotation order
		else if (osg::PositionAttitudeTransform* pat = dynamic_cast<osg::PositionAttitudeTransform*>(node)) {
			pat->setPosition(m_marker->getTransform().getTrans());
			pat->setAttitude(m_marker->getTransform().getRotate());
		}

		// TODO: Handle other types of nodes... ?


		// Traverse the Node's subgraph
		traverse(node,nv);

	}

	MarkerVisibilityCallback::MarkerVisibilityCallback(Marker* marker) : 
		SingleMarkerCallback(marker)
	{
	}

	/*virtual*/ 
	void MarkerVisibilityCallback::operator()(osg::Node* node, osg::NodeVisitor* nv) {

		if (osg::Switch* _switch = dynamic_cast<osg::Switch*>(node)) 
		{
			// Handle visibilty for switch nodes

			_switch->setSingleChildOn(m_marker->valid() ? 0 : 1);	
		} 

		/*
		else if (  ) {

			// Potentially handle visibility on other types of node...

		}
		*/
		else 
		{

			// This method will work for any node.

			// Make sure the visitor will return to invisible nodes
			// If we make this node invisible (because the marker is hidden) then
			// we need to be able to return and update it later, or it will remain
			// hidden forever. 
			nv->setNodeMaskOverride(0xFFFFFFFF);

			node->setNodeMask(m_marker->valid() ? 0xFFFFFFFF : 0x0);
		}

		// must traverse the Node's subgraph            
		traverse(node,nv);

	}


	MarkerDebugCallback::MarkerDebugCallback(Marker* marker) : 
		SingleMarkerCallback(marker)
	{
	}

	/*virtual*/ 
	void MarkerDebugCallback::operator()(osg::Node* node, osg::NodeVisitor* nv) {

		if (m_marker->valid()) {

			// Debug information when marker is visible
			std::cout << 
				"Marker: " << m_marker->getName() << std::endl <<
				"Type: " << typeid(*m_marker).name() << std::endl << 
				"Confidence: " << m_marker->getConfidence() << std::endl << 
				"Transform: " << std::endl << m_marker->getTransform() << std::endl;

		}


		// must traverse the Node's subgraph            
		traverse(node,nv);
	}

	LocalTransformationCallback::LocalTransformationCallback(Marker* base, Marker* paddle) :
		DoubleMarkerCallback(base, paddle) 
	{
	}


	/*virtual*/ 
	void LocalTransformationCallback::operator()(osg::Node* node, osg::NodeVisitor* nv) {
    
		nv->setNodeMaskOverride(0xFFFFFFFF);

		osg::Matrix baseMatrix, paddleMatrix;

		bool bothValid = m_markerA->valid() && m_markerB->valid();
		node->setNodeMask(bothValid ? 0xFFFFFFFF : 0x0);

		if (bothValid) {
			
			baseMatrix = m_markerA->getTransform();
			paddleMatrix = m_markerB->getTransform();
			baseMatrix.invert(baseMatrix);

			if (osg::MatrixTransform* mt = dynamic_cast<osg::MatrixTransform*>(node)) {
				mt->setMatrix(paddleMatrix * baseMatrix);
			} 

		}

		// must traverse the Node's subgraph            
		traverse(node,nv);
	}



};
