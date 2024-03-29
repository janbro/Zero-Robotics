//TODO: 

//Declare any variables shared between functions here
float myState[12],otherState[12],tempVec[3],tP1[3],targetEntryPoint[3],targetVel[3],targetAttRate[3],poi1[3],poi2[3],poi3[3],center[3];
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
    game.getPOILoc(poi3,2);
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
    game.getPOILoc(poi3,2);
    spherePoi = calcClosestPoi();
    time++;
    lastMem = game.getMemoryFilled();
}

void loop(){
	//This function is called once per second.  Use it to control the satellite.
	updateVariables();
	
	facePos(center); //Always face the center
	
	if((timeToFlare>2||timeToFlare==-1)&&!on){ //Turn sphere back on
	    game.turnOn();
	    on=true;
	}
	else if(timeToFlare!=-1&&timeToFlare<=2&&on){ //Auto Shutdown, doesn't let sphere be on during solar flare
	    game.turnOff();
	    on=false;
	}
	if(game.getMemoryFilled()<2&&poiZone<2&&(timeToFlare>12||timeToFlare==-1)&&time<230){ //If we have memory space and no incoming solar flare
    	switch(poiZone){
	        case 0: //Outer Ring
                calcPoiEntry(spherePoi,tP1,.4425f);//.430f
	            safeSetPosition(tP1,.2f);//.15
	            break;
    	    case 1: //Inner Ring
	            calcPoiEntry(spherePoi,tP1,.3725f);//.370f
	            safeSetPosition(tP1,.2f);//.15
	            break;
	    }
	    mathVecSubtract(tempVec,center,myState,3);
	    if(distance(myState,tP1)<.025f&&mathVecInner(tP1,tempVec,3)<.05f){ //If in range, take picture
	        game.takePic(spherePoi);
	        picTries++;
	        if(game.getMemoryFilled()>lastMem||picTries>7){ //Successfully taken picture or stuck taking pic, go to next poi
	            poiZone++;
	        }
	    }
	}else{ //Solar Flare incoming
	    if(game.getMemoryFilled()>0){ //Upload pictures in memory
	        //float memPack[] = {-.5,sphere*.6,0};
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

int calcClosestPoi(){
    float dist1 = distance(poi1,myState);
    float dist2 = distance(poi2,myState);
    float dist3 = distance(poi3,myState);
    if(dist1<dist2&&dist1<dist3)
        return 0;
    else if(dist2<dist1&&dist2<dist3)
        return 1;
    else
        return 2;
}

bool insidePoiZone(int poi){
    if(poi==0){
        mathVecSubtract(tempVec,myState,tP1,3);
        if(mathVecMagnitude(myState,3)>.31&&mathVecMagnitude(myState,3)<.42&&mathVecInner(tempVec,tP1,3)<.8){
            return true;
        }
    }else if(poi==1){
        mathVecSubtract(tempVec,myState,tP1,3);
        if(mathVecMagnitude(myState,3)>.42&&mathVecMagnitude(myState,3)<.53&&mathVecInner(tempVec,tP1,3)<.4){
            return true;
        }
    }else
        return false;
}

void calcPoiEntry(int poiID, float poi[3],float radius){
    game.getPOILoc(poi,poiID);
    float x = poi[0];
    poi[0] = radius*x/(sqrtf(powf(x,2)+powf(poi[1],2)+powf(poi[2],2)));
    poi[1] = poi[0]*poi[1]/x;
    poi[2] = poi[0]*poi[2]/x;
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
	return distance(attTarget,&myState[6]);
}

void safeSetPosition(float tP[3],float speed){
    if(intersectsAsteroid(tP)){
        doOrbit(tP,speed);
    }
    else{
        setPos(tP,speed);
    }
}

void rotate(float result[3],float vector[3],float rotation[3]){
    result[0] = vector[0]*cosf(rotation[1])*cosf(rotation[2])+vector[1]*(cosf(rotation[2])*sinf(rotation[0])*sinf(rotation[1])-cosf(rotation[0])*sinf(rotation[2]))+vector[2]*(cosf(rotation[0])*cosf(rotation[2])*sinf(rotation[1])+sinf(rotation[0])*sinf(rotation[2]));
    result[1] = vector[0]*cosf(rotation[1])*sinf(rotation[2])+vector[1]*(cosf(rotation[0])*cosf(rotation[2])+sinf(rotation[0])*sinf(rotation[1])*sinf(rotation[2]))-vector[2]*(cosf(rotation[2])*sinf(rotation[0])+cosf(rotation[0])+sinf(rotation[1])*sinf(rotation[2]));
    result[2] = vector[0]*-sinf(rotation[1])+vector[1]*cosf(rotation[1])*sinf(rotation[0])+vector[2]*cosf(rotation[0])*cosf(rotation[1]);
}

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
	//Rotate to 3D
	mathVecAdd(tempVec,myState,otherState,3);
	mathVecNormalize(tempVec,3);
	float xAxis[3] = {1.0f,0.0f,0.0f};
	float yAxis[3] = {0.0f,1.0f,0.0f};
	float zAxis[3] = {0.0f,0.0f,1.0f};
	float rots[3] = {mathVecInner(tempVec,xAxis,3),mathVecInner(tempVec,yAxis,3),mathVecInner(tempVec,zAxis,3)};
	rotate(targetPos,targetPos,rots);
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

bool intersectsAsteroid(float targetPos[3]){
    float dir[3];
    mathVecSubtract(dir,myState,targetPos,3);
    float center[] = {0.0f,0.0f,0.0f};
    float r = .31f; //Radius of danger zone
    mathVecSubtract(tempVec,myState,center,3);
    float res = powf(mathVecInner(dir,tempVec,3),2)-powf(mathVecMagnitude(tempVec,3),2)+powf(r,2);
    //Once we've found no intersections, can determine to orbit or not. Rest is to reinforce concept
    mathVecSubtract(tempVec,myState,targetPos,3);

    if(res<0){ //Does not intersect
        return false;
    }else if(res==0){ //Intersects at one point
        return true;
    }else{//if res>0  //Intersects at two points
        return true;
    }
}

bool willCollide(int timeInFuture){
    //Calculate future other position(velocity*time+currentState)
    float futurePos[3];
    for(int i=0;i<3;i++){
        futurePos[i] = otherState[i+3]*timeInFuture+otherState[i];
    }
    //Check magnitude between otherFutureState and current State within collision sphere
    mathVecSubtract(tempVec,futurePos,myState,3);
    if(mathVecMagnitude(tempVec,3)<.15f)
        return true;
    return false;
}

