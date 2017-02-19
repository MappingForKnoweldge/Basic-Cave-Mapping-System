#pragma once

#include "ofMain.h"
#include "ofxAndroid.h"

class ofApp : public ofxAndroidApp {

public:

    void setup();
    void update();
    void draw();
    void keyPressed(int key);
    void keyReleased(int key);
    void windowResized(int w, int h);
    void touchDown(int x, int y, int id);
    void touchMoved(int x, int y, int id);
    void touchUp(int x, int y, int id);
    void touchDoubleTap(int x, int y, int id);
    void touchCancelled(int x, int y, int id);
    void swipe(ofxAndroidSwipeDir swipeDir, int id);
    void pause();
    void stop();
    void resume();
    void reloadTextures();
    bool backPressed();
    void okPressed();
    void cancelPressed();

    void drawAxis();

    ofLight light; // creates a light and enables lighting
    ofEasyCam cam; // add mouse controls for camera movement

    vector<ofVec3f> nodes;
    vector<vector <ofVec3f> > pcls;
    int numPoints;

    float angleX, angleY, scale;
    float traX, traY;
    float x0, x1, dX;
    float y0, y1, dY;
};
