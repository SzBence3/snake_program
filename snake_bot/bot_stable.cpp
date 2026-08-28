#include<bits/stdc++.h>
using namespace std;
#include "usercode.h"

bool init(Api *api)
{
    api->clearColors();
    api->addColor(255,0,0);
    api->addColor(255,255,255);
    api->addColor(0,0,0);
    /*api->addColor(40, 255, 0);
    api->addColor(20, 128, 0);
    api->addColor(10,  64, 0);
    api->addColor(20, 128, 0);*/

    return true;
}
typedef IpcFoodInfo food;

float getFacingDir(Api* api){ // atan(x/y)
    food someFood = *(api->getFood());
    float angleFoodAndY = atan(someFood.x / someFood.y);
    float angleSnakeAndY = angleFoodAndY - someFood.dir;
    return angleSnakeAndY;
}

const float delta=0.001;
bool canGetFood_bad(Api *api, food f){
    auto self = api->getSelfInfo();
    auto max_angle_change=self->max_step_angle;
    return (max_angle_change+delta>=f.dir);
    return true;
}

bool canGetPosition(Api* api, float x, float y, float dir){
    float r =( api->getServerConfig()->snake_turn_radius_factor) * (api->getSelfInfo()->segment_radius);
    float mydir = getFacingDir(api);
    float ox = cos(mydir)*r, oy =sin(mydir)*r; 
    float dist = (ox-x)*(ox-x)+(oy-y)*(oy-y);
    ox = -ox;
    oy = -oy;
    dist = min(dist, (ox-x)*(ox-x)+(oy-y)*(oy-y));
    if(dist < r*r)return false;
    else return true;
}
bool canGetFood(Api* api, food f){
    return canGetPosition(api, f.x, f.y, f.dir);
}

const float distance_limit=0;
food getFoodTarget_H(Api *api){
    const food* foodList = api->getFood();
    int bestFoodInd=0;
    int num_of_foods=api->getFoodCount();
    vector<bool> is_reachable(num_of_foods);
    for(int i=0;i<api->getFoodCount();i++){
        if(canGetFood(api, foodList[i]) && foodList[i].dist < foodList[bestFoodInd].dist) bestFoodInd = i;
        if(canGetFood(api, foodList[i])) is_reachable[i]=1;
    }
    //if(canGetFood(api, foodList[bestFoodInd])) return foodList[bestFoodInd];
    int final_best=bestFoodInd;
    for(int i=0; i<num_of_foods; i++)
        if(is_reachable[i] && foodList[i].dist<=foodList[bestFoodInd].dir+distance_limit) final_best=i;
    if(canGetFood(api, foodList[final_best])) return foodList[final_best];
    api->log("[getFoodTarget] no can get any food this bad\n");
    
    return {0, 0, 0, 0, 0};
}
food getFoodTarget(Api *api){ //using canGetFood_bad
    const food* foodList = api->getFood();
    int bestFoodInd=0;
    for(int i=0;i<api->getFoodCount();i++){
        if(canGetFood(api, foodList[i]) && foodList[i].dist < foodList[bestFoodInd].dist) bestFoodInd = i;
    }
    if(canGetFood(api, foodList[bestFoodInd])) return foodList[bestFoodInd];
    api->log("[getFoodTarget] no can get any food this bad\n");
    return {0, 0, 0, 0, 0};
}

void target(Api *api, float x, float y, int dir){
    auto self = api->getSelfInfo();
    if(abs(dir) < self->max_step_angle)
        api->angle = dir;
    else 
        if(dir < 0){
            api->angle = -self->max_step_angle;
        }
        else 
            api->angle = self->max_step_angle;
}

string to_string(food f){
    auto [x,y,dir,dist, value] = f;
    return "x: "+ to_string(x) + " y: "+ to_string(y) +" dir: "+ to_string(dir) +" dist: "+ to_string(dist) + " value: " + to_string(value);
}

void logShit(Api *api){
    //api->log(("Snake facing compared to Y axis: " + to_string(getFacingDir(api))).c_str());
    api->log(("vision: " + to_string(api->getSelfInfo()->sight_radius)).c_str());
}

bool step(Api *api)
{
    /*
    if(isFLight(api)){
        flight(api);
    }else 
    //*/
    {
        
        food foodtarget = getFoodTarget(api);
        target(api, foodtarget.x, foodtarget.y, foodtarget.dir);
        api->log(("Going for food: " + to_string(foodtarget)).c_str());
    }
    logShit(api);
    return true;
}
