#include<algorithm>
#include<map>
#include<utility>
#include<sstream>
#include<cmath>
#include<vector>
using namespace std;
#include "usercode.h"

map<unsigned long long, vector<IpcSegmentInfo>>segments;
int my_id = 0;

string lastMode, curMode, extraInfo;

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
    auto [x,y,value,dir,dist] = f;
    return "x: "+ to_string(x) + " y: "+ to_string(y) +" dir: "+ to_string(dir) +" dist: "+ to_string(dist) + " value: " + to_string(value);
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

pair<float, float> pos_of_second_seg(Api* api)
{
    return {segments[my_id][1].x, segments[my_id][1].y};
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
    auto [ax,ay] = pos_of_second_seg(api);

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
    if(botNames.count(bot_id))return botNames[bot_id];
    auto b = api->getBots();
    int bc = api->getBotCount();
    for(int i = 0; i < bc; i ++){
        if(b[i].bot_id == bot_id)return botNames[bot_id] = b[i].bot_name;  
        // botNames[b[i].bot_id] = b[i].bot_name;
    }
    return "UNKOWN";
}

pair<IpcSegmentInfo, IpcSegmentInfo> getFirstSegments(Api *api, unsigned long long bot_id){
    return {segments[bot_id][0], segments[bot_id][1]};
}

float const maxAhead = 500;

float dists(float x, float y, float ax, float ay){
    return (x-ax)*(x-ax)+(y-ay)*(y-ay);
}

bool isAhead(Api *api, float x, float y){
    auto [sx, sy] = pos_of_second_seg(api);
    return dists(x,y,sx,sy) > dists(x,y,0,0);
}

bool isSharpAttack(Api *api, Target t){
    if(t.bestidx != 0)return 0;
    auto [f,s] = getFirstSegments(api,t.bot_id);
    float dx = f.x-s.x, dy = f.y-s.y;
    float a = 1/sqrt(dx*dx+dy*dy);
    dx*=a;
    dy*=a;
    for(float b = 40; b < maxAhead; b+=20){
        float tx = f.x+dx*b;
        float ty = f.y+dy*b;
        if(isAhead(api, tx,ty) && dists(tx,ty,0,0)*1.5*1.5 < dists(tx,ty, f.x,f.y)){
            //api->log(("sharp attack on " + getName(api, t.bot_id) + " at " + to_string(tx) + " " + to_string(ty) + " ahead by " + to_string(b)).c_str());
            curMode = "sharp attack: " + getName(api, t.bot_id);
            extraInfo = ", ahead by: " + to_string(b);
            api->boost = 1;
            ftarget(api, tx,ty);
            return 1;
        }
    }
    return 0;
}
void getData(Api* api){
    auto s = api->getSegments();
    int sc = api->getSegmentCount();
    segments.clear();
    for(int i = 0; i < sc; i++){
        segments[s[i].bot_id].push_back(s[i]);
        if(s[i].is_self)my_id = s[i].bot_id;
    }
    for(auto& [id, segs] : segments){
        sort(segs.begin(), segs.end(), [](IpcSegmentInfo a, IpcSegmentInfo b){
            return a.idx < b.idx;
        });
    }
}

pair<float,float> getGoodTargetPos(Api *api, Target t){
    auto [f,s] = getFirstSegments(api,t.bot_id);
    float dx = f.x-s.x, dy = f.y-s.y;
    float a = 1/sqrt(dx*dx+dy*dy);
    dx*=a;
    dy*=a;
    return {f.x+dx*5*f.r, f.y+dy*5*f.r};
}



bool step(Api *api)
{
    lastMode = curMode;
    getData(api);
    api->boost = 0;
    // if(allahAkbar(api)){return 1;}
    Target target1 = getBestTarget(api);
    // api->log((getName(api, target1.bot_id) + "value: " + to_string(target1.value) + " bid: " + to_string(target1.bestidx) + " bot_id: "+ to_string(target1.bot_id)).c_str());
    if(isEnoughSegment(api) && target1.value > 0.8f){
        if(isSharpAttack(api, target1)){
            //sharpAttack(api, target1);
            return 1;
        }
    }
    else target1 = {0,0,0,0,0};
    if(isFlight(api)){
        FlightModeDistanceThreshold = (api->getServerConfig()->snake_turn_radius_factor+1.0f) * api->getSelfInfo()->segment_radius * 2;
        FlightModeDistanceThreshold = max(FlightModeDistanceThreshold, 20.0f);
        //api->log("in flight mode");
        curMode = "Flight"; extraInfo = "";
        flight(api);
    }else{
        if(!isEnoughSegment(api)){

            food foodtarget = getFoodTarget(api);
            target(api, foodtarget.x, foodtarget.y, foodtarget.dir);
            //api->log(("(no second segment yet) Going for food: " + to_string(foodtarget)).c_str());
            curMode = "initial eating"; extraInfo="";
        }
        else {
            food foodtarget = getFoodTargetBetter(api);
            if(foodtarget.val/api->getSelfInfo()->sight_radius < 0.0007 && target1.value > 1.0f){
                //api->log(("Going for target: " + getName(api, target1.bot_id) + " value: " + to_string(target1.value)).c_str());
                curMode = "Attacking " + getName(api, target1.bot_id); extraInfo=", target value: " + to_string(target1.value);
                auto [f,s] = getFirstSegments(api,target1.bot_id);
                auto [x,y] = getGoodTargetPos(api, target1);
                ftarget(api, x,y);
                return 1;
            }

            ftarget(api, foodtarget.x, foodtarget.y);
            if(api->getSelfInfo()->mass < 5000 && foodtarget.val/api->getSelfInfo()->sight_radius > 0.001)api->boost = 1;
            //api->log(("Going for food: " + to_string(foodtarget.val/api->getSelfInfo()->sight_radius)).c_str());
            curMode = "Eating";extraInfo="";
        }
    }
    if(lastMode != curMode){
        api->log(("new mode: " + curMode + extraInfo).c_str());
    }
    return true;
}
