//
//  connections.cpp
//  BrainMc2
//
//  Created by David Dalmazzo on 03/02/15.
//
//

#include "Connections.h"
#include "cinder/app/AppBasic.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

using namespace ci;
using namespace ci::app;

Connections::Connections(void): mLineWidth(1.0f){
}

Connections::~Connections(void){
}

//-------------------------------------------------------------------------------------------
void Connections::draw(){
    if(!mMesh) return;
    
    glPushAttrib( GL_CURRENT_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT );
    
    glLineWidth( mLineWidth );
    gl::color( Color(0.5f, 0.6f, 0.8f) * mAttenuation );
    gl::enableAdditiveBlending();
    
    gl::draw( mMesh );
    
    glPopAttrib();
}

//-------------------------------------------------------------------------------------------
void Connections::clear(){
    mMesh = gl::VboMesh();
    mVertices.clear();
    mIndices.clear();
}

//-------------------------------------------------------------------------------------------
void Connections::setCameraDistance( float distance ){
    static const float minimum = 0.25f;
    static const float maximum = 1.0f;
    static const float range = 50.0f;
    
    if( distance > range ) {
        mAttenuation = ci::lerp<float>( minimum, 0.0f, (distance - range) / range );
        mAttenuation = math<float>::clamp( mAttenuation, 0.0f, maximum );
    }
    else {
        mAttenuation = math<float>::clamp( 1.0f - math<float>::log10( distance ) / math<float>::log10(range), minimum, maximum );
    }
}

//-------------------------------------------------------------------------------------------
void Connections::createMesh(){
    gl::VboMesh::Layout layout;
    layout.setStaticPositions();
    layout.setStaticIndices();
    
    mMesh = gl::VboMesh(mVertices.size(), 0, layout, GL_LINES);
    mMesh.bufferPositions( &(mVertices.front()), mVertices.size() );
    mMesh.bufferIndices( mIndices );
}

//-------------------------------------------------------------------------------------------
Vec3d Connections::getCoordinate( double ra, double dec, double distance )
{
    double alpha = toRadians( ra * 15.0 );
    double delta = toRadians( dec );
    return distance * Vec3d( sin(alpha) * cos(delta), sin(delta), cos(alpha) * cos(delta) );
}

