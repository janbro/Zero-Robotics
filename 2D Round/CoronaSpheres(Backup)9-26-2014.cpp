//TODO: Organize DEBUG statements
//      Make faster set position
//      SafesetPosition in progress

//Declare any variables shared between functions here
float myState[12],otherState[12],tempVec[3],tP1[3],poi1[3],poi2[3],center[3];
float mid[3];
bool memFull,on;
int sphere,spherePoi,timeToFlare,time,poiZone,lastMem,picTries,picsUploaded;

void init(){
	//This function is called once when your code is first loaded.

	//IMPORTANT: make sure to set any variables that need an initial value.
	//Do not assume variables will be set to 0 automatically!
	on=true;
	time=center[0]=center[1]=center[2]=lastMem=picTries=poiZone=picsUploaded=0;
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
        picsUploaded = 0;
        lastMem = 0;
        poiZone = 0;
    }if(time%60==0){
        picTries = 0;
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
	if((game.getMemoryFilled()<2&&poiZone<2)&&(timeToFlare>12||timeToFlare==-1)&&(time<230)&&picsUploaded<(time/60+1)*2){ //If we have space and no incoming solar flare
    	switch(poiZone){
	        case 0: //Outer Ring
                calcPoiEntry(spherePoi,tP1,.42f);
	            api.setPositionTarget(tP1);
	            break;
    	    case 1: //Inner Ring
	            calcPoiEntry(spherePoi,tP1,.370f);
	            api.setPositionTarget(tP1);
	            break;
	    }
	    mathVecSubtract(tempVec,tP1,&myState[0],3);
	    if(distance(&myState[0],tP1)<.025f&&mathVecInner(tempVec,&myState[6],3)<.05f){ //If in range, take picture
	        //DEBUG(("Take Pic!"));
	        game.takePic(spherePoi);
	        picTries++;
	        if(game.getMemoryFilled()>lastMem||picTries>10){ //Successfully taken picture or stuck taking pic, go to next poi
	            poiZone++;
	        }
	    }
	}else{ //Solar Flare incoming
	    //DEBUG(("HERE:%i",game.getMemoryFilled()));
	    if(game.getMemoryFilled()>0){ //Upload pictures in memory
	        //float memPack[] = {-.5,sphere*.6,0};
            calcPoiEntry(spherePoi,tP1,.65);
	        api.setPositionTarget(tP1);
	        game.uploadPic();
    	    if(distance(&myState[0],center)>.5){
	            picsUploaded+=game.getMemoryFilled();
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

void calcPoiEntry(int poiID, float (&poi)[3],float radius){
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

void calculateEntryPoint(float (&targetEntryPoint)[3],float target[3],float radius){//Point closest on sphere
	float sphere[3]={myState[0],myState[1],myState[2]};
	float debris[3]={target[0],target[1],target[2]};
	float r=radius;
	targetEntryPoint[1]=-((r/sqrtf(powf(sphere[0]-debris[0],2)+powf(sphere[1]-debris[1],2)+powf(sphere[2]-debris[2], 2)))*(sphere[0]-debris[0]));
	targetEntryPoint[0]=((r/sqrtf(powf(sphere[0]-debris[0],2)+powf(sphere[1]-debris[1],2)+powf(sphere[2]-debris[2], 2)))*(sphere[1]-debris[1]));
	targetEntryPoint[2]=((r/sqrtf(powf(sphere[0]-debris[0],2)+powf(sphere[1]-debris[1],2)+powf(sphere[2]-debris[2], 2)))*(sphere[2]-debris[2]));
	
	targetEntryPoint[0]=targetEntryPoint[0]+debris[0];
	targetEntryPoint[1]=targetEntryPoint[1]+debris[1];
	targetEntryPoint[2]=targetEntryPoint[2]+debris[2];
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
