#pragma once

class Shader;
class Camera;

namespace render {

void applyDayNightLighting(Shader& shader, Camera& camera, float currentFrame,
                           unsigned int screenWidth, unsigned int screenHeight);

} // namespace render
