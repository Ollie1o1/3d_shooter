#version 330 core
in vec3 vWorld;
out vec4 FragColor;
uniform vec3 uViewPos;
uniform vec3 uOrigin;
uniform vec3 uNormal;
uniform float uTime;
uniform float uAnomaly;
uniform float uMode;
// Thin imperfect observation glass. The persistent reflected service grid deliberately
// disagrees with the exterior seen through the admissions window.
void main(){
    vec3 p=vWorld-uOrigin;
    vec3 ray=normalize(uViewPos-vWorld);
    float fresnel=pow(1.0-abs(dot(ray,uNormal)),4.0);
    vec2 uv=abs(uNormal.y)>.5?p.xz:p.xy;
    float seam=pow(max(0.0,cos((uv.x+ray.x*3.0)*1.3)),100.0)*.32;
    seam+=pow(max(0.0,cos((uv.y+ray.y*3.0)*1.2)),120.0)*.18;
    float glare=pow(max(0.0,dot(ray,normalize(vec3(.4,.8,.2)))),35.0)*.22;
    vec3 color=vec3(.45,.7,.76);float alpha=.018+fresnel*.12+seam*.1+glare;
    if(uMode>.5 && uMode<1.5){
        float ripple=.5+.5*sin(uv.x*.6+sin(uv.y*.8+uTime*.15)*2.0);
        color=mix(vec3(.015,.09,.13),vec3(.08,.28,.32),ripple);
        // Ordinary horizontal ward lighting stays reflected over the impossible water.
        color+=vec3(.3,.5,.5)*seam;
        alpha=uAnomaly*.92;
    }else if(uMode>1.5){alpha*=.7;}
    FragColor=vec4(color,alpha);
}
