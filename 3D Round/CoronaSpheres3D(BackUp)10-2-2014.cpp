//TODO: Picture taking bug(stuck at poi)???

//Declare any variables shared between functions here
float myState[12],otherState[12],tempVec[3],tP1[3],targetEntryPoint[3],targetPos[3],targetVel[3],targetAttRate[3],poi1[3],poi2[3],center[3];
float mid[3];
bool memFull,on;
int sphere,spherePoi,timeToFlare,time,poiZone,lastMem,picTries;

void init(){
	//This function is called once when your code is first loaded.

	//IMPORTANT: make sure to set any variables that need an initial value.
	//Do not assume variables will be set to 0 automatically!
	on=true;
	time=center[0]=center[1]=center[2]=lastMem=picTries=poiZone=0;
	api.getMyZRState(myState);
	api.getOtherZRState(otherState);
	sphere=myState[1]>0.0?1:-1;//Blue Sphere = 1
	spherePoi=sphere>0?1:0;
    game.getPOILoc(poi1,0);
    game.getPOILoc(poi2,1);
}

void updateVariables(){
	api.getMyZRState(myState);
	api.getOtherZRState(otherState);
    timeToFlare = game.getNextFlare();
    if((picTries<5&&game.getMemoryFilled()==0)||time%60==0){
        lastMem = 0;
        poiZone = 0;
        if(game.getMemoryFilled()>0)
            poiZone = 2;
    }
    game.getPOILoc(poi1,0);
    game.getPOILoc(poi2,1);
    if(distance(poi1,&myState[0])<distance(poi2,&myState[0]))
        spherePoi=0;
    else
        spherePoi=1;
    time++;
    lastMem = game.getMemoryFilled();
}

void loop(){
	//This function is called once per second.  Use it to control the satellite.
	updateVariables();
	
	facePos(center); //Always face the center
	//DEBUG(("Flare: %i",timeToFlare));
	//DEBUG(("Mem:%i",game.getMemoryFilled()));
	//DEBUG(("SpherePOI:%i",spherePoi));
	//DEBUG(("PicTries:%i",picTries));
	//DEBUG(("PicZone:%i",poiZone));
	
	if((timeToFlare>2||timeToFlare==-1)&&!on){ //Turn sphere back on
	    //DEBUG(("TURN ON"));
	    game.turnOn();
	    on=true;
	}
	else if(timeToFlare!=-1&&timeToFlare<=2&&on){ //Auto Shutdown, doesn't let sphere be on during solar flare
	    //DEBUG(("TURN OFF"));
	    game.turnOff();
	    on=false;
	}
	if(game.getMemoryFilled()<2&&poiZone<2&&(timeToFlare>12||timeToFlare==-1)&&time<230){ //If we have memory space and no incoming solar flare
    	switch(poiZone){
	        case 0: //Outer Ring
                calcPoiEntry(spherePoi,tP1,.4425f);//.430f
	            safeSetPosition(tP1,.15f);//api.setPositionTarget(tP1);
	            break;
    	    case 1: //Inner Ring
	            calcPoiEntry(spherePoi,tP1,.3725f);//.370f
	            safeSetPosition(tP1,.15f);//api.setPositionTarget(tP1);
	            break;
	    }
	    mathVecSubtract(tempVec,center,myState,3);
	    if(distance(myState,tP1)<.025f&&mathVecInner(tP1,tempVec,3)<.05f){ //If in range, take picture
	        //DEBUG(("Take Pic!"));
	        game.takePic(spherePoi);
	        picTries++;
	        if(game.getMemoryFilled()>lastMem||picTries>7){ //Successfully taken picture or stuck taking pic, go to next poi
	            poiZone++;
	        }
	    }
	}else{ //Solar Flare incoming
	    //DEBUG(("HERE:%i",game.getMemoryFilled()));
	    if(game.getMemoryFilled()>0){ //Upload pictures in memory
	        float memPack[] = {-.5,sphere*.6,0};
            calcPoiEntry(spherePoi,tP1,.7);
	        safeSetPosition(tP1,.15f);
	        game.uploadPic();
    	    if(distance(myState,center)>.5){
	            game.uploadPic();
	            picTries=0;
	            poiZone=0;
	        }
    	}else{ //Stop
    	    api.setVelocityTarget(center);
    	    game.takePic(spherePoi);
    	}
	}
}

void calcPoiEntry(int poiID, float poi[3],float radius){
    game.getPOILoc(poi,poiID);
    float x = poi[0];
    poi[0] = radius*x/(sqrtf(powf(x,2)+powf(poi[1],2)+powf(poi[2],2)));
    poi[1] = poi[0]*poi[1]/x;
    poi[2] = poi[0]*poi[2]/x;
    DEBUG(("\n(%f,%f,%f)",poi[0],poi[1],poi[2]));   
}

float distance(float startLoc[3],float targetLoc[3]){ //Distance between two 3d vectors
	float tempVec[3];
	mathVecSubtract(tempVec,targetLoc,startLoc,3);
	return mathVecMagnitude(tempVec,3);
}

float facePos(float target[3]){ //Points sphere to passed target
	//Rotate to target
	float attTarget[3];
	mathVecSubtract(attTarget,target,myState,3);
	mathVecNormalize(attTarget,3);
	api.setAttitudeTarget(attTarget);
	//DEBUG(("\n%f",distance(attTarget,&myState[6])));
	return distance(attTarget,&myState[6]);
}

void safeSetPosition(float tP[3],float speed){
    float rad2 = 0.1764;
    float dr2 = powf(tP[0]-myState[0],2)+powf(tP[1]-myState[1],2);
    float D = myState[0]*tP[1]-tP[0]*myState[1];
    float delta = rad2*dr2-powf(D,2);
    /*if(delta>=0){
        DEBUG(("Orbit!"));
        doOrbit(tP,speed);
    }
    else{*/
        DEBUG(("\nGo directly to!"));
        setPos(tP,speed);
    }
//}

void doOrbit(float targetPos[3],float speed){
    float angle = 0.0f;
    float radius = mathVecMagnitude(myState,3);
    //Safety
    if(radius>.6f)
        radius = .6f;
    //Calculate Current Angle
    angle = acosf(myState[0]/(sqrtf(powf(myState[0],2)+powf(myState[1],2)+powf(myState[2],2))));
    if(myState[1]<0){
        angle=2*PI-angle;
    }
    mathVecSubtract(tempVec,targetPos,myState,3);
	if(tempVec[0]==0)
	    tempVec[0]=-1;
	if(tempVec[1]==0)
	    tempVec[1]=-1;
	//Calc Orbit Direction
	if(fabsf(tempVec[1])/tempVec[1]*fabsf(tempVec[0])/tempVec[0]>0){
	    angle-=(20*PI/180);//clockwise
	}
	else{
	    angle+=(20*PI/180);
	}
	//Calculate position on orbit circle
    targetPos[0] = radius * cosf(angle);
	targetPos[1] = radius * sinf(angle);
	targetPos[2] = 0.0f;
	setPos(targetPos,speed);
}

void setPos(float targetPos[3],float speed)
{
	float velocityTarget[3];
	mathVecSubtract(tempVec,targetPos,myState,3);
	float distance = mathVecMagnitude(tempVec,3);
	
	float speedTarget = distance*speed-distance*distance*0.1f;
	if((speedTarget>.055f || distance>.3f))
		speedTarget = .055f;
	
	//sets the target velocity using the speed target and position target
	for (int i = 0; i < 3; i++)
		velocityTarget[i] = speedTarget*(targetPos[i]-myState[i])/distance;
	api.setVelocityTarget(velocityTarget);
}
