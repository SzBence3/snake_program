#pragma once
 #include <stdlib.h>
 #include <stdint.h>
 #pragma pack(push)
 #pragma pack(4)
 typedef float ipc_real_t;
 typedef uint64_t ipc_guid_t;
 struct IpcSelfInfo {
     ipc_real_t segment_radius;
     ipc_real_t mass;
     ipc_real_t sight_radius;
     ipc_real_t consume_radius;
     uint32_t start_frame;
     uint32_t current_frame;
     ipc_real_t speed;
     ipc_real_t max_step_angle;
     ipc_real_t consumed_natural_food;
     ipc_real_t consumed_food_hunted_by_self;
     ipc_real_t consumed_food_hunted_by_others;
 };
 struct IpcServerConfig {
     uint32_t   snake_boost_steps;
     ipc_real_t snake_turn_radius_factor;
     ipc_real_t snake_pull_factor;
     ipc_real_t snake_conversion_factor;
     ipc_real_t snake_segment_distance_factor;
     ipc_real_t snake_segment_distance_exponent;
     ipc_real_t snake_consume_range;
     ipc_real_t snake_boost_loss_factor;
     ipc_real_t snake_survival_loss_factor;
     ipc_real_t snake_self_kill_mass_theshold;
     ipc_real_t food_decay_step;
     ipc_real_t log_credits_per_frame;
     ipc_real_t log_initial_credits;
     ipc_real_t log_max_credits;
 };
 struct IpcFoodInfo {
     ipc_real_t x;
     ipc_real_t y;
     ipc_real_t val;
     ipc_real_t dir;
     ipc_real_t dist;
 };
 struct IpcBotInfo {
     ipc_guid_t bot_id;
     char       bot_name[64];
 };
 struct IpcSegmentInfo {
     ipc_real_t x;
     ipc_real_t y;
     ipc_real_t r;
     ipc_real_t dir;
     ipc_real_t dist;
     uint32_t   idx;
     ipc_guid_t bot_id;
     bool       is_self;
 };
 struct IpcColor {
     uint8_t r;
     uint8_t g;
     uint8_t b;
 };
 const size_t IPC_FOOD_MAX_BYTES = 1 * 1024*1024;
 const size_t IPC_FOOD_MAX_COUNT = IPC_FOOD_MAX_BYTES / sizeof(struct IpcFoodInfo);
 const size_t IPC_BOT_MAX_COUNT = 1024;
 const size_t IPC_BOT_MAX_BYTES = IPC_BOT_MAX_COUNT * sizeof(struct IpcBotInfo);
 const size_t IPC_SEGMENT_MAX_BYTES = 1 * 1024*1024;
 const size_t IPC_SEGMENT_MAX_COUNT = IPC_SEGMENT_MAX_BYTES / sizeof(struct IpcSegmentInfo);
 const size_t IPC_COLOR_MAX_COUNT = 1024;
 const size_t IPC_COLOR_MAX_BYTES = IPC_COLOR_MAX_COUNT * sizeof(struct IpcColor);
 const size_t IPC_LOG_MAX_BYTES = 1024;
 const size_t IPC_PERSISTENT_MAX_BYTES = 4096;
 struct IpcSharedMemory {
     struct IpcServerConfig serverConfig;
     struct IpcSelfInfo selfInfo;
     uint32_t foodCount;
     struct IpcFoodInfo foodInfo[IPC_FOOD_MAX_COUNT];
     uint32_t botCount;
     struct IpcBotInfo botInfo[IPC_BOT_MAX_COUNT];
     uint32_t segmentCount;
     struct IpcSegmentInfo segmentInfo[IPC_SEGMENT_MAX_COUNT];
     uint32_t colorCount;
     struct IpcColor colors[IPC_COLOR_MAX_COUNT];
     char logData[IPC_LOG_MAX_BYTES];
     uint32_t faceID;
     uint32_t dogTagID;
     uint8_t persistentData[IPC_PERSISTENT_MAX_BYTES];
 };
 const size_t IPC_SHARED_MEMORY_BYTES = sizeof(struct IpcSharedMemory);
 /*
  * Communication structures.
  */
 enum IpcRequestType {
     REQ_INIT,
     REQ_STEP
 };
 struct IpcRequest {
     enum IpcRequestType type;
 };
 enum IpcResponseType {
     RES_OK,
     RES_ERROR
 };
 struct IpcResponse {
     enum IpcResponseType type;
     union {
         struct {
             ipc_real_t deltaAngle;
             bool       boost;
         } step;
     };
 };
 #pragma pack(pop)