//Global Variables
ZRState myState, otherState;
float tempVec[3], targetPoiEntry[3], poi1[3], poi2[3], center[3];
bool on, memFull;
int sphere, time, targetPoi, poiZone, timeToFlare, lastNumPics, picsTried, memFilled;

void init(){
	sphere=myState[1]>0.0?1:-1;//Blue Sphere = 1
	on = true;
	memFull = false;
	memFilled = time = center[0] = center[1] = center[2] = lastMem = 0;
	game.getPOILoc(poi1,0);
	game.getPOILoc(poi2,1);
}

void updateVariables(){
	//Update game variables
	api.getMyZRState(myState);
	api.getOtherZRState(otherState);
	timeToFlare = game.getNextFlare();
	memFull = game.getMemoryFilled()<game.getMemorySize();
	memFilled = game.getMemoryFilled();
	//Calculate closest poi(hardcoded 2 poi)
    if(distance(poi1,&myState[0])<distance(poi2,&myState[0]))
        spherePoi=0;
    else
        spherePoi=1;
	
	lastNumPics = game.getMemoryFilled();
	++time;
}

void loop(){
	updateVariables();
	
	//Solar Flare Auto Shutdown
	if(timeToFlare==-1&&!on){
		game.turnOn();
		on = true;
	}
	else if(timeToFlare<3&&on){
		game.turnOff();
		on = false;
	}
	
	//POI Navigator
	if(!memFull&&(timeToFlare>12||timeToFlare==-1)){
		//Goto target poiZone(Outer or Inner)
		switch(poiZone){
			case 0:
				calcPoiEntry(spherePoi,targetPoiEntry,.42f);
				setPos(targetPoiEntry,.175f);
				break;
			case 1:
				calcPoiEntry(spherePoi,targetPoiEntry,.37f);
				setPos(targetPoiEntry,.175f);
				break;
			default:
				facePos(center);
		}
		//Take Pictures
		if(distance(myState,targetPoiEntry)<.02f&&game.alignLine()){
			game.takePic(spherePoi);
			++picsTried;
			//Successfully taken picture or stuck taking poi, go to next poi
			if(game.getMemoryFilled()>lastMem||picsTried){
				++poiZone;
			}
		}
	}
	//Solar Flare Incoming or Upload Pictures
	else{
		//Pictures in memory
		if(memFilled>0){
			calcPoiEntry(spherePoi,targetPoiEntry,.65f);
			api.setPositionTarget(targetPoiEntry);
			//Outside poiZones, upload pictures
			if(distance(myState,center)>.5){
				game.uploadPic();
				if(game.getMemoryFilled()==0){
					picsTried = 0;
					poiZone = 0;
				}
			}
		}
		else{
			api.setVelocityTarget(center);
			game.takePic(spherePoi);	
		}
	}
}

void calcPoiEntry(int poiID, float poi[3],float radius){
    game.getPOILoc(poi,poiID);
    if(poi[1]==0.0){
         poi[0]=-radius;
         poi[1]=0.0;
         poi[2]=0.0;
    }
    float m=poi[0]/poi[1];
    if(poi[1]<0)
        poi[1]=-radius/sqrtf(pow(m,2)+1);
    else
        poi[1]=radius/sqrtf(pow(m,2)+1);
    poi[0]=m*poi[1];
    poi[2]=0;
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

//Steelmen
void setPos(float targetPos[3],float speedConstant)
{
	float velocityTarget[3];
	mathVecSubtract(tempVec,targetPos,myState,3);
	float distance = mathVecMagnitude(tempVec,3);
	
	float speedTarget = distance*speedConstant-distance*distance*0.1f;
	if((speedTarget>.055f || distance>.3f))
		speedTarget = .055f;
	
	//sets the target velocity using the speed target and position target
	for (int i = 0; i < 3; i++)
		velocityTarget[i] = speedTarget*(targetPos[i]-myState[i])/distance;
	api.setVelocityTarget(velocityTarget);
}
