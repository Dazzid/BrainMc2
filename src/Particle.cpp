#include "Particle.h"

Particle::Particle( const ci::Vec3f& inPosition, float inRadius, float inMass, float inDrag, const ci::Vec4f inColor  ){
    position = inPosition;
    radius = inRadius;
    mass = inMass;
    drag = inDrag;
    colors = inColor;
    average = 0;
    prevPosition = inPosition;
    fixLocation = inPosition;
    forces = ci::Vec3f::zero();
    signalsAndTime.clear();
}

void Particle::update(){
    
    ci::Vec3f temp = position;
    //ci::Vec3f vel = ( position - prevPosition ) * drag;
    //position += vel + forces / mass;
    prevPosition = temp;
    forces = ci::Vec3f::zero();
}
void Particle::setSignals(float inValue){
    signalsAndTime.push_back(inValue);
}

float Particle::getSignals(int inPosition){
    if(!signalsAndTime.empty() && inPosition <= signalsAndTime.size()){
    return signalsAndTime[inPosition];
    }
    else return 0.0f;
}

void Particle::draw(){

}

void Particle::updatePosition(){
    
}

void Particle::setAverage(float inAvg){
    this->average = inAvg;
}

float Particle::getAverage(){
    return average;
}

void Particle::setId(int inId){
    this->myId=inId;
}

int Particle::getId(){
    return myId;
}

void Particle::setDistance(float inDistance){
    this->myDistance = inDistance;
}

float Particle::getDistance(){
    return myDistance;
}

void Particle::updateColors(){
    //ir = temporalColors.x;
    //ig = temporalColors.y;
    //ib = temporalColors.z;
    ir += (temporalColors.x - ir) * easing;
    ig += (temporalColors.y - ig) * easing;
    ib += (temporalColors.z - ib) * easing;
    ab += (temporalColors.w - ab) * easing;
    //ab = temporalColors.w;
    colors = ci::Vec4f(ir, ig, ib, ab);
    //colors = temporalColors;
}

void Particle::setColor(cinder::Vec4f inColors){
    temporalColors = inColors;
}

void Particle::rotate(int xi, int yi, int zi, float degrees){
    float radians = degrees * float (M_PI) / 180.0f;
    ci::Matrix44f transform;
    transform.setToIdentity();
    transform.rotate( ci::Vec3f(xi, yi, zi), radians );
    ci::Vec3f node = this->position;
    position = transform.transformPoint(node);
}
