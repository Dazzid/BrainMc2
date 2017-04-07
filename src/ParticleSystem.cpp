#include "ParticleSystem.h"


ParticleSystem::~ParticleSystem(){
    for( std::vector<Particle*>::iterator it = particles.begin(); it != particles.end(); ++it ){
        delete *it;
    }
    particles.clear();
}

void ParticleSystem::update(){
    for( std::vector<Particle*>::iterator it = particles.begin(); it != particles.end(); ++it ){
        (*it)->updatePosition();
    }
}

void ParticleSystem::draw(){
    for( std::vector<Particle*>::iterator it = particles.begin(); it != particles.end(); ++it ){
        (*it)->draw();
    }
}

void ParticleSystem::addParticle( Particle *particle ){
    particles.push_back( particle );
}

void ParticleSystem::destroyParticle( Particle *particle ){
    std::vector<Particle*>::iterator it = std::find( particles.begin(), particles.end(), particle );
    delete *it;
    particles.erase( it );
}


