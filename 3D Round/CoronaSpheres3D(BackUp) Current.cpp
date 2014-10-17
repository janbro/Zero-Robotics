//TODO: Add go to second poi in one 60 second interval
//      Fix asteroid line collision detection(orbit disabled for now)
//      Take pics in outer poizone of two poi if close together
//      Add poi change decision based on distance from new poi and time left in 60 second interval

//Declare any variables shared between functions here
float myState[12],otherState[12],tempVec[3],tP1[3],targetEntryPoint[3],targetVel[3],targetAttRate[3],poi1[3],poi2[3],poi3[3],center[3];
float mid[3];
bool memFull,on;
int sphere,spherePoi,timeToFlare,time,poiZone,lastMem,picTries,poi[3],picTaken;

void init(){
	//This function is called once when your code is first loaded.

	//IMPORTANT: make sure to set any variables that need an initial value.
	//Do not assume variables will be set to 0 automatically!
	on=true;
	time=center[0]=center[1]=center[2]=lastMem=picTries=poiZone=picTaken=0;
	poi[0]=poi[1]=poi[2]=2;
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
    if(time%60==0){
        poi[0] = poi[1] = poi[2] = 2;
    }if(time<60)
        poi[2] = 0;
    game.getPOILoc(poi1,0);
    game.getPOILoc(poi2,1);
    game.getPOILoc(poi3,2);
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
	if(game.getMemoryFilled()<2&&poiZone<2&&(timeToFlare>12||timeToFlare==-1)&&time<230){ //If we have memory space and no incoming solar flare;
    	switch(poiZone){
	        case 0: //Outer Ring
			    spherePoi = calcClosestPoi();
                calcPoiEntry(spherePoi,tP1,.41f);//.430f
	            safeSetPosition(tP1,.175f);//.15
	            break;
    	    case 1: //Inner Ring
            	spherePoi = calcClosestPoi();
	            calcPoiEntry(spherePoi,tP1,.36f);//.370f
	            safeSetPosition(tP1,.175f);//.15
	            break;
	    }
	    mathVecSubtract(tempVec,center,myState,3);
	    if(game.alignLine(spherePoi)&&distance(myState,tP1)<.025f){//&&mathVecInner(tP1,tempVec,3)<.05f){ //If in range, take picture
	        game.takePic(spherePoi);
	        picTries++;
	        if(game.getMemoryFilled()>lastMem||picTries>5){ //Successfully taken picture or stuck taking pic, go to next poi
	            poiZone++;
	        }
	    }
	}else{
	    if(game.getMemoryFilled()>0){ //Upload pictures in memory
	        //float memPack[] = {-.5,sphere*.6,0};
          calcPoiEntry(spherePoi,tP1,.6);
	        safeSetPosition(tP1,.2f);
	        game.uploadPic();
    	    if(distance(myState,center)>.5&&time-picTaken>3){
	            game.uploadPic();
	            picTries=0;
	            if(time>60)
					poi[spherePoi]-=game.getMemoryFilled();
	            poiZone=0;
	            picTaken = time;
	        }
    	}else{ //Stop
    	    api.setVelocityTarget(center);
    	    game.takePic(spherePoi);
    	}
	}
}

int calcClosestPoi(){
    float dist1 = 0;
    if(poi[0]==1)
        dist1 = distance(poi1,myState);
    else
        dist1 = 10000.0f;
    float dist2 = 0;
    if(poi[1]==1)
        dist2 = distance(poi2,myState);
    else
        dist2 = 10000.0f;
    float dist3 = 0;
    if(poi[2]==1)
        dist3 = distance(poi3,myState);
    else
        dist3 = 10000.0f;
    if(dist3<dist1&&dist3<dist2)
        return 2;
    else if(dist2<dist1&&dist2<dist3)
        return 1;
    else
        return 0;
}

bool insidePoiZone(int poiZ){
    if(poiZ==0){
        mathVecSubtract(tempVec,myState,tP1,3);
        if(mathVecMagnitude(myState,3)>.31&&mathVecMagnitude(myState,3)<.42&&mathVecInner(tempVec,tP1,3)<.1){
            return true;
        }
    }else if(poiZ==1){
        mathVecSubtract(tempVec,myState,tP1,3);
        if(mathVecMagnitude(myState,3)>.42&&mathVecMagnitude(myState,3)<.53&&mathVecInner(tempVec,tP1,3)<.1){
            return true;
        }
    }
    else
        return false;
}

void calcPoiEntry(int poiID, float poi[3],float radius){
    game.getPOILoc(poi,poiID);
    float x = poi[0];
    poi[0] = radius*x/(sqrtf(powf(x,2)+powf(poi[1],2)+powf(poi[2],2)));
    if(x==0)
        x=1;
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
        DEBUG(("ORBIT!"));
        doOrbit(tP,speed);
    }
    else{
        DEBUG(("GOTO"));
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

	if(velocityTarget==NULL)
        return;
	api.setVelocityTarget(velocityTarget);
}

bool intersectsAsteroid(float targetPos[3]){
    /*float dir[3];
    mathVecSubtract(dir,myState,targetPos,3);
    float center[] = {0.0f,0.0f,0.0f};
    float r = .32f; //Radius of danger zone
    mathVecSubtract(tempVec,myState,center,3);
    float res = powf(mathVecInner(dir,tempVec,3),2)-powf(mathVecMagnitude(tempVec,3),2)+powf(r,2);
    //Once we've found no intersections, can determine to orbit or not. Rest is to reinforce concept
    mathVecSubtract(tempVec,myState,targetPos,3);
    DEBUG(("\nRes:%f",res));
    if(res<0){ //Does not intersect
        return false;
    }else if(res==0){ //Intersects at one point
        return true;
    }else{//if res>0  //Intersects at two points
        return true;
    }*/
    float diffVec[3];
    mathVecSubtract(diffVec,targetPos,myState,3);
    float dist = mathVecMagnitude(diffVec,3);
    float lineToPoint[3];
    mathVecSubtract(lineToPoint,center,myState,3);
    float dot = mathVecInner(diffVec,lineToPoint,3);
    float percOnLine = dot/dist;
    if(percOnLine<0.0f)
        percOnLine=0.0f;
    else if(percOnLine>1.0f)
        percOnLine = 1.0f;
    float intersectionPoint[3];
    mathVecSubtract(tempVec,targetPos,myState,3);
    scalarMult(tempVec,percOnLine,3);
    mathVecAdd(tempVec,myState,tempVec,3);
    mathVecSubtract(tempVec,tempVec,center,3);
    float len = mathVecMagnitude(tempVec,3);
    DEBUG(("\nlen%f",len));
    return !len>=.31f;
}

void scalarMult(float result[3],float mult,int len){
    for(int i=0;i<len;i++)
        result[i]=result[i]*mult;
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
