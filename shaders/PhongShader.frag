#version 450#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;layout(location = 1) in vec3 fragNorm;layout(location = 2) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(set = 1, binding = 1) uniform sampler2D texSampler;
layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
	vec3 lightDir[4]; 
	vec3 lightPos[4];
	vec4 lightColor[4];
	float cosIn;	float cosOut;
	vec3 eyePos;	vec4 eyeDir;	vec3 lightOn;	} gubo;		
vec3 direct_light_dir(vec3 pos, int i) {	return normalize(gubo.lightDir[i]) ;}vec3 direct_light_color(vec3 pos, int i) {	return gubo.lightColor[i].rgb;}vec3 point_light_dir(vec3 pos, int i) {	return normalize(gubo.lightPos[i] - pos);}vec3 point_light_color(vec3 pos, int i) {	return gubo.lightColor[i].rgb*pow((gubo.lightColor[i].a/length(gubo.lightPos[i]-pos)), 2.0f);}vec3 spot_light_dir(vec3 pos, int i) {	return normalize(gubo.lightPos[i] - pos);}vec3 spot_light_color(vec3 pos, int i) {	return
		gubo.lightColor[i].rgb*
		pow(
			(gubo.lightColor[i].a/length(gubo.lightPos[i]-pos))
			, 2.0f
		)* vec3(
			clamp(
			(
				dot(
					normalize(gubo.lightPos[i] - pos), 
					gubo.lightDir[i]
				) 
				- gubo.cosOut
			)/(gubo.cosIn-gubo.cosOut), 0.0f, 1.0f
			)
		)
		
	;}vec3 BRDF(vec3 V, vec3 Norm, vec3 LD, vec3 Md, vec3 Ms, float gamma) {	vec3 Diffuse;	vec3 Specular;	Diffuse = Md * clamp(dot(Norm, LD),0.0,1.0);	Specular = Ms* vec3(pow(clamp(dot(V, -reflect(LD, Norm)),0.0f,1.0f), gamma));		return Diffuse + Specular;}
void main() {	vec3 Norm = normalize(fragNorm);	vec3 EyeDir = normalize(gubo.eyePos - fragPos);	vec3 Albedo = texture(texSampler, fragTexCoord).rgb;	vec3 LD;	// light direction	vec3 LC;	// light color	vec3 RendEqSol = vec3(0);	// First light	LD = direct_light_dir(fragPos, 0);	LC = direct_light_color(fragPos, 0);	RendEqSol += BRDF(EyeDir, Norm, LD, Albedo, vec3(1.0f), 160.0f) * LC         * gubo.lightOn.x;	// Point lights	for(int i = 1; i < gubo.lightDir.length() - 1; i++){		LD = point_light_dir(fragPos, i);		LC = point_light_color(fragPos, i);		RendEqSol += BRDF(EyeDir, Norm, LD, Albedo, vec3(1.0f), 160.0f) * LC         * gubo.lightOn.z;	}		// Cube light	LD = spot_light_dir(fragPos, gubo.lightDir.length() -1);	LC = spot_light_color(fragPos, gubo.lightDir.length() -1);	RendEqSol += BRDF(EyeDir, Norm, LD, Albedo, vec3(1.0f), 160.0f) * LC         * gubo.lightOn.y;	// Indirect illumination simulation	// A special type of non-uniform ambient color, invented for this course	const vec3 cxp = vec3(1.0,0.5,0.5) * 0.15;	const vec3 cxn = vec3(0.9,0.6,0.4) * 0.15;	const vec3 cyp = vec3(0.3,1.0,1.0) * 0.15;	const vec3 cyn = vec3(0.5,0.5,0.5) * 0.15;	const vec3 czp = vec3(0.8,0.2,0.4) * 0.15;	const vec3 czn = vec3(0.3,0.6,0.7) * 0.15;		vec3 Ambient =((Norm.x > 0 ? cxp : cxn) * (Norm.x * Norm.x) +				   (Norm.y > 0 ? cyp : cyn) * (Norm.y * Norm.y) +				   (Norm.z > 0 ? czp : czn) * (Norm.z * Norm.z)) * Albedo;	RendEqSol += Ambient;		// Output color	outColor = vec4(RendEqSol, 1.0f);}