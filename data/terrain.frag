#version 330 core

in vec3 pos;
out vec4 color;

uniform float minHeight;
uniform float maxHeight;
uniform float seaLevel;
uniform float regionAlpha;
uniform vec2 regionMin;
uniform vec2 regionMax;
uniform vec2 selectedPoint;
uniform float pointSize;
uniform sampler1D heightPalette;
uniform vec3 lightDir;
uniform float shadingFactor;

const vec3 REGION_COLOR = vec3(1, 0, 0);
const vec3 SELECT_COLOR = vec3(0, 1, 1);


void main() {
	if (pos.z < seaLevel) discard;
	
	// normal estimation and shading
	vec3 dx = dFdx(pos);
	vec3 dy = dFdy(pos);
	vec3 N = normalize(cross(dx, dy));
	float shade = clamp(dot(N, lightDir), 0, 1);
	
	// height based color
	float h = (pos.z - minHeight)/(maxHeight - minHeight);
	vec3 col = texture(heightPalette, h).rgb;	
	col = col*(1 - shadingFactor) + shade*col*shadingFactor;	
	
	// selected region
	float alpha = regionAlpha;
	alpha *= step(regionMin.x, pos.x) * step(regionMin.y, pos.y);
	alpha *= (1-step(regionMax.x, pos.x)) * (1-step(regionMax.y, pos.y));
	col = REGION_COLOR*alpha + col*(1-alpha);	
	
	// point
	float pselDist = distance(pos.xy, selectedPoint);
	alpha = 0.8 * pow(1-smoothstep(0, pointSize, pselDist), 2);
	col = SELECT_COLOR*alpha + col*(1-alpha);	
	
    color = vec4(col, 1);
}

