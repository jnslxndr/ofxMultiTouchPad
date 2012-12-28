/* ------------------------------------------------------------------------- */
/*  ofxMultiTouchPad.cpp                                                     */
/* ------------------------------------------------------------------------- */
/*                                                                           */
/*  Created by jens a. ewald on 26.10.09.                                    */
/*                                                                           */
/*  http://www.openFrameworks.cc                                             */
/*                                                                           */
/*  access the data from the multitouchtrackpad sensor as addon for OF       */
/*                                                                           */
/*  based on the code at http://steike.com/code/multitouch                   */
/*  & the puredata external by Hans-Christoph Steiner                        */
/*                                                                           */
/*                                                                           */
/*  Copyright (c) 2009 - 2012 jens alexander ewald http://lea.io             */
/*  Copyright (c) 2009 Hans-Christoph Steiner                                */
/*  Copyright (c) 2008 Steike                                                */
/*                                                                           */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU General Public License               */
/* as published by the Free Software Foundation; either version 3            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* See file LICENSE for further informations on licensing terms.             */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software Foundation,   */
/* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA */
/*                                                                           */
/* ------------------------------------------------------------------------- */

#include "ofxMultiTouchPad.h"

int ofxMultiTouchPad::_guard = 0;
const int ofxMultiTouchPad::maxTouches = _NUM_TOUCH_FINGERS;


static int _mt_callback(int device, Finger *data, int nFingers,
                        double timestamp, int frame)
{
    memcpy(_touches, data, nFingers * sizeof(Finger));
    fingerCount = nFingers;
    
    memcpy(touchEvent.touches, data, nFingers * sizeof(Finger));
    touchEvent.count = nFingers;
    touchEvent.timestamp = timestamp;
    touchEvent.frame = frame;
    touchEvent.device = device;
    ofNotifyEvent(MTUpdateBlock, touchEvent);
    
    return 0;
}


ofxMultiTouchPad::ofxMultiTouchPad()
{
    _guard++;
    if (_guard==1) {
        printf("Creating Multitouch device\n");
        _mt_device = MTDeviceCreateDefault();
        MTRegisterContactFrameCallback(_mt_device, _mt_callback);
        MTDeviceStart(_mt_device, 0);
    }
    else {
        printf("there's another instance already created,"
               "we must not init the device\n");
    }
    ofAddListener(MTUpdateBlock, this,
                  &ofxMultiTouchPad::callBackTriggered);
    MTUpdateBlock.enable();
    
    this->fingers = &_touches;
    this->_fingerCount = &fingerCount;
}

ofxMultiTouchPad::~ofxMultiTouchPad()
{
    _guard--;
    if (_guard==0) {
        printf("Multitouch device has been disconnected\n");
        MTDeviceStop(_mt_device);
        MTUnregisterContactFrameCallback(_mt_device, _mt_callback);
        MTDeviceRelease(_mt_device);
        _mt_device = NULL;
    }
    ofRemoveListener(MTUpdateBlock, this,
                     &ofxMultiTouchPad::callBackTriggered);
    
    this->fingers = NULL;
    this->_fingerCount = NULL;
}


void Finger2MTouch(Finger &f,MTouch &_t)
{
  //if(f==NULL || _t==NULL) return;
  _t.ID    = f.identifier;
  _t.frame = f.frame;
  _t.x     = f.normalized.pos.x;
  _t.y     = 1.f-f.normalized.pos.y;
  if (f.size > 0.001) _t.angle = 180 - (f.angle * 90 / atan2(1,0));
  _t.size  = f.size;
}


bool ofxMultiTouchPad::getTouchAt(int pos, MTouch* touch)
{
  MTouch _t;
  bool success = false;
  if (pos < _touchData.count) {
    Finger f = _touchData.touches[pos];
    Finger2MTouch(f, _t);
    success = true;
  }
  *touch = _t;
  return success;
}

bool ofxMultiTouchPad::getTouchAsOfPointAt(int pos, ofPoint* p)
{
  ofPoint _t(0,0,0);
  bool success = false;
  if (pos < _touchData.count) {
    Finger f = _touchData.touches[pos];
    _t.x = f.normalized.pos.x;
    _t.y = 1.f-f.normalized.pos.y;
  }
  *p = _t;
  return success;
}

int ofxMultiTouchPad::getTouchCount()
{
    return _touchData.count;
}

void ofxMultiTouchPad::getTouchesAsOfPoints(std::vector<ofPoint> * pointv){
  if (pointv == NULL) return; // guard for evil pointers
  
  pointv->erase(pointv->begin(),pointv->begin()+pointv->size());
  
  TouchFrame _data = _touchData;
  for (int i=0; i<_data.count; i++)
    pointv->push_back(ofPoint(_data.touches[i].normalized.pos.x,
                              1-_data.touches[i].normalized.pos.y, 0));
}


MTouch ofxMultiTouchPad::getTouchAt(int pos)
{
  MTouch _t;
  if (pos < _touchData.count) {
    Finger f = _touchData.touches[pos];
    Finger2MTouch(f, _t);
  }
  return _t;
}

std::vector<MTouch> ofxMultiTouchPad::getTouches()
{
  std::vector<MTouch> touches;
  TouchFrame _data = _touchData;
  Finger f;  MTouch _t;
  for (int i=0; i<_data.count; i++)
  {
    f = _data.touches[i];
    Finger2MTouch(f, _t);
    touches.push_back(_t);
  }
  return touches;
}



// ------------------------------------------------------------- protected
void ofxMultiTouchPad::callBackTriggered(TouchFrame  & _t)
{
    int oldTouchCount = _touchData.count; // Cache the old count
    _touchData = _t; // Update the data
	
    if (oldTouchCount < _t.count) {
        /*
         TODO: extract the new touch and pass its ID
         */
        ofNotifyEvent(touchAdded, _t.count, this);
    } else if (oldTouchCount > _t.count) {
        ofNotifyEvent(touchRemoved, _t.count, this);
    } else {
	ofNotifyEvent(update, _t.count, this);
    }
}