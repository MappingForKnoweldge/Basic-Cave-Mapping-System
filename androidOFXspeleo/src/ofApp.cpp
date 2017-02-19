#include "ofApp.h"
#include <jni.h>

float yawC, rollC, pitchC, distC, dist1, rssiC;
int rotC, scaC, traC;
int mesC, secC, clrC;
string msg;

extern "C" {
	void Java_cc_openframeworks_androidOFXspeleo_OFActivity_callOF (JNIEnv* env, jobject thiz, jfloat rssiJ, jfloat yawJ, jfloat rollJ, jfloat pitchJ, jfloat distJ) {
		rssiC = rssiJ;
		yawC = yawJ;
		rollC = rollJ;
		pitchC = pitchJ;
		distC = distJ;
	}
}

extern "C" {
	void Java_cc_openframeworks_androidOFXspeleo_OFActivity_callOFRotScaTra (JNIEnv* env, jobject thiz, jint rotJ, jint scaJ, jint traJ) {
		rotC = rotJ;
		scaC = scaJ;
		traC = traJ;
	}
}

extern "C" {
    void Java_cc_openframeworks_androidOFXspeleo_OFActivity_callOFmeasure (JNIEnv* env, jobject thiz, jint mesJ) {
        mesC = mesJ;
    }
}

extern "C" {
    void Java_cc_openframeworks_androidOFXspeleo_OFActivity_callOFsection (JNIEnv* env, jobject thiz, jint secJ) {
        secC = secJ;
    }
}

extern "C" {
    void Java_cc_openframeworks_androidOFXspeleo_OFActivity_callOFclear (JNIEnv* env, jobject thiz, jint clrJ) {
        clrC = clrJ;
    }
}


//--------------------------------------------------------------
void ofApp::setup(){
	//ofSetLogLevel(OF_LOG_VERBOSE);

	//ofEnableAlphaBlending();
	ofSetVerticalSync(false);
	//ofEnableSmoothing();

	ofSetLineWidth(2);

	nodes.push_back(ofVec3f(0.0, 0.0, 0.0));    // aggiungo un punto al vettore dei nodi
	pcls.push_back(vector<ofVec3f>());          // aggiungo un vettore vuoto al vettore delle nuvole

	nodes.push_back(ofVec3f(0.0, 0.0, 0.0));
	pcls.push_back(vector<ofVec3f>());

	numPoints = 2;

	angleX = angleY = 0;
    x0 = x1 = dX = 0;
	y0 = y1 = dY = 0;

	distC = -1;
    dist1 = 1;

	rotC = 1;
	scaC = 0;
	traC = 0;

    secC = mesC = 0;
    clrC = 0;

    traX = 0;
    traY = 0;

	angleX = 0;
	angleY = 0;

	scale = 1;

	/*jclass classID = ofxJavaGetClassID("cc/openframeworks/androidOFXspeleo/OFActivity");
	jfieldID fieldID = ofxJavaGetStaticFieldID(classID, "valJ", "F");
	jobject obj = ofGetJNIEnv()->GetStaticObjectField(classID, fieldID);
	valC = ofGetJNIEnv()->GetStaticFloatField(classID, fieldID);*/
}

//--------------------------------------------------------------
void ofApp::update(){
	dX = (x1 - x0)/(float)ofGetWidth();
	dY = (y1 - y0)/(float)ofGetHeight();
    x0 = x1;
	y0 = y1;

	if (traC == 1) {
    	traX += dX;
    	traY += dY;
    }

	if (rotC == 1) {
		angleX -= (360 * dX);
		angleY -= (360 * dY);
	}

	if (scaC == 1) {
		scale -= (10 * dY);
		if (scale < 0.1) {
			scale = 0.1;
		}
	}

    if (clrC == 1) {
        nodes.clear();
		pcls.clear();

		nodes.push_back(ofVec3f(0.0, 0.0, 0.0));
		pcls.push_back(vector<ofVec3f>());

		nodes.push_back(ofVec3f(0.0, 0.0, 0.0));
		pcls.push_back(vector<ofVec3f>());

        numPoints = 2;

        clrC = 0;
    }

	if (distC != -1) {
        if (mesC == 1) {
            dist1 = distC;

			// nuovo punto misurato
            nodes[numPoints - 1].x = nodes[numPoints - 2].x + 30.0 * dist1 * sin(ofDegToRad(90 - pitchC)) * cos(ofDegToRad(yawC));
            nodes[numPoints - 1].y = nodes[numPoints - 2].y + 30.0 * dist1 * sin(ofDegToRad(90 - pitchC)) * sin(ofDegToRad(yawC));
            nodes[numPoints - 1].z = nodes[numPoints - 2].z + 30.0 * dist1 * cos(ofDegToRad(90 - pitchC));

			// ultimo punto per direzione
			nodes.push_back(ofVec3f(0.0, 0.0, 0.0));
			pcls.push_back(vector<ofVec3f>());

            numPoints++;

            mesC = 0;
        } else if(secC == 1) {
            // cloudPoint section
            dist1 = distC;

			// nuovo punto per nuvola
            ofVec3f pnt;
			pnt.x = nodes[numPoints - 2].x + 30.0 * dist1 * sin(ofDegToRad(90 - pitchC)) * cos(ofDegToRad(yawC));
			pnt.y = nodes[numPoints - 2].y + 30.0 * dist1 * sin(ofDegToRad(90 - pitchC)) * sin(ofDegToRad(yawC));
			pnt.z = nodes[numPoints - 2].z + 30.0 * dist1 * cos(ofDegToRad(90 - pitchC));

			pcls[numPoints - 2].push_back(pnt);

            secC = 0;
        }

        distC = -1;
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackground(0);
	ofSetColor(255);

	ofEnableDepthTest();

	ofPushMatrix();

	ofTranslate(ofGetWidth() * (0.5 + traX), ofGetHeight() * (0.5  + traY), 0);

	ofRotateX(angleY);
	ofRotateZ(angleX);

	ofScale(scale, scale, scale);

	drawAxis();

	// ultimo punto per direzione
    nodes[numPoints - 1].x = nodes[numPoints - 2].x + 30.0 * sin(ofDegToRad(90 - pitchC)) * cos(ofDegToRad(yawC));
    nodes[numPoints - 1].y = nodes[numPoints - 2].y + 30.0 * sin(ofDegToRad(90 - pitchC)) * sin(ofDegToRad(yawC));
    nodes[numPoints - 1].z = nodes[numPoints - 2].z + 30.0 * cos(ofDegToRad(90 - pitchC));

	ofSetColor(50, 100, 200);
	ofFill();
    ofDrawIcoSphere(nodes[0].x, nodes[0].y, nodes[0].z, 2);
	ofSetColor(200);
	ofNoFill();
    //ofDrawIcoSphere(nodes[0].x, nodes[0].y, nodes[0].z, 2);

    // 3D
	for (int i = 1; i < numPoints; i++) {
		if (i == numPoints - 1) {
			ofSetColor(200, 100, 50);   // ultimo punto
		} else {
			ofSetColor(50, 100, 200);   // gli altri punti
		}
		ofFill();
		ofDrawIcoSphere(nodes[i].x, nodes[i].y, nodes[i].z, 2);
		ofSetColor(200);
		ofNoFill();
		ofDrawLine(nodes[i - 1].x, nodes[i - 1].y, nodes[i - 1].z, nodes[i].x, nodes[i].y, nodes[i].z);

		// nuvola di punti intorno al punto i
		if (pcls[i].size() > 0) {
			int npt = pcls[i].size();
			for (int j = 0; j < npt; j++) {
                ofSetColor(200);
                ofFill();
                ofDrawIcoSphere(pcls[i][j].x, pcls[i][j].y, pcls[i][j].z, 2);
                ofSetColor(100);
                ofNoFill();
                //ofDrawLine(nodes[i].x, nodes[i].y, nodes[i].z, pcls[i][j].x, pcls[i][j].y, pcls[i][j].z);
			}
		}
	}

    // proiezione piano Z = 0
    for (int i = 1; i < (numPoints - 1); i++) {
        if (i == numPoints - 1) {
            //ofSetColor(200, 100, 50);   // ultimo punto
        } else {
            ofSetColor(50, 100, 200);   // gli altri punti
        }
        ofFill();
        ofDrawIcoSphere(nodes[i].x, nodes[i].y, 0, 2);
        ofSetColor(50, 100, 200);
        ofNoFill();
        ofDrawLine(nodes[i - 1].x, nodes[i - 1].y, 0, nodes[i].x, nodes[i].y, 0);

        // nuvola di punti intorno al punto i
        if (pcls[i].size() > 0) {
            int npt = pcls[i].size();
            for (int j = 0; j < npt; j++) {
                ofSetColor(50, 100, 200);
                ofFill();
                ofDrawIcoSphere(pcls[i][j].x, pcls[i][j].y, 0, 2);
                ofSetColor(100);
                ofNoFill();
                //ofDrawLine(nodes[i].x, nodes[i].y, 0, pcls[i][j].x, pcls[i][j].y, 0);
            }
        }
    }

    // proiezione piano Y = 0
    for (int i = 1; i < (numPoints - 1); i++) {
        if (i == numPoints - 1) {
            //ofSetColor(200, 100, 50);   // ultimo punto
        } else {
            ofSetColor(200, 100, 50);   // gli altri punti
        }
        ofFill();
        ofDrawIcoSphere(nodes[i].x, 0, nodes[i].z, 2);
        ofSetColor(200, 100, 50);
        ofNoFill();
        ofDrawLine(nodes[i - 1].x, 0, nodes[i - 1].z, nodes[i].x, 0, nodes[i].z);

        // nuvola di punti intorno al punto i
        if (pcls[i].size() > 0) {
            int npt = pcls[i].size();
            for (int j = 0; j < npt; j++) {
                ofSetColor(200, 100, 50);
                ofFill();
                ofDrawIcoSphere(pcls[i][j].x, 0, pcls[i][j].z, 2);
                ofSetColor(100);
                ofNoFill();
                //ofDrawLine(nodes[i].x, 0, nodes[i].z, pcls[i][j].x, 0, pcls[i][j].z);
            }
        }
    }

	ofPopMatrix();

	ofDisableDepthTest();

	ofPushMatrix();
	ofScale(2, 2, 1);
	ofSetColor(255);
	//msg = "\nfps: " + ofToString(ofGetFrameRate(), 2);
	//ofDrawBitmapString(msg, 10, 10);
	msg = "\nRSSI: " + ofToString(rssiC, 2);
	ofDrawBitmapString(msg, 10, 10);
	msg = "\nyaw: " + ofToString(yawC, 2);
	ofDrawBitmapString(msg, 10, 22);
	msg = "\nroll: " + ofToString(rollC, 2);
	ofDrawBitmapString(msg, 10, 34);
	msg = "\npitch: " + ofToString(pitchC, 2);
	ofDrawBitmapString(msg, 10, 46);
	msg = "\ndist: " + ofToString(dist1, 2);
	ofDrawBitmapString(msg, 10, 58);
	ofPopMatrix();
}

void ofApp::drawAxis(){
	ofPushMatrix();
	ofSetLineWidth(4);
	ofSetColor(200, 0, 0);
	ofLine(0, 0, 0, 120, 0, 0);
	ofSetColor(0, 200, 0);
	ofLine(0, 0, 0, 0, 120, 0);
	ofSetColor(0, 0, 200);
	ofLine(0, 0, 0, 0, 0, 120);
	ofRotateY(90);
	ofSetColor(100);
	ofSetLineWidth(2);
	ofDrawGridPlane(30.0);
	ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key){ 
	
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){ 
	
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::touchDown(int x, int y, int id){
	x0 = x1 = x;
	y0 = y1 = y;
}

//--------------------------------------------------------------
void ofApp::touchMoved(int x, int y, int id){
	x1 = x;
	y1 = y;
}

//--------------------------------------------------------------
void ofApp::touchUp(int x, int y, int id){
	x0 = x1 = dX = 0;
	y0 = y1 = dY = 0;
}

//--------------------------------------------------------------
void ofApp::touchDoubleTap(int x, int y, int id){
    traX = traY = 0;
	angleX = x0 = x1 = dX = 0;
	angleY = y0 = y1 = dY = 0;
	scale = 1;
}

//--------------------------------------------------------------
void ofApp::touchCancelled(int x, int y, int id){

}

//--------------------------------------------------------------
void ofApp::swipe(ofxAndroidSwipeDir swipeDir, int id){

}

//--------------------------------------------------------------
void ofApp::pause(){

}

//--------------------------------------------------------------
void ofApp::stop(){

}

//--------------------------------------------------------------
void ofApp::resume(){

}

//--------------------------------------------------------------
void ofApp::reloadTextures(){

}

//--------------------------------------------------------------
bool ofApp::backPressed(){
	return false;
}

//--------------------------------------------------------------
void ofApp::okPressed(){

}

//--------------------------------------------------------------
void ofApp::cancelPressed(){

}
