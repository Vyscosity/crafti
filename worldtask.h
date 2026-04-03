#ifndef WORLDTASK_H
#define WORLDTASK_H

#include "task.h"
#include "world.h"
#include "gl.h"
#include "aabb.h"

class WorldTask : public Task
{
public:
    virtual void makeCurrent() override;

    virtual void logic(GLFix dt) override;
    virtual void render() override;

    void resetWorld();
    void respawnPlayer();

    GLFix x, y = World::HEIGHT * Chunk::SIZE * BLOCK_SIZE, z, xr, yr;

    static constexpr GLFix player_width = BLOCK_SIZE*0.8f, player_height = BLOCK_SIZE*1.8f, eye_pos = BLOCK_SIZE*1.6f;

    void setMessage(const char *message);

    /** Player damage from mobs / hazards (hearts, death screen, optional HUD message). */
    void hurtPlayer(unsigned int dmg, const char *msg = "Ouch!");

    unsigned int frameCount() { return frame_counter; }

private:
    void crosshairPixel(int x, int y);

    void getForward(GLFix *x, GLFix *z);
    void getRight(GLFix *x, GLFix *z);

    GLFix speed();

    //Player position and movement
    AABB aabb;
    bool can_jump = false, tp_had_contact = false;
    int tp_last_x = 0, tp_last_y = 0;
    GLFix vy = 0; //Y-Velocity for gravity and jumps
    bool in_water = false;

    // --- Survival / combat (Minecraft-ish) ---
    static constexpr unsigned int max_hearts = 10;
    unsigned int hearts = max_hearts;

    // Accumulated downward travel since last time we touched ground.
    GLFix fall_distance = 0;

    // After reset we want to place the player on solid ground (1 block above it)
    // so the first fall doesn't instantly kill the player once fall damage is enabled.
    bool safe_spawn_pending = true;

    VECTOR3 mining_pos = {-1, -1, -1};
    int mining_progress = 0;
    int mining_duration = 0;

    static constexpr unsigned int blockselection_frames = 2;
    unsigned int blockselection_frame = 0, blockselection_frame_fraction = 0;

    VECTOR3 selection_pos; AABB::SIDE selection_side; VECTOR3 selection_pos_abs; bool do_test = true; //For intersectsRay

    char message[40]; unsigned int message_timeout = 0;

    bool draw_inventory = true;
    unsigned int frame_counter = 0; // Incremented after each render

    /** Fractional simulation ticks carried across frames (mobs / ground drops). */
    GLFix sim_tick_accum = 0;
    /** Fractional mining progress toward the next integer tick. */
    GLFix mining_tick_accum = 0;
};

extern WorldTask world_task;

#endif // WORLDTASK_H
