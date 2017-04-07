#include "TextLabel.h"

TextLabel::TextLabel( string inText, Vec3f myPosition){
    Brodmann = inText;
    position = myPosition;
    string normalFont( "Arial" );
    TextLayout layout;
    layout.clear( ColorA( 0.0f, 0.0f, 0.0f, 0.0f ) );
    layout.setColor( Color( 1.0f, 1.0f, 1.0f ) );
    layout.setFont( Font( normalFont, 20 ) );
    layout.addCenteredLine( Brodmann );
    Surface8u rendered = layout.render( true, PREMULT );
    mTextTexture = gl::Texture( rendered );
}

void TextLabel::update(){
}


void TextLabel::draw(CameraPersp pointOfView){
   // Quatf viewQuat( Vec3f::xAxis(), M_PI );
    gl::enable( GL_TEXTURE_2D );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    glEnable(GL_DEPTH_TEST);
    gl::enableAlphaBlending();
    gl::pushModelView();
    glLoadIdentity();
    gl::setMatrices( pointOfView);
    gl::scale( Vec3f( -0.2, -0.2, -0.2 ) );
   // gl::rotate( pointOfView.getOrientation() );
  //  gl::rotate( viewQuat.inverse());
    gl::translate( (position.x * -5.0), position.y*-5.0, position.z*-5.0);
    gl::color( Color::white() );
    gl::draw( mTextTexture, Vec2f( 0,0 ) );
    gl::popModelView();
    gl::disable( GL_TEXTURE_2D );
    gl::disableAlphaBlending();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}





