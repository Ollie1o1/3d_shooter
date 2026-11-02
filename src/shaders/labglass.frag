#version 330 core
in vec3 vWorld;
out vec4 FragColor;
uniform vec3 uViewPos;
uniform vec3 uOrigin;
uniform vec3 uNormal;
uniform float uTime;
uniform float uAnomaly;
uniform float uMode;
// Modes 3 (a lamp's light shaft) and 4 (the portal's vortex) are additive.
uniform vec3 uBeamA;
uniform vec3 uBeamB;
uniform vec2 uBeamR;
uniform vec3 uBeamColor;
float labHash(vec3 p){return fract(sin(dot(p,vec3(12.9898,78.233,37.719)))*43758.5453);}
// Thin imperfect observation glass. The persistent reflected service grid deliberately
// disagrees with the exterior seen through the admissions window.
void main(){
    if(uMode>2.5 && uMode<3.5){
        // A cone of lamp light: brightest down its axis and near the lamp, dust turning in it
        vec3 ab=uBeamB-uBeamA;float len=length(ab);vec3 d=ab/len;
        vec3 q=vWorld-uBeamA;float along=dot(q,d),t=clamp(along/len,0.0,1.0);
        float r=mix(uBeamR.x,uBeamR.y,t),dist=length(q-d*along);
        float core=1.0-clamp(dist/r,0.0,1.0);
        float a=core*core*(1.0-t*.7)*smoothstep(0.0,.05,t)*.13;
        vec3 cell=floor(vWorld*5.0+vec3(0.0,uTime*.35,0.0));
        float mote=step(.992,labHash(cell))*core*.35;
        FragColor=vec4(uBeamColor*(a+mote),1.0);return;
    }
    if(uMode>3.5){
        // The portal: two sets of logarithmic spiral arms turning inward, cyan
        // into violet, round a white-hot core. It reddens as the Successor weakens.
        vec2 q=(vWorld-uBeamA).xy/uBeamR.x;float rad=length(q);
        if(rad>1.0)discard;
        float ang=atan(q.y,q.x),lr=log(rad+.035);
        float s1=.5+.5*sin(2.0*ang+6.5*lr+uTime*1.1);
        float s2=.5+.5*sin(3.0*ang+10.0*lr+uTime*1.6+1.7);
        float grain=labHash(vec3(floor(q*60.0),floor(uTime*8.0)));
        vec3 cyan=vec3(.2,.95,1.25),violet=mix(vec3(.75,.25,1.3),vec3(1.4,.12,.3),uBeamColor.x);
        vec3 col=mix(cyan,violet,s2*.8+rad*.3)*(pow(s1,2.2)*.9+pow(s2,3.0)*.5)*smoothstep(1.0,.55,rad);
        col+=vec3(.95,1.0,1.25)*exp(-rad*rad*28.0)*1.6;
        col+=mix(cyan,violet,.5)*exp(-pow((rad-.94)*14.0,2.0))*.8;
        col*=.85+.3*grain*smoothstep(.2,.9,rad);
        FragColor=vec4(col*smoothstep(1.0,.97,rad),1.0);return;
    }
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
