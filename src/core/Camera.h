#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// =============================================================================
// Camera
// =============================================================================
// Owns the view and projection matrices. The Player holds a Camera and updates
// its position every physics tick. During rendering, the camera position is
// interpolated between the last two physics states so motion looks smooth at
// any framerate, not just at the fixed 60Hz physics rate.
//
// HOW TO EXTEND:
//   - FOV zoom (scope/ads): lerp camera.fov toward a lower value, call
//     projectionMatrix() — it recalculates every frame so any change takes
//     effect immediately.
//   - Camera shake: keep a shake offset vec3 and add it to position inside
//     viewMatrix() without touching the real position.
//   - Third-person: change the position passed to glm::lookAt to orbit around
//     the player position at some radius.
// =============================================================================

class Camera {
public:
    glm::vec3 position;
    float yaw;    // horizontal angle in degrees (left/right)
    float pitch;  // vertical angle in degrees (up/down), clamped to -89..89

    float fov;         // vertical field of view in degrees
    float aspectRatio; // width / height — update this on window resize
    float nearPlane;   // anything closer than this is clipped (keep > 0)
    float farPlane;    // anything farther is clipped

    Camera(glm::vec3 pos = {0, 1.7f, 0}, float fov = 90.f,
           float aspect = 16.f / 9.f, float nearP = 0.05f, float farP = 500.f)
        : position(pos), yaw(-90.f), pitch(0.f),
          fov(fov), aspectRatio(aspect), nearPlane(nearP), farPlane(farP)
    {}

    // The direction the camera is looking, including vertical pitch.
    // Used for viewMatrix() and for aiming weapons/projectiles.
    glm::vec3 forward() const {
        return glm::normalize(glm::vec3{
            cosf(glm::radians(yaw)) * cosf(glm::radians(pitch)),
            sinf(glm::radians(pitch)),
            sinf(glm::radians(yaw)) * cosf(glm::radians(pitch))
        });
    }

    // Horizontal-only forward — ignores pitch so moving forward while looking
    // up/down doesn't send the player into the sky or ground.
    // Used by Player::handleMovement for WASD direction.
    glm::vec3 flatForward() const {
        return glm::normalize(glm::vec3{
            cosf(glm::radians(yaw)), 0.f, sinf(glm::radians(yaw))
        });
    }

    // Strafe axis — perpendicular to forward on the horizontal plane.
    glm::vec3 right() const {
        return glm::normalize(glm::cross(forward(), glm::vec3{0, 1, 0}));
    }

    // Apply raw mouse delta (pixels) from SDL_MOUSEMOTION.
    // sensitivity scales pixels -> degrees; lower = slower turn.
    void applyMouseDelta(float dx, float dy, float sensitivity = 0.1f) {
        yaw   += dx * sensitivity;
        pitch -= dy * sensitivity; // dy is inverted: moving mouse up = positive dy in SDL
        pitch  = glm::clamp(pitch, -89.f, 89.f); // prevent gimbal lock at poles
    }

    // View matrix: transforms world-space positions into camera-space.
    // Upload this to the "view" uniform in the shader every frame.
    glm::mat4 viewMatrix() const {
        return glm::lookAt(position, position + forward(), glm::vec3{0, 1, 0});
    }

    // Projection matrix: applies perspective (things far away look small).
    // Upload this to the "projection" uniform. Only needs to change on FOV or
    // aspect ratio change, but cheap to recompute every frame.
    glm::mat4 projectionMatrix() const {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }
};
