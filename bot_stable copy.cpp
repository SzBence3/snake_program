#include<algorithm>
#include<utility>
#include<sstream>
#include<cmath>
#include<vector>
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
food getFoodTarget(Api *api){
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

float FlightModeDistanceThreshold = 0;

bool isFlight(Api *api){
    const IpcSegmentInfo* segs = api->getSegments();
    for(int i=0;i<api->getSegmentCount();i++){
        if(!segs[i].is_self && segs[i].dist - segs[i].r - api->getSelfInfo()->segment_radius <= FlightModeDistanceThreshold) return true;
    }
    return false;
}

int fordul(float x1, float y1, float x2, float y2, float x3, float y3)
{
    float s=(x3-x1)*(y2-y1)-(x2-x1)*(y3-y1);
    if(s<0) return -1;
    if(s>0) return 1;
    return 0;
}

pair<float, float> pos_of_first_seg(Api* api)
{
    auto self = api->getSelfInfo();
    const IpcSegmentInfo* segs = api->getSegments();
    for(int i=0;i<api->getSegmentCount();i++)
        if(segs[i].is_self && segs[i].idx==1) return {segs[i].x,segs[i].y};
    
}
int const enoughSegmentCount = 4;

bool isEnoughSegment(Api* api)
{
    const IpcSegmentInfo* segs = api->getSegments();
    for(int i=0;i<api->getSegmentCount();i++)
        if(segs[i].is_self && segs[i].idx== enoughSegmentCount-1) return 1;
    return 0;
    
}

void ftarget(Api *api, float x, float y){
    auto [ax,ay] = pos_of_first_seg(api);

    // stringstream ss;
    // ss << ax << " " << ay <<  " " << x << " " << y  << " " << fordul(ax,ay, 0, 0, x,y);
    // api->log(ss.str().c_str());
    
    api->angle = -fordul(ax,ay, 0, 0, x,y) * api->getSelfInfo()->max_step_angle;
}

bool allahAkbar(Api*api){
    const IpcSegmentInfo* segs = api->getSegments();
    int head = -1;
    for(int i=0;i<api->getSegmentCount();i++){
        if(!segs[i].is_self && segs[i].idx == 0) head = i;
    }
    if(head!=-1){
        target(api, segs[head].x, segs[head].y, segs[head].dir);
        api->boost=1;
        return 1;
    }
    return 0;
}

void flight(Api *api){
    const IpcSegmentInfo* segs = api->getSegments();
    int worst_segment=-1;
    for(int i=0;i<api->getSegmentCount();i++){
        if(!segs[i].is_self && (worst_segment<0 || segs[i].dist-segs[i].r<segs[worst_segment].dist-segs[worst_segment].r)) worst_segment=i;
    }
    auto self = api->getSelfInfo();
    if(segs[worst_segment].dir>0) api->angle=-self->max_step_angle;
    else api->angle=self->max_step_angle;
}

food getFoodTargetBetter(Api *api){
    int sight = api->getSelfInfo()->sight_radius;
    int consrad = api->getSelfInfo()->consume_radius;
    
    vector<vector<float>>v();
    int n = sight/consrad;
    float d = consrad*n;
    vector<vector<float>>grid(n*2+1, vector<float>(2*n+1));
    const food *foods = api->getFood();
    int foodCount = api->getFoodCount();
    for(int i = 0; i < foodCount; i++){
        food f = foods[i];
        int x = round(f.x/consrad);
        int y = round(f.y/consrad);
        x+=n;
        y+=n;
        if(x < 0 || x > 2*n || y < 0 || y > 2*n) continue;
        grid[x][y]+=f.val;
    }
    food t = {0,0,0.0001,0,0};
    for(int i = 0; i < 2*n+1; i++){
        for(int j = 0; j < n*2+1; j++){
            float x = (i-n)*consrad, y = (j-n)*consrad;

            if(grid[i][j]/pow(x*x+y*y,0.5) > t.val && canGetPosition(api, x,y,0)){
                t.x = x;
                t.y = y;
                t.val = grid[i][j]/pow(x*x+y*y,0.5);
            }
        }
    }
    /*string s;
    for(auto v : grid){
        s+="{";
        for(float i : v){
            s+=to_string(i)+",";
        }
        s+="},";
    }
    api->log(s.c_str());*/
    if(t.x == 0 && t.y == 0)api->log("didnt find good pos");
    
    // t.dir = atan2(t.x, t.y);
    // t.dir -= pi/2;
    // t.dir += getFacingDir(api);
    // while(t.dir < pi) t.dir += pi;
    // while(t.dir > pi) t.dir -= pi;

    return t;

}


bool step(Api *api)
{
    
    // if(allahAkbar(api)){return 1;}

    if(isFlight(api)){
        FlightModeDistanceThreshold = api->getServerConfig()->snake_turn_radius_factor * api->getSelfInfo()->segment_radius * 4;
        FlightModeDistanceThreshold = max(FlightModeDistanceThreshold, 30.0f);
        api->log("in flight mode");
        flight(api);
    }else{
        if(!isEnoughSegment(api)){

            food foodtarget = getFoodTarget(api);
            target(api, foodtarget.x, foodtarget.y, foodtarget.dir);
            api->log(("(no second segment yet) Going for food: " + to_string(foodtarget)).c_str());
        }
        else {
            food foodtarget = getFoodTargetBetter(api);
            ftarget(api, foodtarget.x, foodtarget.y);
            api->log(("Going for food: " + to_string(foodtarget)).c_str());
        }
    }
    logShit(api);
    return true;
}
