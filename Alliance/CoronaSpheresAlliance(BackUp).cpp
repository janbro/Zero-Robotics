//TODO: MAKE A WORKING ORBIT FUNCTION
//      Change poi storing method pois[9] all poi
ZRState myState,otherState;
char state;
float tempVec[3],center[3],pois[9],Earth[3];
int sphere,spherePoi,poi[3],flareFlag;
static const unsigned short WARNING = 30;
unsigned int time,memCapacity;
int memFilled;

void init(){
	state = 'M';
	center[0]=center[1]=center[2]=time=Earth[1]=Earth[2]=0.0f;
	Earth[0]=.64f;
	poi[0]=poi[1]=poi[2]=2;
	sphere=myState[1]>0.0?1:-1;//Blue Sphere = 1
	spherePoi=sphere>0?1:0;
}

void updateVariables(){
    api.getMyZRState(myState);
    api.getOtherZRState(otherState);
    game.getPOILoc(pois,0);
    game.getPOILoc(&pois[3],1);
    game.getPOILoc(&pois[6],2);
    memFilled = game.getMemoryFilled();
    memCapacity = game.getMemorySize();
    flareFlag = game.getNextFlare();
    spherePoi = calcClosestPoi(true,true,true);
    if(time%60==0)
        poi[0]=poi[1]=poi[2]=2;
    time++;
}

void loop(){
	//This function is called once per second.  Use it to control the satellite.
	updateVariables();
	
    float idlePicSpot[3] = {0.1f,0.0f,-0.45f};
	
	if(flareFlag<=WARNING&&flareFlag!=-1||game.getFuelRemaining()<15){
	    state = 'F'; //Flare
	}else if(willCollide(5)){
	    state = 'C'; //Collision
	}
	//else if(!game.hasMemoryPack(0, 1)){
	//    state = 'M';
	//}
	else if(memFilled==memCapacity){
	    state = 'U'; //Upload
	}
	else if(memFilled<memCapacity){
	    state = 'T'; //Take picture
	}
	
	
	switch(state){
	    case 'C':
	        setPos(otherState,.3f);
	        break;
	    case 'M': //Grab mempack
    	    //pickUpItem(1);
	        break;
	    case 'F': //Go to shadow zone
    	    //Go to shadow zone position(need orbit function) and attempt upload while on the way
        	if(memFilled>0){ //If we have pictures, attempt to upload them
        	    if(facePos(Earth)<.2){
        	        game.uploadPic();
        	    }
        	}else{
        	    tempVec[0]=0.0f;tempVec[1]=0.0f;tempVec[2]=1.0f;
        	    mathVecAdd(tempVec,tempVec,myState,3);
        	    facePos(tempVec);
        	}
        	if((myState[0]<0.0f)){
        	    setPos(idlePicSpot,.25f);
            }else{
        	    float idleShadowZone[3] = {0.35f,0.0f,-0.125f};
        	    if(flareFlag!=-1&&flareFlag<10)
            	    setPos(idleShadowZone,.4f);
        	    else
                	setPos(idleShadowZone,.25f);
            }
            break;
        case 'U': //Upload pictures
    	    api.setPositionTarget(idlePicSpot);
    	    if(facePos(Earth)<.2){
    	        game.uploadPic();
    	    }
    	    break;
    	case 'T': //Take Picture
    	    float idlePosition[3] = {pois[spherePoi*3],pois[spherePoi*3+1],-0.4f};
        	facePos(center);
        	if(myState[2]>-0.25f&&myState[0]>0.0f){
        	    idlePosition[0] = -.1f;
        	    idlePosition[1] = 0.0f;
        	    idlePosition[2] = -.6f;
        	}else if(pois[spherePoi*3+2]<0.0f){//&&fabsf(tP1[1])<.19f){
        	    game.getPOILoc(idlePosition,spherePoi);
        	    predictPOIFutureState(5,idlePosition);
            	calcEntry(idlePosition,.43f);
        	}
        	//DEBUG(("\n(%f,%f,%f)",idlePosition[0],idlePosition[1],idlePosition[2]));
        	setPos(idlePosition,.22f);
        	
        	if(insidePoiZone(0,spherePoi)||game.alignLine(spherePoi)){
        	    game.takePic(spherePoi);
        	    if(game.getMemoryFilled()>memFilled){ //Successfully taken picture or stuck taking pic, go to next poi(Important that game.getMemoryFilled is called here)
        	        poi[spherePoi] = 0;
        	    }
        	}
        	break;
	}
}

int calcClosestPoi(bool p1, bool p2, bool p3){
    float dist1 = 0;
    if(poi[0]>0&&p1)
        dist1 = distance(pois,myState);
    else
        dist1 = 10000.0f;
    float dist2 = 0;
    if(poi[1]>0&&p2)
        dist2 = distance(&pois[3],myState);
    else
        dist2 = 10000.0f;
    float dist3 = 0;
    if(poi[2]>0&&p3)
        dist3 = distance(&pois[6],myState);
    else
        dist3 = 10000.0f;
    if(dist3<dist1&&dist3<dist2)
        return 2;
    else if(dist2<dist1&&dist2<dist3)
        return 1;
    else
        return 0;
}

float facePos(float target[3]){ //Points sphere to passed target
    //Rotate to target
    float attTarget[3];
    mathVecSubtract(attTarget,target,myState,3);
    mathVecNormalize(attTarget,3);
    api.setAttitudeTarget(attTarget);
    return distance(attTarget,&myState[6]);
}

bool intersectsAsteroid(float targetPos[3]){
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
    mathVecSubtract(tempVec,targetPos,myState,3);
    mathVecScale(tempVec,percOnLine,false);
    mathVecAdd(tempVec,myState,tempVec,3);
    mathVecSubtract(tempVec,tempVec,center,3);
    float len = mathVecMagnitude(tempVec,3);
    DEBUG(("\nlen%f",len));
    return len>=.31f;
}

void mathVecScale(float src[3], float mag, bool norm)
{
    if(norm) mathVecNormalize(src,3);
    src[0]*=mag;
    src[1]*=mag;
    src[2]*=mag;
}

// void doOrbit(float targetPos[3],float speed){ //Need to optimize this(currently testing)
//     float mySt[12];
//     memcpy(mySt,myState,12);
//     float orig[3];
//     memcpy(orig,targetPos,3);
//     //Rotate problem
//  float xAxis1[3] = {-1.0f,0.0f,0.0f};
//  float yAxis1[3] = {0.0f,-1.0f,0.0f};
//  float zAxis1[3] = {0.0f,0.0f,-1.0f};
//  float rots1[3] = {mathVecInner(myState,xAxis1,3),mathVecInner(myState,yAxis1,3),mathVecInner(myState,zAxis1,3)};
//     float rots2[3] = {mathVecInner(targetPos,xAxis1,3),mathVecInner(targetPos,yAxis1,3),mathVecInner(targetPos,zAxis1,3)};
//  rotate(mySt,myState,rots1);
//  rotate(targetPos,targetPos,rots2);
//     float angle = 0.0f;
//     float radius = mathVecMagnitude(mySt,3);
//     //Safety
//     if(radius>.6f)
//         radius = .6f;
//     //Calculate Current Angle
//     angle = acosf(mySt[0]/(sqrtf(powf(mySt[0],2)+powf(mySt[1],2)+powf(mySt[2],2))));
//     if(mySt[1]<0){
//         angle=2*PI-angle;
//     }
//     mathVecSubtract(tempVec,targetPos,mySt,3);
//  if(tempVec[0]==0)
//      tempVec[0]=-1;
//  if(tempVec[1]==0)
//      tempVec[1]=-1;
//  //Calc Orbit Direction
//  if(fabsf(tempVec[1])/tempVec[1]*fabsf(tempVec[0])/tempVec[0]>0){
//      angle-=(20*PI/180);//clockwise
//  }
//  else{
//      angle+=(20*PI/180);
//  }
//  //Calculate position on orbit circle
//     targetPos[0] = radius * cosf(angle);
//  targetPos[1] = radius * sinf(angle);
//  //Rotate to on to plane between targetPos and myPos
//  mathVecAdd(tempVec,mySt,orig,3);
//  mathVecNormalize(tempVec,3);
//  float xAxis[3] = {1.0f,0.0f,0.0f};
//  float yAxis[3] = {0.0f,1.0f,0.0f};
//  float zAxis[3] = {0.0f,0.0f,1.0f};
//  float rots[3] = {mathVecInner(tempVec,xAxis,3),mathVecInner(tempVec,yAxis,3),mathVecInner(tempVec,zAxis,3)};
//  rotate(targetPos,targetPos,rots);
//  setPos(targetPos,speed);
 
//  /*if(distance(myState,center)<.35){
//      mathVecSubtract(tempVec,myState,center,3);
//      api.setForces(tempVec);
//  }else{
//  //mathVecSubtract(tempVec,myState,center,3);
//  //float axis[3] = {1.0,0.0,0.0};
//  //mathVecCross(tempVec,tempVec,axis);
//  //mathVecAdd(targetPos,center,tempVec,3);
//  api.setPositionTarget(targetPos);*/
//  calcMidPoint(targetPos,distance(myState,center));
//  calcMidPoint(targetPos,distance(myState,center));
//  DEBUG(("\nOrbiting!:(%f,%f,%f)",targetPos[0],targetPos[1],targetPos[2]));
//  setPos(targetPos,.15f);
// }

void predictPOIFutureState(int timeInFuture, float poiState[3]){ //Error +/-0.0055
    float rotVector[3] = {0.0f,-0.1f,0.0f};
    float y = poiState[1]; //Verify Y value of poi is undisturbed
    for(int i=0;i<timeInFuture;i++){
        rotate(poiState,poiState,rotVector);
    }
    poiState[1] = y;
}

bool insidePoiZone(int poiZ,int spherePoi){ //0=Outer poi zone, 1=Inner poi zone
    float diff[3];
    float poi[3];
    game.getPOILoc(poi,spherePoi);
    mathVecSubtract(diff,myState,poi,3);
    float angle = acosf((myState[0]*diff[0]+myState[1]*diff[1]+myState[2]*diff[2])/(sqrtf((myState[0]*myState[0]+myState[1]*myState[1]+myState[2]*myState[2])*(diff[0]*diff[0]+diff[1]*diff[1]+diff[2]*diff[2]))));
    if(poiZ==1&&angle<.8*180/PI)
        return true;
    else if(poiZ==0&&angle<.4*180/PI)
        return true;
    else
        return false;
}

void calcPoiEntry(int poiID, float poi[3],float radius){
    game.getPOILoc(poi,poiID);
    float x = poi[0];
    poi[0] = radius*x/(sqrtf(x*x+poi[1]*poi[1]+poi[2]*poi[2]));
    if(x==0)
        x=1;
    poi[1] = poi[0]*poi[1]/x;
    poi[2] = poi[0]*poi[2]/x;
    //DEBUG(("\nPOI(%f,%f,%f)",poi[0],poi[1],poi[2]));
}

bool pickUpItem(int itemID){
    if(game.hasMemoryPack(0, itemID))
        return true;
    float memPack[3] = {-.5f,.6f,0.0};
    memPack[1] = itemID==0?-.6f:.6f;
    if(distance(myState,memPack)<.05){
        float attTarget[3] = {0.0f,0.7f,0.0f};
        api.setAttRateTarget(attTarget);
    }else{
        float attTarget[3] = {0.0f,0.0f,0.0f};
        api.setAttRateTarget(attTarget);
    }
    setPos(memPack,.125f);
    return false;
}

// void safeSetPosition(float tP[3],float speed){
//     if(intersectsAsteroid(tP)){
//         doOrbit(tP,speed);
//         DEBUG(("ORBIT!"));
//     }
//     else{
//         setPos(tP,speed);
//     }
// }

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

void rotate(float result[3],float vector[3],float rotation[3]){
    result[0] = vector[0]*cosf(rotation[1])*cosf(rotation[2])+vector[1]*(cosf(rotation[2])*sinf(rotation[0])*sinf(rotation[1])-cosf(rotation[0])*sinf(rotation[2]))+vector[2]*(cosf(rotation[0])*cosf(rotation[2])*sinf(rotation[1])+sinf(rotation[0])*sinf(rotation[2]));
    result[1] = vector[0]*cosf(rotation[1])*sinf(rotation[2])+vector[1]*(cosf(rotation[0])*cosf(rotation[2])+sinf(rotation[0])*sinf(rotation[1])*sinf(rotation[2]))-vector[2]*(cosf(rotation[2])*sinf(rotation[0])+cosf(rotation[0])+sinf(rotation[1])*sinf(rotation[2]));
    result[2] = vector[0]*-sinf(rotation[1])+vector[1]*cosf(rotation[1])*sinf(rotation[0])+vector[2]*cosf(rotation[0])*cosf(rotation[1]);
}

float distance(float startLoc[3],float targetLoc[3]){ //Distance between two 3d vectors
    mathVecSubtract(tempVec,targetLoc,startLoc,3);
    return mathVecMagnitude(tempVec,3);
}

void calcEntry(float poi[3],float radius){
    float x = poi[0];
    //poi[0] = radius*x/(sqrtf(powf(x,2)+powf(poi[1],2)+powf(poi[2],2)));
    poi[0] = radius*x/(sqrtf(x*x+poi[1]*poi[1]+poi[2]*poi[2]));
    if(x==0)
        x=1;
    poi[1] = poi[0]*poi[1]/x;
    poi[2] = poi[0]*poi[2]/x;
}

bool willCollide(int timeInFuture){
    //Calculate future other position(velocity*time+currentState)
    float futureOtherPos[3];
    float futureOurPos[3];
    for(int i=0;i<3;i++){
        futureOtherPos[i] = otherState[i+3]*timeInFuture+otherState[i];
        futureOurPos[i] = myState[i+3]*timeInFuture+myState[i+3];
    }
    //Check magnitude between otherFutureState and our Future State within collision sphere
    mathVecSubtract(tempVec,futureOtherPos,futureOurPos,3);
    if(mathVecMagnitude(tempVec,3)<.15f)
        return true;
    return false;
}
void calcMidPoint(float targetPos[3],float radius){
    float midPoint[3];
    mathVecAdd(midPoint,myState,targetPos,3);
    midPoint[0]/=2;
    midPoint[1]/=2;
        midPoint[2] = sqrtf(powf(radius,2)-powf(midPoint[0],2)-powf(midPoint[1],2));
    if(targetPos[2]<=0.0f){
        midPoint[2] = -midPoint[2];
    }
 }