#include "cinder/app/AppBasic.h"
#include "cinder/params/Params.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/Camera.h"
#include "cinder/Rand.h"
#include "cinder/Text.h"
#include "cinder/gl/TextureFont.h"

#include "Resources.h"
#include "Controller.h"
#include "Room.h"
#include "SpringCam.h"
#include "ParticleSystem.h"
#include "cinder/Json.h"
#include "Talairach.h"
#include "TextLabel.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define APP_WIDTH		1400
#define APP_HEIGHT		800
#define PI              3.14159265358979323846
#define SIGNAL_POINTS   1200

#define ROOM_WIDTH		350
#define ROOM_HEIGHT		150
#define ROOM_DEPTH		250
#define ROOM_FBO_RES	2
#define FBO_DIM			50//167


struct signalActivity {
    int id;
    float timeSignal;
};

struct ParticleOrder
{
    int id = 0; //place of the array
    int particleId = 0;
    int point = 0;
    float distance = 0.0;
    float zScore = 0.0;
    float kind = 1.0;
    Vec3f myPosition = Vec3f(0,0,0);
    Vec3f referencePositionForRegion = Vec3f(0,0,0);
    Vec4f myColor = Vec4f(0,0,0,0);
    vector<Vec3f> myConnections;
    string level_1= "";
    string level_2= "";
    string level_3= "";
    string level_4= "";
    string Brodmann= "";
    string range_mm= "";
    string sideBrodmann= "";
    
};

struct HistogramAreas
{
    string Brodmann= "";
    string side = "";
    int positiveCounter = 0;
    int negativeCounter = 0;
};

bool sortByDistanceOne(const ParticleOrder &first, const ParticleOrder &second) { return first.distance < second.distance; }
bool sortByDistanceTwo(const ParticleOrder &first, const ParticleOrder &second) { return first.distance > second.distance; }
bool sortById(const ParticleOrder &lhs, const ParticleOrder &rhs) { return lhs.id < rhs.id; }

class BrainMc2App : public AppBasic {
public:
    virtual void      prepareSettings( Settings *settings );
    virtual void      setup();
    virtual void      mouseDown( MouseEvent event );
    virtual void      mouseUp( MouseEvent event );
    virtual void      mouseMove( MouseEvent event );
    virtual void      mouseDrag( MouseEvent event );
    virtual void      mouseWheel( MouseEvent event );
    virtual void      keyDown( KeyEvent event );
    virtual void      update();
    virtual void      draw();
    void              drawIntoRoomFbo();
    void              drawInfoPanel();
    void              readData();
    void              readFile();
    void              seeking();
    void              readNewFile(int timeline);
    void              readColorBar();
    void              colorMap(float inValue);
    void              colorAverage();
    void              setConnections();
    void              readAreas();
    void              drawConnections();
    void              paintAll();
    void              filterRegion( string myRegion, int mode);
    void              getAreas();
    void              filterAverage();
    void              getAllAreas();
    void              sumHistogram();
    void              printResult();
    void              regionToPlot();
    virtual void      setDataPoints(int in);
    virtual void      runSimulation();
    
    // CAMERA
    SpringCam           mSpringCam;
    
    // SHADERS
    gl::GlslProg        mRoomShader;
    
    // TEXTURES
    gl::Texture         mIconTex;
    
    // ROOM
    Room                mRoom;
    gl::Fbo             mRoomFbo;
    
    // CONTROLLER
    Controller          mController;
    
    // MOUSE
    Vec2f               mMousePos, mMouseDownPos, mMouseOffset;
    bool                mMousePressed;
    
    //Talairach Zone
    Talairach           myTalairach;
    Vec3f               locBrain;
    int                 cubeSize;
    
    bool                mSaveFrames;
    bool                running     = false;
    bool                loadingData = true;
    bool                readTheFile = false;
    bool                drawAllConnections = false;
    int                 mNumSavedFrames;
    int                 dataCounter = 0;
    
    gl::Texture         mTexture;
    gl::Texture         imageTexture;
    ParticleSystem      mParticleSystem;
    int                 mNumParticles;
    float               *mRadiuses;
    float               cameraDepth   = -150.0f;
    
    gl::GlslProg        mShader;
    vector<Vec3f>       positions;
    vector<Vec3f>       colorBar;
    vector<Vec3f>       allAverageColors;
    vector<float>       allAverageValues;
    vector<string>      allBrodmannAreas;
    vector<vector<signalActivity>> AllSignals;
    Vec4f               *theColors;
    FILE                *pFile;
    long                fileSize;
    float               alpha = 1.0;
    float               radius = 1.5f;
    
    Vec3f               *allPositions;
    Vec3f               cameraPosition;
    int                 seekCounter = 0;
    Quatf				mSceneRotation;
    
    int step = 0;
    
    //distances of connections
    float maxDist = 18.0;
    float minDist = 5.0;
    float lineThickness = 0.5;
    float windowThreshold = 210; //distance value of activity per time frame upper and lower of the mean
    
    vector<ParticleOrder> particleOrder;
    vector<HistogramAreas> histogramAreas;
    
    // PARAMS
    params::InterfaceGl	mParams;
    bool				mShowParams;
    string				mySearch;
    int                 level = 5;
    
    //Text
    vector<TextLabel>   myText;
    
    //Regions Search
    vector<string> regionsSearch;
    
};

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::prepareSettings( Settings *settings )
{
    settings->setWindowSize( APP_WIDTH, APP_HEIGHT );
    settings->setBorderless( false );
    settings->setResizable( true );
    settings->setFullScreen( false );
}

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::readData(){
    try{
        JsonTree json(app::loadResource("xyz_80k.json"));
        
        for( auto &nodes : json["nodes"].getChildren() ){
            // std::cout << nodes << std::endl;
            float x = nodes[0].getValue<float>() + ci::randFloat(-1.0f, 1.0f);
            float y = nodes[1].getValue<float>() + ci::randFloat(-1.0f, 1.0f);
            float z = nodes[2].getValue<float>() + ci::randFloat(-1.0f, 1.0f);
            positions.push_back(Vec3f(x,y,z));
        }
        json = JsonTree();
        std::cout << "xyz ready! " << std::endl;
    }
    catch(...){
        std::cout << "Failed to parse xyz." << std::endl;
    }
    
    // Reading average colors
    try{
        JsonTree json2(app::loadResource("allColors.json"));
        for( auto &values : json2["allColors"].getChildren() ){
            float r = values[0].getValue<float>();
            float g = values[1].getValue<float>();
            float b = values[2].getValue<float>();
            allAverageColors.push_back(Vec3f(r,g,b));
            
        }
        json2 = JsonTree();
        std::cout << "average color ready! " << std::endl;
    }
    catch(...){
        std::cout << "Failed to parse average color." << std::endl;
    }
    
    // Reading average
    try{
        JsonTree json2(app::loadResource("averageActivity_80k.json"));
        for( auto &values : json2["averageActivity"].getChildren() ){
            float d = values.getValue<float>();
            allAverageValues.push_back(d);
        }
        json2 = JsonTree();
        std::cout << "average values ready! " << std::endl;
    }
    catch(...){
        std::cout << "Failed to parse average color." << std::endl;
    }
}

//Rigions to plot
void BrainMc2App::regionToPlot(){
    ifstream myfile( getResourcePath("regions_search.txt").c_str(), ifstream::in);
    string line;
    if(!myfile.is_open())
    {
        cout<<"\n Cannot open the regions_search.txt file"<<"\n";
    }
    else
    {
        while( myfile.good() )
        {
            getline( myfile , line);
            regionsSearch.push_back(line);
            // cout<<line<<endl;
        }
    }
}

//Reding Areas
//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::readAreas(){
    ifstream myfile( getResourcePath("nearGrayMatter.txt").c_str(), ifstream::in);
    string line;
    int point = mNumParticles-1;
    int reference = 0;
    if(!myfile.is_open())
    {
        cout<<"\n Cannot open the xyz_80k_csv_areas.txt file"<<"\n";
    }
    else
    {
        while( myfile.good() )
        {
            getline( myfile , line);
            vector< string >  data = ci::split( line, ","";", false );
            if(reference >= 1){
                string area = data[4];
                vector< string >  val = ci::split( area, " ", false );
                string sideBrodmann = val[0] + " " + data[8];
                // cout<<sideBrodmann<<endl;
                mParticleSystem.particles[point]->level_1 = data[4];
                mParticleSystem.particles[point]->level_2 = data[5];
                mParticleSystem.particles[point]->level_3 = data[6];
                mParticleSystem.particles[point]->level_4 = data[7];
                mParticleSystem.particles[point]->Brodmann = data[8];
                mParticleSystem.particles[point]->sideBrodmann = sideBrodmann;
                //cout<<"data areas: "<<data[0]<<" "<<data[4]<<" "<<data[5]<<" "<<data[6]<<" "<<data[7]<<" "<<data[8]<<endl;
                point--;
            }
            reference++;
        }
        cout<<"loading areas ready! " << endl;
    }
    myfile.close();
}
//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::readColorBar(){
    ifstream myfile( getResourcePath("colorMap.txt").c_str(), ifstream::in);
    string line;
    if(!myfile.is_open())
    {
        cout<<"\n Cannot open the colorMap.txt file"<<"\n";
    }
    else
    {
        while( myfile.good() )
        {
            getline( myfile , line);
            vector< string >  v1 = ci::split( line, ","";", false );
            vector< string >  v2 = ci::split( v1[1], " ", false );
            colorBar.push_back(Vec3f(std::stof(v2[1]), std::stof(v2[2]), std::stof(v2[3])));
            //cout<<"data: "<<v1[0]<< endl;
        }
        cout<<"colorbar done! " << endl;
    }
    myfile.close();
}
//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::seeking(){
    //aun no termina de funcionar ... los valores son diferentes a la version original
    int fd;
    int ret;
    std::string b;
    const size_t NUM_ELEMS = 8;
    const size_t NUM_BYTES = NUM_ELEMS * sizeof(char);
    
    fd = open("/Users/David/Dropbox/BrainMc2/assets/signal_80k.txt",O_RDONLY);
    if(fd < 0){
        perror("open");
        //exit(1);
    }
    
    ret = lseek(fd, seekCounter*NUM_BYTES, SEEK_SET);
    ret = read(fd, &b, sizeof(char));
    cout<<"> " << seekCounter << ": " << b<<endl;
    seekCounter++;
    close(fd);
    
}

//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::readFile(){
    ifstream inFile("/Users/David/Dropbox/BrainMc2/assets/signal_80k.txt");
    string line;
    int count = 0 ;
    int reference = mParticleSystem.particles.size()-1;
    if(!inFile.is_open())
    {
        cout<<"\n Cannot open the signal_80k.txt file"<<"\n";
    }
    else
    {
        cout<<"loading all data... "<<"\n";
        while(getline( inFile , line)){
            vector< string > numbers = ci::split( line, " ", false );
            for(int i = 0; i <numbers.size(); i++){
                try{
                    float thisNumber =  std::stof(numbers.at(i));
                    mParticleSystem.particles.at(reference)->signalsAndTime.push_back(thisNumber);
                    //cout<<"numbers at: " << reference << " = "<< thisNumber <<"\n";
                }
                catch (...){
                }
            }
            count++;
            reference--;
            //cout<<"done: "<<count<<"\n";
        }
        cout<<"all data ready!"<<"\n";
        inFile.close();
    }
}

//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::readNewFile(int timeline){
    ifstream inFile("/Users/David/Dropbox/BrainMc2/assets/signal_2_out.txt");
    //it has 1200 lines with 80k values per node
    string line;
    int count = 0 ;
    int reference = mParticleSystem.particles.size()-1;
    if(!inFile.is_open())
    {
        cout<<"\n Cannot open the signal_2_out.txt file"<<"\n";
    }
    else
    {
        //cout<<"loading data... "<<"\n";
        if(timeline < SIGNAL_POINTS){
            while(getline( inFile , line) ){
                if(count == timeline){
                    vector< string > numbers = ci::split( line, "   ", false );
                    for(int i = 0; i <numbers.size(); i++){
                        try{
                            float thisNumber =  std::stof(numbers.at(i));
                            mParticleSystem.particles.at(reference)->timeSeriesSignal = thisNumber;
                            //cout<<"numbers at: " << reference << " = "<< thisNumber <<"\n";
                            reference--;
                        }
                        catch (...){
                        }
                    }
                }
                else if(count > timeline)
                {
                    //cout<<"signal ready! "<<"\n";
                    inFile.close();
                    break;
                }
                count++;
            }
        }
    }
}
//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::filterAverage(){
    int reference = mParticleSystem.particles.size()-1;
    float maximum = 10000.0;
    for (int i=0; i<mParticleSystem.particles.size();i++){
        float inData = allAverageValues[reference];
        //float lookAt = lmap<float>( mParticleSystem.particles[i]->getSignals(in), 0.0f, 18000.0f, 0.0f, 788.0f );
        float lookAt = lmap<float>(inData, 0.0f, 21566.1f, 0.0f, 788.0f );
        if(maximum < inData){
            maximum = inData;
        }
        mParticleSystem.particles[i]->zScore = inData;
        int point = int(lookAt);
        if(point > colorBar.size()-1) point = 788;
        if(point > colorBar.size()/2 + windowThreshold) {
            alpha = 1.0f;
            mParticleSystem.particles[i]->kind = 1;
        }
        else if(point < colorBar.size()/2 - windowThreshold){
            alpha = 1.0f;
            mParticleSystem.particles[i]->kind = 1;
        }
        else {
            alpha = 0.0f;
            mParticleSystem.particles[i]->kind = 0;
        }
        mParticleSystem.particles[i]->setColor(Vec4f(colorBar[point].x, colorBar[point].y, colorBar[point].z, alpha ));
        mParticleSystem.particles[i]->point = point;
        particleOrder[i].point = mParticleSystem.particles[i]->point;
        particleOrder[i].kind = mParticleSystem.particles[i]->kind;
        particleOrder[i].myPosition = mParticleSystem.particles[i]->position;
        particleOrder[i].myColor = mParticleSystem.particles[i]->temporalColors;
        particleOrder[i].level_1 = mParticleSystem.particles[i]->level_1;
        particleOrder[i].level_2 = mParticleSystem.particles[i]->level_2;
        particleOrder[i].level_3 = mParticleSystem.particles[i]->level_3;
        particleOrder[i].level_4 = mParticleSystem.particles[i]->level_4;
        particleOrder[i].zScore = mParticleSystem.particles[i]->zScore;
        particleOrder[i].Brodmann = mParticleSystem.particles[i]->Brodmann;
        particleOrder[i].sideBrodmann = mParticleSystem.particles[i]->sideBrodmann;
        /* if(point > 620){
         alpha = 0.15f;
         mParticleSystem.particles[i]->kind = 2;
         }
         else {
         alpha = 0.0f;
         mParticleSystem.particles[i]->kind = -1;
         }
         mParticleSystem.particles[i]->setColor(Vec4f(colorBar[point].x, colorBar[point].y, colorBar[point].z, alpha ));*/
        reference --;
    }
    cout<<maximum<<endl;
}

//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::setDataPoints(int in){
    int reference = mParticleSystem.particles.size()-1;
    float maximum = 0;
    float minimum = 0;
    
    //Checking range of values
    for (int i=0; i<mParticleSystem.particles.size();i++){
        float inData = allAverageValues[reference];
        float realValue = mParticleSystem.particles[i]->getSignals(in)-inData;

        if(maximum < realValue){
            maximum = realValue;
        }
        if(minimum > realValue){
            minimum = realValue;
        }
        reference --;
    }

    //define variance
    reference = mParticleSystem.particles.size()-1;
    for (int i=0; i<mParticleSystem.particles.size();i++){
        float inData = allAverageValues[reference];
        float lookAt = lmap<float>(mParticleSystem.particles[i]->getSignals(in)-inData, minimum*0.5, maximum*0.5, 0.0f, 788.0f );
        mParticleSystem.particles[i]->zScore = mParticleSystem.particles[i]->getSignals(in)-inData;
        int point = int(lookAt);
        if(point > colorBar.size()-1) point = 788;
        if(point > colorBar.size()/2 + windowThreshold) {
            alpha = 1.0f;
            mParticleSystem.particles[i]->kind = 1;
        }
        else if(point < colorBar.size()/2 - windowThreshold){
            alpha = 1.0f;
            mParticleSystem.particles[i]->kind = 1;
        }
        else {
            alpha = 0.0f;
            mParticleSystem.particles[i]->kind = 0;
        }
        mParticleSystem.particles[i]->setColor(Vec4f(colorBar[point].x, colorBar[point].y, colorBar[point].z, alpha ));
        mParticleSystem.particles[i]->point = point;
        particleOrder[i].point = mParticleSystem.particles[i]->point;
        particleOrder[i].kind = mParticleSystem.particles[i]->kind;
        particleOrder[i].myPosition = mParticleSystem.particles[i]->position;
        particleOrder[i].myColor = mParticleSystem.particles[i]->temporalColors;
        particleOrder[i].level_1 = mParticleSystem.particles[i]->level_1;
        particleOrder[i].level_2 = mParticleSystem.particles[i]->level_2;
        particleOrder[i].level_3 = mParticleSystem.particles[i]->level_3;
        particleOrder[i].level_4 = mParticleSystem.particles[i]->level_4;
        particleOrder[i].zScore = mParticleSystem.particles[i]->zScore;
        particleOrder[i].Brodmann = mParticleSystem.particles[i]->Brodmann;
        particleOrder[i].sideBrodmann = mParticleSystem.particles[i]->sideBrodmann;
        /* if(point > 620){
         alpha = 0.15f;
         mParticleSystem.particles[i]->kind = 2;
         }
         else {
         alpha = 0.0f;
         mParticleSystem.particles[i]->kind = -1;
         }
         mParticleSystem.particles[i]->setColor(Vec4f(colorBar[point].x, colorBar[point].y, colorBar[point].z, alpha ));*/
        reference --;
    }
    cout<<"reading: " << in << " max:"<<maximum<<" min:" << minimum<<endl;
}

//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::runSimulation(){
    
    step++;
    if(step == 1 && dataCounter<=SIGNAL_POINTS-1){
        dataCounter++;
        //dataCounter= dataCounter%SIGNAL_POINTS;
        setDataPoints(dataCounter);
    }
    else if(step == 2){
        sumHistogram();
    }
    else if(dataCounter == SIGNAL_POINTS-1){
        printResult();
    }
    step = step%3;
}
//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::colorAverage(){
    int counter = 0;
    alpha = 1.0f;
    for (int i=mParticleSystem.particles.size()-1;i >= 0; i--){
        mParticleSystem.particles[i]->setColor(Vec4f(allAverageColors[counter].x, allAverageColors[counter].y, allAverageColors[counter].z, alpha ));
        mParticleSystem.particles[i]->setAverage(allAverageValues[counter]);
        counter ++;
    }
    //important it check if particleOrder was created, so this method is called differently the very fisrt time it runs
    if(particleOrder.size() != 0){
        for (int i=mParticleSystem.particles.size()-1;i >= 0; i--){
            particleOrder[i].point = mParticleSystem.particles[i]->point;
            particleOrder[i].myPosition = mParticleSystem.particles[i]->position;
            particleOrder[i].myColor = mParticleSystem.particles[i]->colors;
            particleOrder[i].level_1 = mParticleSystem.particles[i]->level_1;
            particleOrder[i].level_2 = mParticleSystem.particles[i]->level_2;
            particleOrder[i].level_3 = mParticleSystem.particles[i]->level_3;
            particleOrder[i].level_4 = mParticleSystem.particles[i]->level_4;
            particleOrder[i].Brodmann = mParticleSystem.particles[i]->Brodmann;
            particleOrder[i].sideBrodmann = mParticleSystem.particles[i]->sideBrodmann;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::setConnections(){
    for (int i=0; i<particleOrder.size();i++){
        particleOrder[i].myConnections.clear();
        if(particleOrder[i].kind == 1){
            for (int j=i+1; j<particleOrder.size()-1;j++){
                if(particleOrder[j].kind == 1){
                    float dist = particleOrder[i].myPosition.distance(particleOrder[j].myPosition);
                    if(dist > minDist && dist < maxDist && particleOrder[i].myColor == particleOrder[j].myColor){
                        particleOrder[i].myConnections.push_back(particleOrder[j].myPosition);
                    }
                }
            }
        }
    }
    
}

//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::drawConnections(){
    float pointalpha = (mRoom.getLightPower() * 0.8f) + 0.2f;
    gl::lineWidth(lineThickness);
    for (int i=0; i<particleOrder.size();i++){
        for (int j=0; j<particleOrder[i].myConnections.size();j++){
            gl::color(particleOrder[i].myColor.x, particleOrder[i].myColor.y,particleOrder[i].myColor.z, pointalpha);
            gl::drawLine(particleOrder[i].myPosition, particleOrder[i].myConnections[j]);
        }
    }
    gl::color( ColorA( 1.0f, 1.0f, 1.0f, 1.0f ) );
    //if(mRoom.isPowerOn()) lineThickness = 1.0;
    //else lineThickness = 0.33;
}

//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::paintAll(){
    for(int i = 0; i<mNumParticles; i++){
        particleOrder[i].myColor.w = 1.0;
    }
}
//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::getAllAreas(){
    string thisAreas = "null";
    myText.clear();
    allBrodmannAreas.clear();
    histogramAreas.clear();
    for(int i = 0; i < particleOrder.size(); i++){
        if(particleOrder[i].kind == 1){
            if(thisAreas != particleOrder[i].sideBrodmann){
                thisAreas = particleOrder[i].sideBrodmann;
                if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), thisAreas) != allBrodmannAreas.end()) {
                    //cout<<"it contains: "<<thisAreas<<" in "<<i<<endl;
                } else {
                    histogramAreas.push_back(*new HistogramAreas);
                    allBrodmannAreas.push_back(thisAreas);
                    // myText.push_back(*new TextLabel(thisAreas, particleOrder[i].myPosition));
                }
            }
        }
    }
    for(int i = 0; i < histogramAreas.size(); i++){
        cout<<i<<" "<<allBrodmannAreas[i]<<endl;
        histogramAreas[i].Brodmann = allBrodmannAreas[i];
    }
}
//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::sumHistogram(){
    //positive
    string thisAreas = "null";
    myText.clear();
    allBrodmannAreas.clear();
    for(int i = 0; i < particleOrder.size(); i++){
        if(particleOrder[i].point >= colorBar.size() - 10 && particleOrder[i].kind == 1){
            if(thisAreas != particleOrder[i].sideBrodmann){
                thisAreas = particleOrder[i].sideBrodmann;
                if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), thisAreas) != allBrodmannAreas.end()) {
                    //cout<<"it contains: "<<thisAreas<<" in "<<i<<endl;
                } else {
                    allBrodmannAreas.push_back(thisAreas);
                }
            }
        }
    }
    for(int i = 0; i < histogramAreas.size(); i++){
        if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), histogramAreas[i].Brodmann) != allBrodmannAreas.end()) {
            //cout<< "positive "<<i<<": "<<histogramAreas[i].Brodmann<<endl;
            histogramAreas[i].positiveCounter++;
        }
    }
    
    //negative
    thisAreas = "";
    myText.clear();
    allBrodmannAreas.clear();
    for(int i = 0; i < particleOrder.size(); i++){
        if(particleOrder[i].point <= 50 && particleOrder[i].kind == 1){
            if(thisAreas != particleOrder[i].sideBrodmann){
                thisAreas = particleOrder[i].sideBrodmann;
                if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), thisAreas) != allBrodmannAreas.end()) {
                    //cout<<"it contains: "<<thisAreas<<" in "<<i<<endl;
                } else {
                    allBrodmannAreas.push_back(thisAreas);
                }
            }
        }
    }
    for(int i = 0; i < histogramAreas.size(); i++){
        if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), histogramAreas[i].Brodmann) != allBrodmannAreas.end()) {
            //cout<< "negative "<<i<<": "<<histogramAreas[i].Brodmann<<endl;
            histogramAreas[i].negativeCounter++;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::printResult(){
    for(int i = 0; i < histogramAreas.size(); i++){
        cout<<histogramAreas[i].Brodmann<<"; "<<histogramAreas[i].positiveCounter<<"; "<<histogramAreas[i].negativeCounter<<endl;
    }
}

//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::getAreas(){
    string thisAreas = "null";
    myText.clear();
    allBrodmannAreas.clear();
    for(int i = 0; i < particleOrder.size(); i++){
        if(particleOrder[i].point >= colorBar.size() - 10 && particleOrder[i].kind == 1){
            if(thisAreas != particleOrder[i].Brodmann){
                thisAreas = particleOrder[i].Brodmann;
                if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), thisAreas) != allBrodmannAreas.end()) {
                    //cout<<"it contains: "<<thisAreas<<" in "<<i<<endl;
                } else {
                    allBrodmannAreas.push_back(thisAreas);
                    myText.push_back(*new TextLabel(thisAreas, particleOrder[i].myPosition));
                    cout << particleOrder[i].level_1  <<", "<<particleOrder[i].level_3 << ", " <<particleOrder[i].myPosition.x <<" "<<particleOrder[i].myPosition.y<<" "<<particleOrder[i].myPosition.z << ", " << particleOrder[i].Brodmann << ", " << particleOrder[i].zScore*0.01f << "," << endl;
                }
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------
void BrainMc2App::filterRegion(string myRegion, int mode){
    float totalAlha = 1.0;
    float littleAlpha = 0.05;
    myText.clear();
    allBrodmannAreas.clear();
    vector<int> myReference;
    switch (mode) {
        case 1:
            for(int i = 0; i<mNumParticles; i++){
                if(particleOrder[i].level_1 == myRegion && particleOrder[i].kind == 1){
                    particleOrder[i].myColor.w = totalAlha;
                    if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), particleOrder[i].level_1) != allBrodmannAreas.end()) {
                    } else {
                        myText.push_back(*new TextLabel(particleOrder[i].level_1, particleOrder[i].myPosition));
                        allBrodmannAreas.push_back(particleOrder[i].level_1);
                    }
                    
                }
                else {
                    particleOrder[i].myColor.w = littleAlpha;
                }
            }
            break;
        case 2:
            for(int i = 0; i<mNumParticles; i++){
                if(particleOrder[i].level_2 == myRegion && particleOrder[i].kind == 1){
                    particleOrder[i].myColor.w = totalAlha;
                    if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), particleOrder[i].level_2) != allBrodmannAreas.end()) {
                    } else {
                        allBrodmannAreas.push_back(particleOrder[i].level_2);
                        myText.push_back(*new TextLabel(particleOrder[i].level_2, particleOrder[i].myPosition));
                    }
                } else {
                    particleOrder[i].myColor.w = littleAlpha;
                }
            }
            break;
        case 3:
            for(int i = 0; i<mNumParticles; i++){
                if(particleOrder[i].level_3 == myRegion && particleOrder[i].kind == 1){
                    particleOrder[i].myColor.w = totalAlha;
                    if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), particleOrder[i].level_3) != allBrodmannAreas.end()) {
                    } else {
                        allBrodmannAreas.push_back(particleOrder[i].level_3);
                        myText.push_back(*new TextLabel(particleOrder[i].level_3, particleOrder[i].myPosition));
                    }
                } else {
                    particleOrder[i].myColor.w = littleAlpha;
                }
            }
            break;
        case 4:
            for(int i = 0; i<mNumParticles; i++){
                if(particleOrder[i].level_4 == myRegion && particleOrder[i].kind == 1){
                    particleOrder[i].myColor.w = totalAlha;
                    if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), particleOrder[i].level_4) != allBrodmannAreas.end()) {
                    } else {
                        allBrodmannAreas.push_back(particleOrder[i].level_4);
                        myText.push_back(*new TextLabel(particleOrder[i].level_4, particleOrder[i].myPosition));
                    }
                } else {
                    particleOrder[i].myColor.w = littleAlpha;
                }
            }
            break;
        case 5:
            for(int i = 0; i<mNumParticles; i++){
                if(particleOrder[i].Brodmann == myRegion && particleOrder[i].kind == 1){
                    particleOrder[i].myColor.w = totalAlha;
                    myReference.push_back(i);
                    if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), particleOrder[i].Brodmann) != allBrodmannAreas.end()) {
                    } else {
                        allBrodmannAreas.push_back(particleOrder[i].Brodmann);
                        myText.push_back(*new TextLabel(particleOrder[i].Brodmann, particleOrder[i].myPosition));
                    }
                } else {
                    particleOrder[i].myColor.w = littleAlpha;
                }
            }
            if(myReference.size() > 0 && particleOrder[myReference[myReference.size()-1]].Brodmann == myRegion){
                myText.push_back(*new TextLabel(particleOrder[myReference[myReference.size()-1]].Brodmann, particleOrder[myReference[myReference.size()-1]].myPosition));
            }
            myReference.clear();
            break;
        case 6:
            allBrodmannAreas.clear();
            for(int i = 0; i<mNumParticles; i++){
                particleOrder[i].myColor.w = littleAlpha;
            }
            for(int i = 0; i<mNumParticles; i++){
                if(find(regionsSearch.begin(), regionsSearch.end(), particleOrder[i].sideBrodmann) != regionsSearch.end()) {
                    particleOrder[i].myColor.w = totalAlha;
                    particleOrder[i].kind = 1;
                }
            }
            for(int i = 0; i<mNumParticles; i++){
                if(particleOrder[i].kind == 1){
                    if(find(allBrodmannAreas.begin(), allBrodmannAreas.end(), particleOrder[i].sideBrodmann) != allBrodmannAreas.end()) {
                    } else {
                        allBrodmannAreas.push_back(particleOrder[i].sideBrodmann);
                        myText.push_back(*new TextLabel(particleOrder[i].Brodmann, particleOrder[i].myPosition));
                    }
                }
            }
            break;
        default:
            for(int i = 0; i<mNumParticles; i++){
                particleOrder[i].myColor.w = totalAlha;
            }
            break;
    }
}

/*                                                      SETUP                                                   */
//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::setup()
{
    // CAMERA
    mSpringCam		= SpringCam( -10.0f, getWindowAspectRatio() );
    
    // LOAD SHADERS
    try {
        mRoomShader	= gl::GlslProg( loadResource( "room.vert" ), loadResource( "room.frag" ) );
    } catch( gl::GlslProgCompileExc e ) {
        std::cout << e.what() << std::endl;
        quit();
    }
    
    //hideCursor ();
    
    // TEXTURE FORMAT
    gl::Texture::Format mipFmt;
    mipFmt.enableMipmapping( true );
    mipFmt.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
    mipFmt.setMagFilter( GL_LINEAR );
    
    // LOAD TEXTURES
    mIconTex			= gl::Texture( loadImage( loadResource( "iconRoom.png" ) ), mipFmt );
    
    // ROOM
    gl::Fbo::Format roomFormat;
    roomFormat.setColorInternalFormat( GL_RGB );
    mRoomFbo			= gl::Fbo( APP_WIDTH/ROOM_FBO_RES, APP_HEIGHT/ROOM_FBO_RES, roomFormat );
    bool isPowerOn		= false;
    bool isGravityOn	= false;
    mRoom				= Room( Vec3f( ROOM_WIDTH, ROOM_HEIGHT, ROOM_DEPTH ), isPowerOn, isGravityOn );
    mRoom.init();
    
    // MOUSE
    mMousePos			= Vec2f::zero();
    mMouseDownPos		= Vec2f::zero();
    mMouseOffset		= Vec2f::zero();
    mMousePressed		= false;
    
    // CONTROLLER
    mController.init( &mRoom );
    
    mSaveFrames		= false;
    mNumSavedFrames = 0;
    
    mTexture = loadImage( loadAsset( "particle.png" ) );
    mShader = gl::GlslProg( loadAsset( "shader.vert"), loadAsset( "shader.frag" ) );
    
    readData();
    readColorBar();
    regionToPlot();
    
    mNumParticles = positions.size();
    theColors = new Vec4f[ mNumParticles ];
    allPositions = new Vec3f[mNumParticles];
    mRadiuses = new float[ mNumParticles ];
    
    for(int i = positions.size()-1; i>=0; i--){
        Particle *particle = new Particle( positions.at(i), radius, 1.0f, 0.9f, Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
        mParticleSystem.addParticle( particle );
    }
    
    //reading all the data
    if(readTheFile)readFile(); //read all points from timeSeries
    positions.clear();
    colorAverage();
    readNewFile(0);
    readAreas();
    
    //glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
    glDepthRange(1, -1);
    
    getAllAreas();
    //readFile();    //-------> time file
    
    //	PARAMS
    mShowParams		= true;
    mParams			= params::InterfaceGl( "BrainX3", Vec2i( 400, 220 ), ColorA(0.4f,0.4f,0.4f,0.4f) );
    // mParams.addParam( "Scene Rotation", &mSceneRotation, "opened=1" );
    mParams.addSeparator();
    mParams.addParam( "ActivityWindowThreshold", &windowThreshold, "min=1.0 max=390.0 step=1.0 keyIncr=t keyDecr=T" );
    mParams.addSeparator();
    mParams.addParam( "minDistance", &minDist, "min=1.0 max=50.0 step=0.25 keyIncr=d keyDecr=D" );
    mParams.addParam( "maxDistance", &maxDist, "min=5.0 max=150.0 step=0.25 keyIncr=f keyDecr=F" );
    mParams.addSeparator();
    mParams.addParam( "lineThickness", &lineThickness, "min=0.1 max=2.0 step=0.1 keyIncr=l keyDecr=L" );
    mParams.addSeparator();
    mParams.addText( "text", "label=`Search for Brodmann areas`" );
    mParams.addParam( "Levels", &level, "min=1 max=6 step=1 keyIncr=k keyDecr=K" );
    mParams.addParam( "Search", &mySearch );
    
    //PARTCILES
    for( int i=0; i<mNumParticles; i++ ){
        particleOrder.push_back(*new ParticleOrder);
        mParticleSystem.particles[i]->setId(i);
        mRadiuses[i] = mParticleSystem.particles[i]->radius;
        mParticleSystem.particles[i]->updateColors();
        particleOrder[i].id = i;
        particleOrder[i].myPosition = mParticleSystem.particles[i]->position;
        particleOrder[i].referencePositionForRegion = mParticleSystem.particles[i]->position;
        particleOrder[i].myColor = mParticleSystem.particles[i]->colors;
        particleOrder[i].level_1 = mParticleSystem.particles[i]->level_1;
        particleOrder[i].level_2 = mParticleSystem.particles[i]->level_2;
        particleOrder[i].level_3 = mParticleSystem.particles[i]->level_3;
        particleOrder[i].level_4 = mParticleSystem.particles[i]->level_4;
        particleOrder[i].Brodmann = mParticleSystem.particles[i]->Brodmann;
        particleOrder[i].sideBrodmann = mParticleSystem.particles[i]->sideBrodmann;
        allPositions[i] = particleOrder[i].myPosition;
        theColors[i] = mParticleSystem.particles[i]->colors;
        particleOrder[i].point = mParticleSystem.particles[i]->point;
        //cout<< " particle id: "<<particleOrder[i].id<<endl;
    }
    
    
    //rotate position
    for(int i = 0; i<mNumParticles; i++){
        mParticleSystem.particles[i]->rotate(1,0,0,-90.0f);
        particleOrder[i].myPosition = mParticleSystem.particles[i]->position;
        particleOrder[i].myColor = mParticleSystem.particles[i]->colors;
        mParticleSystem.particles[i]->rotate(0,1,0,-180.0f);
        particleOrder[i].myPosition = mParticleSystem.particles[i]->position;
        particleOrder[i].myColor = mParticleSystem.particles[i]->colors;
    }
    
    myTalairach.setup("../../../assets/talairach.jar");
    locBrain= Vec3f(0,0,0);
    cubeSize=3;
}

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::mouseDown( MouseEvent event )
{
    mMouseDownPos = event.getPos();
    //mMousePressed = true;
    mMouseOffset = Vec2f::zero();
    
    //    Vec2i mouseCenter = Vec2i(getWindowSize().x/2, getWindowSize().y/2);
    //    Vec2i checkFor = (mMouseDownPos*0.2)-(mouseCenter*0.2);
    //    locBrain = Vec3i(checkFor.x, checkFor.y, 0);
    //    myTalairach.getLabelsArroundCube(locBrain, cubeSize);
    //    myTalairach.getLabels(locBrain);
    //    myTalairach.getStructuralProbMap(locBrain);
    
}

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::mouseUp( MouseEvent event )
{
    mMousePressed = false;
    mMouseOffset = Vec2f::zero();
}

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::mouseMove( MouseEvent event )
{
    mMousePos = event.getPos();
}

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::mouseDrag( MouseEvent event )
{
    mouseMove( event );
    mMouseOffset = ( mMousePos - mMouseDownPos ) * 0.5f;
}

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::mouseWheel( MouseEvent event )
{
    float dWheel	= event.getWheelIncrement();
    //mRoom.adjustTimeMulti( dWheel );
    if(cameraDepth <= -1.0){
        cameraDepth+= dWheel;
    }else {
        cameraDepth = -2.0;
    }
}

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::keyDown( KeyEvent event )
{
    switch( event.getChar() ){
        case ' ':  mRoom.togglePower();
            break;
        case 'g': mRoom.toggleGravity();		break;
            //case 's': mSaveFrames = !mSaveFrames;	break;
        default:								break;
    }
    vector< string >  in_search = ci::split( mySearch, " ", false );
    switch( event.getCode() ){
            
        case KeyEvent::KEY_x:       dataCounter++; dataCounter= dataCounter%SIGNAL_POINTS;setDataPoints(dataCounter);               break;
        case KeyEvent::KEY_z:       dataCounter--; dataCounter= abs(dataCounter%SIGNAL_POINTS);setDataPoints(dataCounter);          break;
        case KeyEvent::KEY_s:       dataCounter++; dataCounter= dataCounter%SIGNAL_POINTS; setDataPoints(dataCounter);  setConnections();              break;
        case KeyEvent::KEY_a:       dataCounter--; dataCounter= abs(dataCounter%SIGNAL_POINTS);setDataPoints(dataCounter); setConnections();           break;
        case KeyEvent::KEY_r:       running =! running;                                 break;
        case KeyEvent::KEY_v:       colorAverage();                                     break;
            
        case KeyEvent::KEY_c:       setConnections();                                   break;
        case KeyEvent::KEY_m:       drawAllConnections =! drawAllConnections;           break;
        case KeyEvent::KEY_i:       readFile();                                         break;
        case KeyEvent::KEY_o:       getAreas();                                         break;
        case KeyEvent::KEY_p:       mShowParams =! mShowParams;                         break;
        case KeyEvent::KEY_7:       filterAverage();                                    break;
        case KeyEvent::KEY_8:       printResult();                                      break;
        case KeyEvent::KEY_9:       sumHistogram();                                     break;
        case KeyEvent::KEY_0:       getAllAreas();                                      break;
            
            //rotation
        case KeyEvent::KEY_UP:
            for(int i = 0; i<mNumParticles; i++){
                mParticleSystem.particles[i]->rotate(1,0,0,45.0f);
                particleOrder[i].myPosition = mParticleSystem.particles[i]->position;
                particleOrder[i].myColor = mParticleSystem.particles[i]->temporalColors;
                particleOrder[i].level_1 = mParticleSystem.particles[i]->level_1;
                particleOrder[i].level_2 = mParticleSystem.particles[i]->level_2;
                particleOrder[i].level_3 = mParticleSystem.particles[i]->level_3;
                particleOrder[i].level_4 = mParticleSystem.particles[i]->level_4;
                particleOrder[i].Brodmann = mParticleSystem.particles[i]->Brodmann;
                particleOrder[i].sideBrodmann = mParticleSystem.particles[i]->sideBrodmann;
                particleOrder[i].point = mParticleSystem.particles[i]->point;
            }
            break;
        case KeyEvent::KEY_DOWN:
            for(int i = 0; i<mNumParticles; i++){
                mParticleSystem.particles[i]->rotate(1,0,0,-45.0f);
                particleOrder[i].myPosition = mParticleSystem.particles[i]->position;
                particleOrder[i].myColor = mParticleSystem.particles[i]->temporalColors;
                particleOrder[i].level_1 = mParticleSystem.particles[i]->level_1;
                particleOrder[i].level_2 = mParticleSystem.particles[i]->level_2;
                particleOrder[i].level_3 = mParticleSystem.particles[i]->level_3;
                particleOrder[i].level_4 = mParticleSystem.particles[i]->level_4;
                particleOrder[i].Brodmann = mParticleSystem.particles[i]->Brodmann;
                particleOrder[i].sideBrodmann = mParticleSystem.particles[i]->sideBrodmann;
                particleOrder[i].point = mParticleSystem.particles[i]->point;
            }
            break;
        case KeyEvent::KEY_LEFT:
            for(int i = 0; i<mNumParticles; i++){
                mParticleSystem.particles[i]->rotate(0,1,0,45.0f);
                particleOrder[i].myPosition = mParticleSystem.particles[i]->position;
                particleOrder[i].myColor = mParticleSystem.particles[i]->temporalColors;
                particleOrder[i].level_1 = mParticleSystem.particles[i]->level_1;
                particleOrder[i].level_2 = mParticleSystem.particles[i]->level_2;
                particleOrder[i].level_3 = mParticleSystem.particles[i]->level_3;
                particleOrder[i].level_4 = mParticleSystem.particles[i]->level_4;
                particleOrder[i].Brodmann = mParticleSystem.particles[i]->Brodmann;
                particleOrder[i].sideBrodmann = mParticleSystem.particles[i]->sideBrodmann;
                particleOrder[i].point = mParticleSystem.particles[i]->point;
            }
            break;
        case KeyEvent::KEY_RIGHT:
            for(int i = 0; i<mNumParticles; i++){
                mParticleSystem.particles[i]->rotate(0,1,0,-45.0f);
                particleOrder[i].myPosition = mParticleSystem.particles[i]->position;
                particleOrder[i].myColor = mParticleSystem.particles[i]->temporalColors;
                particleOrder[i].level_1 = mParticleSystem.particles[i]->level_1;
                particleOrder[i].level_2 = mParticleSystem.particles[i]->level_2;
                particleOrder[i].level_3 = mParticleSystem.particles[i]->level_3;
                particleOrder[i].level_4 = mParticleSystem.particles[i]->level_4;
                particleOrder[i].Brodmann = mParticleSystem.particles[i]->Brodmann;
                particleOrder[i].sideBrodmann = mParticleSystem.particles[i]->sideBrodmann;
                particleOrder[i].point = mParticleSystem.particles[i]->point;
            }
            break;
        case KeyEvent::KEY_1:
            filterRegion("Frontal Lobe",2);
            break;
        case KeyEvent::KEY_2:
            filterRegion("Sub-lobar",2);
            break;
        case KeyEvent::KEY_3:
            filterRegion("Temporal Lobe",2);
            break;
        case KeyEvent::KEY_4:
            filterRegion("Occipital Lobe",2);
            break;
        case KeyEvent::KEY_5:
            filterRegion("Limbic Lobe",2);
            break;
        case KeyEvent::KEY_6:
            filterRegion("Parietal Lobe",2);
            break;
        case KeyEvent::KEY_RETURN:
            if(in_search.size() == 1){
                filterRegion(in_search[0], level);
            }
            else if(in_search.size() == 2 && in_search[0] != "b" && in_search[0] != "B"){
                filterRegion(in_search[0]+" "+in_search[1], level);
            } else {
                filterRegion("Brodmann area "+in_search[1], level);
            }
            if(in_search.size() == 3){
                filterRegion(in_search[0]+" "+in_search[1]+" "+in_search[2], level);
            }
            break;
        default: break;
    }
}

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::update()
{
    cameraPosition = mSpringCam.getEye();
    if( minDist > maxDist ) minDist = maxDist;
    for( int i=0; i<mNumParticles; i++ ){
        float d = cameraPosition.distance(particleOrder[i].myPosition);
        particleOrder[i].distance = d;
        theColors[i] = particleOrder[i].myColor;
        allPositions[i] = particleOrder[i].myPosition;
    }
    if(!mRoom.mIsPowerOn)sort(particleOrder.begin(), particleOrder.end(), sortByDistanceTwo);
    
    // ROOM
    mRoom.update( mSaveFrames );
    
    // CAMERA
    mSpringCam.dragCam( ( mMouseOffset ) * 0.02f, ( mMouseOffset ).length() * 0.02f );
    mSpringCam.update( 1.0f );
    mSpringCam.mEyeNode.mRestPos = Vec3f( 0.0f, 0.0f, cameraDepth );
    
    drawIntoRoomFbo();
    if(running) runSimulation();
    
    alpha = mRoom.getLightPower() + 0.2f;
    if(alpha < 1.0){
        for (int i=particleOrder.size()-1;i >= 0; i--){
            particleOrder[i].myColor.w = alpha * particleOrder[i].kind;
        }
    }
    //cout<<cameraPosition<<endl;
}

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::drawIntoRoomFbo()
{
    mRoomFbo.bindFramebuffer();
    gl::clear( ColorA( 0.0f, 0.0f, 0.0f, 0.0f ), true );
    
    gl::setMatricesWindow( mRoomFbo.getSize(), false );
    gl::setViewport( mRoomFbo.getBounds() );
    gl::disableAlphaBlending();
    gl::enable( GL_TEXTURE_2D );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    Matrix44f m;
    m.setToIdentity();
    m.scale( mRoom.getDims() );
    
    mRoomShader.bind();
    mRoomShader.uniform( "mvpMatrix", mSpringCam.mMvpMatrix );
    mRoomShader.uniform( "mMatrix", m );
    mRoomShader.uniform( "eyePos", mSpringCam.mEye );
    mRoomShader.uniform( "roomDims", mRoom.getDims() );
    mRoomShader.uniform( "power", mRoom.getPower() );
    mRoomShader.uniform( "lightPower", mRoom.getLightPower() ); //just for the ceiling
    mRoomShader.uniform( "timePer", mRoom.getTimePer() * 1.5f + 0.5f );
    mRoom.draw();
    mRoomShader.unbind();
    
    mRoomFbo.unbindFramebuffer();
    glDisable( GL_CULL_FACE );
}

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::draw()
{
    //gl::clear( ColorA( 0.0f, 0.0f, 0.0f, 0.0f ), true );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    // SET MATRICES TO WINDOW
    gl::setMatricesWindow( getWindowSize(), false );
    gl::setViewport( getWindowBounds() );
    
    gl::enable( GL_TEXTURE_2D );
    gl::enable(GL_ALPHA_TEST);
    
    if(mRoom.isPowerOn() == true){
        gl::color( ColorA( 0.0f, 0.0f, 0.0f, 0.5f ) );
        gl::drawSolidRect( getWindowBounds() );
        gl::enableAdditiveBlending();
        mRoomFbo.unbindTexture();
    } else {
        // DRAW ROOM FBO
        mRoomFbo.bindTexture();
        gl::color( ColorA( 1.0f, 0.99f, 0.95f, 1.0f ) );
        gl::drawSolidRect( getWindowBounds() );
        gl::enableAlphaBlending();
        mRoomFbo.bindTexture();
    }
    
    // SET MATRICES TO SPRING CAM
    gl::setMatrices( mSpringCam.getCam() );
    if(drawAllConnections)drawConnections();
    
    gl::color( ColorA( 1.0f, 1.0f, 1.0f, 1.0f ) );
    mShader.bind();
    //GLint pixel_loc = glGetAttribLocation(mShader.getHandle(), "thePixelSize");
    
    // store current OpenGL state
    glPushAttrib( GL_POINT_BIT | GL_ENABLE_BIT );
    gl::disableDepthRead();
    gl::disableDepthWrite();
    // enable point sprites and initialize it
    gl::enable( GL_POINT_SPRITE_ARB );
    glPointParameterfARB( GL_POINT_FADE_THRESHOLD_SIZE_ARB, -1.0f );
    glPointParameterfARB( GL_POINT_SIZE_MIN_ARB, 0.1f );
    glPointParameterfARB( GL_POINT_SIZE_MAX_ARB, 200.0f );
    
    // allow vertex shader to change point size
    gl::enable( GL_VERTEX_PROGRAM_POINT_SIZE );
    GLint thisColor = mShader.getAttribLocation( "myColor" );
    glEnableVertexAttribArray(thisColor);
    glVertexAttribPointer(thisColor,4,GL_FLOAT,true,0,theColors);
    GLint particleRadiusLocation = mShader.getAttribLocation( "particleRadius" );
    glEnableVertexAttribArray(particleRadiusLocation);
    glVertexAttribPointer(particleRadiusLocation, 1, GL_FLOAT, true, 0, mRadiuses);
    glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, allPositions);
    mTexture.enableAndBind();
    glDrawArrays( GL_POINTS, 0, mNumParticles );
    mTexture.unbind();
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableVertexAttribArrayARB(thisColor);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableVertexAttribArrayARB(particleRadiusLocation);
    glDisable(GL_DEPTH_TEST);
    
    // unbind shader
    mShader.unbind();
    
    // restore OpenGL state
    glPopAttrib();
    
    // DRAW INFO PANEL
    // drawInfoPanel();
    
    // SAVE FRAMES
    if( mSaveFrames && mNumSavedFrames < 10 ){
        writeImage( toString(getHomeDirectory()) + "Room/" + toString( mNumSavedFrames ) + ".png", copyWindowSurface() );
        mNumSavedFrames ++;
    }
    
    // DRAW PARAMS WINDOW
    if( mShowParams ){
        gl::setMatricesWindow( getWindowSize() );
        //params::InterfaceGl::draw();
        mParams.draw();
    }
    // DRAW TALAIRACH INFO
    //myTalairach.drawDebug(20, 200);
    
    //Draw text
    if(myText.size() >= 1){
        for(int i = 0; i < myText.size(); i++){
            myText[i].draw(mSpringCam.getCam());
        }
    }
}

//---------------------------------------------------------------------------------------------------------------

void BrainMc2App::drawInfoPanel()
{
    gl::pushMatrices();
    gl::translate( mRoom.getDims() );
    gl::scale( Vec3f( -1.0f, -1.0f, 1.0f ) );
    gl::color( Color( 1.0f, 1.0f, 1.0f ) * ( 1.0 - mRoom.getPower() ) );
    gl::enableAlphaBlending();
    
    float iconWidth		= 50.0f;
    
    float X0			= 15.0f;
    float X1			= X0 + iconWidth;
    float Y0			= 300.0f;
    float Y1			= Y0 + iconWidth;
    
    // DRAW ICON
    float c = mRoom.getPower() * 0.5f + 0.5f;
    gl::color( ColorA( c, c, c, 0.5f ) );
    gl::draw( mIconTex, Rectf( X0, Y0, X1, Y1 ) );
    
    c = mRoom.getPower();
    gl::color( ColorA( c, c, c, 0.5f ) );
    gl::disable( GL_TEXTURE_2D );
    
    // DRAW TIME BAR
    float timePer		= mRoom.getTimePer();
    gl::drawSolidRect( Rectf( Vec2f( X0, Y1 + 2.0f ), Vec2f( X0 + timePer * ( iconWidth ), Y1 + 2.0f + 4.0f ) ) );
    
    // DRAW FPS BAR
    float fpsPer		= getAverageFps()/60.0f;
    gl::drawSolidRect( Rectf( Vec2f( X0, Y1 + 4.0f + 4.0f ), Vec2f( X0 + fpsPer * ( iconWidth ), Y1 + 4.0f + 6.0f ) ) );
    
    gl::popMatrices();
}

CINDER_APP_BASIC( BrainMc2App, RendererGl )