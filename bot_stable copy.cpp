#include<algorithm>
#include<map>
#include<utility>
#include<sstream>
#include<cmath>
#include<vector>
using namespace std;
#include "usercode.h"

map<int,string>botNames;
bool init(Api *api)
{
    api->clearColors();
    api->addColor(255,0,0);
    api->addColor(255,0,0);
    api->addColor(255,255,255);
    api->addColor(255,255,255);
    api->addColor(0,0,0);
    api->addColor(0,0,0);
    auto b = api->getBots();
    int bc = api->getBotCount();
    for(int i = 0; i < bc; i ++){
        botNames[b[i].bot_id] = b[i].bot_name;
    }

    return true;
}
typedef IpcFoodInfo food;

float getFacingDir(Api* api){ // prob bugged
    food someFood = *(api->getFood());
    float angleFoodAndY = atan(someFood.x / someFood.y);
    float angleSnakeAndY = angleFoodAndY - someFood.dir;
    return angleSnakeAndY;
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

float FlightModeDistanceThreshold = 0;

bool isFlight(Api *api){
    const IpcSegmentInfo* segs = api->getSegments();
    for(int i=0;i<api->getSegmentCount();i++){
        if(!segs[i].is_self && segs[i].dist - segs[i].r*2 - api->getSelfInfo()->segment_radius <= FlightModeDistanceThreshold) return true;
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

void flight(Api *api){
    const IpcSegmentInfo* segs = api->getSegments();
    int worst_segment=-1;
    for(int i=0;i<api->getSegmentCount();i++){
        if(!segs[i].is_self && 
            (worst_segment<0 || 
            segs[i].dist-segs[i].r < segs[worst_segment].dist-segs[worst_segment].r)
        ) 
            worst_segment=i;
    }
    auto self = api->getSelfInfo();
    if(segs[worst_segment].dir>0) api->angle=-self->max_step_angle;
    else api->angle=self->max_step_angle;
}

food getFoodTargetBetter(Api *api){
    int sight = api->getSelfInfo()->sight_radius;
    int n = 30;
    int consrad = sight/(n-2);
    
    vector<vector<float>>v();
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
    return t;

}

struct Target{
    float r, dist;
    unsigned long long bot_id;
    unsigned int bestidx;
    float value;
};

Target getBestTarget(Api *api){
    map<int,Target>mep;
    auto segments = api->getSegments();
    int segmentcount = api->getSegmentCount();
    for(int i = 0; i < segmentcount; i++){
        if(segments[i].is_self)continue;
        if(!mep.count(segments[i].bot_id)){
            mep[segments[i].bot_id] = {segments[i].r, segments[i].dist, segments[i].bot_id, segments[i].idx,0};
        }else 
        if(segments[i].idx < mep[segments[i].bot_id].bestidx){
            mep[segments[i].bot_id] = {segments[i].r, segments[i].dist, segments[i].bot_id, segments[i].idx,0};
        }
    }
    Target best = {0,0,0,0,0};
    for(auto& [id, t] : mep){
        t.value = t.r*(pow(1/(t.dist + t.r*t.bestidx), 0.5));
        if(t.value > best.value)best = t;
    }
    return best;
}

string getName(Api *api, unsigned long long bot_id){
    auto b = api->getBots();
    int bc = api->getBotCount();
    for(int i = 0; i < bc; i ++){
        if(b[i].bot_id == bot_id)return b[i].bot_name;  
        // botNames[b[i].bot_id] = b[i].bot_name;
    }
    return "UNKOWN";
}

bool step(Api *api)
{
    
    // if(allahAkbar(api)){return 1;}
    Target target1 = getBestTarget(api);
    if(isEnoughSegment(api) ){
        
    }
    api->log((getName(api, target1.bot_id) + "value: " + to_string(target1.value) + " bid: " + to_string(target1.bestidx) + " bot_id: "+ to_string(target1.bot_id)).c_str());
    if(isFlight(api)){
        FlightModeDistanceThreshold = api->getServerConfig()->snake_turn_radius_factor * api->getSelfInfo()->segment_radius * 4;
        FlightModeDistanceThreshold = max(FlightModeDistanceThreshold, 50.0f);
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
    return true;
}
