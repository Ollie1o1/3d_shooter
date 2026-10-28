#version 330 core
// A picture in the menus (MenuArt.h): its texture times a tint (alpha fades it)
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uImage;
uniform vec4 uTint;
void main() {
    FragColor = texture(uImage, vUV) * uTint;
}
