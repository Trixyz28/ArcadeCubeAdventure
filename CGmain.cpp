// This has been adapted from the Vulkan tutorial

// TO MOVE
#define JSON_DIAGNOSTICS 1
#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"



std::vector<SingleText> outText = {
	{1, {"Third Person", "Press 1 to change view", "", ""}, 0, 0},
	{1, {"First Person", "Press 1 to change view", "", ""}, 0, 0}
};


// Descriptor Buffers: data structure to be sent to the shader
// Alignments: float 4, vec2 8, vec3/4 mat3/4 16
struct UniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

struct GlobalUniformBufferObject {
	/*
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 eyePos;
	alignas(16) glm::vec4 eyeDir;
	*/
	struct {
		alignas(16) glm::vec3 v;
	} lightDir[2];
	struct {
		alignas(16) glm::vec3 v;
	} lightPos[2];
	alignas(16) glm::vec4 lightColor[2];
	alignas(4) float cosIn;
	alignas(4) float cosOut;
	alignas(16) glm::vec3 eyePos;
	alignas(16) glm::vec4 eyeDir;
	alignas(16) glm::vec2 lightOn;
};



// The vertices data structures
struct Vertex {
	glm::vec3 pos;
	glm::vec2 UV;
	glm::vec3 norm;
};


#include "modules/Scene.hpp"

// MAIN ! 
class CGmain: public BaseProject {
	protected:
	
	// Descriptor Layouts ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSL, DSLBlinn;

	// Vertex formats
	VertexDescriptor VD;

	// Pipelines [Shader couples]
	Pipeline P, PBlinn;

	Scene SC;
	glm::vec3 **deltaP;
	std::vector<PipelineRef> PRs;

	TextMaker txt;

	// Draw the main 
	std::vector<std::string> mainObj = {"tb"};

	// Models for first-person view
	// std::vector<std::string> trkOpEl = {"tbo", "tswo"};

	// Landscape drawing
	std::vector<std::string> staticObj =  {"pln", "prm", "cube"};


	// Aspect ratio
	float Ar;

	// Main application parameters
	void setWindowParameters() {
		// window size, title and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "CG - Project";
    	windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.0f, 0.85f, 1.0f, 1.0f};
		
		/*
		// Descriptor pool sizes
		uniformBlocksInPool = 19 * 2 + 2;
		texturesInPool = 19 + 1;
		setsInPool = 19 + 1;
		*/
		

		Ar = 4.0f / 3.0f;
	}

	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		std::cout << "Window resized to: " << w << " x " << h << "\n";
		Ar = (float)w / (float)h;
	}


	// Other application parameters
	float Yaw;
	glm::vec3 cubePosition;


	// currScene = 0: third person view, currScene = 1: first person view
	int currScene = 0;

	
	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSL.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(UniformBufferObject)},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
					{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject)}
				});

		DSLBlinn.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(UniformBufferObject)},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
					{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject)}
			});

		// Vertex descriptors
		VD.init(this, {
				  {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
				}, {
				  {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
				         sizeof(glm::vec3), POSITION},
				  {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
				         sizeof(glm::vec2), UV},
				  {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm),
				         sizeof(glm::vec3), NORMAL}
				});

		// Pipelines [Shader couples]
		P.init(this, &VD, "shaders/Vert.spv", "shaders/PhongFrag.spv", {&DSL});
		P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
 								    VK_CULL_MODE_NONE, false);
		PBlinn.init(this, &VD, "shaders/Vert.spv", "shaders/BlinnFrag.spv", { &DSLBlinn });


		PRs.resize(2);
		PRs[0].init("P", &P, &VD);
		PRs[1].init("PBlinn", &PBlinn, &VD);


		// Models, textures and Descriptors (values assigned to the uniforms)
/*		std::vector<Vertex> vertices = {
					   {{-100.0,0.0f,-100.0}, {0.0f,0.0f}, {0.0f,1.0f,0.0f}},
					   {{-100.0,0.0f, 100.0}, {0.0f,1.0f}, {0.0f,1.0f,0.0f}},
					   {{ 100.0,0.0f,-100.0}, {1.0f,0.0f}, {0.0f,1.0f,0.0f}},
					   {{ 100.0,0.0f, 100.0}, {1.0f,1.0f}, {0.0f,1.0f,0.0f}}};
		M1.vertices = std::vector<unsigned char>(vertices.size()*sizeof(Vertex), 0);
		memcpy(&vertices[0], &M1.vertices[0], vertices.size()*sizeof(Vertex));
		M1.indices = {0, 1, 2,    1, 3, 2};
		M1.initMesh(this, &VD); */

		uniformBlocksInPool = 0;
		texturesInPool = 0;
		setsInPool = 0;

		// Load Scene: the models are stored in json
		SC.init(this, &VD, DSL, PRs, "models/scene.json");
		
		// Updates the text
		txt.init(this, &outText);

		// Init local variables
		cubePosition = glm::vec3(SC.I[SC.InstanceIds["tb"]]->Wm[3]);
		Yaw = 0;
		
		deltaP = (glm::vec3 **)calloc(SC.InstanceCount, sizeof(glm::vec3 *));
		for(int i=0; i < SC.InstanceCount; i++) {
			deltaP[i] = new glm::vec3(SC.I[i]->Wm[3]);
		}

		std::cout << "Initialization completed!\n";
		std::cout << "Uniform Blocks in the Pool  : " << uniformBlocksInPool << "\n";
		std::cout << "Textures in the Pool        : " << texturesInPool << "\n";
		std::cout << "Descriptor Sets in the Pool : " << setsInPool << "\n";
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		// This creates a new pipeline (with the current surface), using its shaders
		P.create();
		PBlinn.create();

		// Here you define the data set
		SC.pipelinesAndDescriptorSetsInit(DSL);
		txt.pipelinesAndDescriptorSetsInit();
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup pipelines
		P.cleanup();

		SC.pipelinesAndDescriptorSetsCleanup();
		txt.pipelinesAndDescriptorSetsCleanup();
	}

	// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	// You also have to destroy the pipelines: since they need to be rebuilt, they have two different
	// methods: .cleanup() recreates them, while .destroy() delete them completely
	void localCleanup() {	
		/*
		for(int i=0; i < SC.InstanceCount; i++) {
			delete deltaP[i];
		}
		free(deltaP);*/

		// Cleanup descriptor set layouts
		DSL.cleanup();
		
		// Destroies the pipelines
		P.destroy();		

		SC.localCleanup();		
		txt.localCleanup();		
	}




	glm::mat4 MakeViewProjectionLookInDirection(glm::vec3 Pos, float Yaw, float Pitch, float Roll, float FOVy, float Ar, float nearPlane, float farPlane) {
		// Create a View Projection Matrix with the following characteristics:
		// Projection:
		//	- Perspective with:
		//	- Fov-y defined in formal parameter >FOVy<
		//  - Aspect ratio defined in formal parameter >Ar<
		//  - Near Plane distance defined in formal parameter >nearPlane<
		//  - Far Plane distance defined in formal parameter >farPlane<
		// View:
		//	- Use the Look-In-Direction model with:
		//	- Camera Positon defined in formal parameter >Pos<
		//	- Looking direction defined in formal parameter >Yaw<
		//	- Looking elevation defined in formal parameter >Pitch<
		//	- Looking rool defined in formal parameter >Roll<

		glm::mat4 Mp = glm::mat4(1.0f / (Ar * glm::tan(FOVy / 2.0f)), 0, 0, 0,
			0, -1.0f / glm::tan(FOVy / 2.0f), 0, 0,
			0, 0, farPlane / (nearPlane - farPlane), -1,
			0, 0, (nearPlane * farPlane) / (nearPlane - farPlane), 0);

		glm::mat4 Mv = glm::rotate(glm::mat4(1.0f), -Roll, glm::vec3(0, 0, 1)) *
			glm::rotate(glm::mat4(1.0f), -Pitch, glm::vec3(1, 0, 0)) *
			glm::rotate(glm::mat4(1.0f), -Yaw, glm::vec3(0, 1, 0)) *
			glm::translate(glm::mat4(1.0f), -Pos);

		glm::mat4 M = Mp * Mv;

		return M;
	}

	glm::mat4 MakeViewProjectionLookAt(glm::vec3 Pos, glm::vec3 Target, glm::vec3 Up, float Roll, float FOVy, float Ar, float nearPlane, float farPlane) {
		// Create a View Projection Matrix with the following characteristics:
		// Projection:
		//	- Perspective with:
		//	- Fov-y defined in formal parameter >FOVy<
		//  - Aspect ratio defined in formal parameter >Ar<
		//  - Near Plane distance defined in formal parameter >nearPlane<
		//  - Far Plane distance defined in formal parameter >farPlane<
		// View:
		//	- Use the Look-At model with:
		//	- Camera Positon defined in formal parameter >Pos<
		//	- Camera Target defined in formal parameter >Target<
		//	- Up vector defined in formal parameter >Up<
		//	- Looking rool defined in formal parameter >Roll<

		glm::mat4 Mp = glm::mat4(1.0f / (Ar * glm::tan(FOVy / 2.0f)), 0, 0, 0,
			0, -1.0f / glm::tan(FOVy / 2.0f), 0, 0,
			0, 0, farPlane / (nearPlane - farPlane), -1,
			0, 0, (nearPlane * farPlane) / (nearPlane - farPlane), 0);

		glm::mat4 Mv = glm::rotate(glm::mat4(1.0f), -Roll, glm::vec3(0, 0, 1)) * glm::lookAt(Pos, Target, Up);

		glm::mat4 M = Mp * Mv;

		return M;
	}

	glm::mat4 MakeWorld(glm::vec3 Pos, float Yaw, float Pitch, float Roll) {
		// Create a World Matrix with the following characteristics:
		//	- Object Positon defined in formal parameter >Pos<
		//	- Euler angle rotation yaw defined in formal parameter >Yaw<
		//	- Euler angle rotation pitch defined in formal parameter >Pitch<
		//	- Euler angle rotation roll defined in formal parameter >Roll<
		//  - Scaling constant and equal to 1 (and not passed to the procedure)
		glm::mat4 M = glm::translate(glm::mat4(1.0f), Pos) *
			glm::rotate(glm::mat4(1.0f), Yaw, glm::vec3(0, 1, 0)) *
			glm::rotate(glm::mat4(1.0f), Pitch, glm::vec3(1, 0, 0)) *
			glm::rotate(glm::mat4(1.0f), Roll, glm::vec3(0, 0, 1));
		// * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));

		return M;
	}

	
	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		

/*		// binds the data set
		DS1.bind(commandBuffer, P, 0, currentImage);
					
		// binds the model
		M1.bind(commandBuffer);
		
		// record the drawing command in the command buffer
		vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(M1.indices.size()), 1, 0, 0, 0);
		std::cout << M1.indices.size();
*/
		SC.populateCommandBuffer(commandBuffer, currentImage);
		txt.populateCommandBuffer(commandBuffer, currentImage, currScene);
	}


	void getTime(float& deltaT) {
		static auto startTime = std::chrono::high_resolution_clock::now();
		static float lastTime = 0.0f;

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - startTime).count();
		deltaT = time - lastTime;
		lastTime = time;
	}

	// Control cube's movements
	void getCubeDirection(glm::vec3 &cubeVec) {

		// Left 
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			cubeVec.x = -1.0f;
		}

		// Right
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			cubeVec.x = 1.0f;
		}

		// Forward
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			cubeVec.z = 1.0f;
		}

		// Backward
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			cubeVec.z = -1.0f;
		}

		// Up
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			cubeVec.y = 1.0f;
		}

		// Down
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			cubeVec.y = -1.0f;
		}
	}

	
	// Control the visual direction
	void getCameraDirection(glm::vec3 &viewVec) {
		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			viewVec.y = -1.0f;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			viewVec.y = 1.0f;
		}
		if (glfwGetKey(window, GLFW_KEY_UP)) {
			viewVec.x = -1.0f;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			viewVec.x = 1.0f;
		}
	}


	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		static bool debounce = false;
		static int curDebounce = 0;

		// Time control
		float deltaT;
		getTime(deltaT);

		glm::vec3 cubeDirection = glm::vec3(0.0f);
		getCubeDirection(cubeDirection);

		glm::vec3 cameraDirection = glm::vec3(0.0f);
		getCameraDirection(cameraDirection);
		
		static float cameraPitch = glm::radians(20.0f);
		static float cameraYaw   = M_PI;
		static float cameraRoll = 0.0f;
		static float cameraDistance  = 10.0f;

		const glm::vec3 CamTargetDelta = glm::vec3(0,2,0);
		const glm::vec3 Cam1stPos = glm::vec3(0.5f, 2.0f, 2.5f);
		
		static float rotationAngle = 0.0f;
		static float dampedVel = 0.0f;

		
// printVec3("m: ", m);
// printVec3("r: ", r);

		// Speed of rotating direction of move
		const float rotationSpeed = glm::radians(100.0f);

		// Speed of moving
		const float moveSpeed = 4.0f;

		const float FLY_SPEED = 0.05f;

		// rotation angle
		rotationAngle += -cubeDirection.x * deltaT;
		

		glm::mat4 M;
		glm::vec3 CamPos = cubePosition;
		static glm::vec3 dampedCamPos = CamPos;

		double lambdaVel = 8.0f;
		double dampedVelEpsilon = 0.001f;
		double flyVel = 0.0f;
		dampedVel =  moveSpeed * deltaT * cubeDirection.z * (1 - exp(-lambdaVel * deltaT)) +
					 dampedVel * exp(-lambdaVel * deltaT);
		dampedVel = ((fabs(dampedVel) < dampedVelEpsilon) ? 0.0f : dampedVel);


		flyVel = FLY_SPEED * cubeDirection.y;

		if(dampedVel != 0.0f) {
			glm::vec3 oldPos = cubePosition;
			
			if(rotationAngle != 0.0f) {
				const float l = 2.78f;
				float r = l / tan(rotationAngle);
				float cx = cubePosition.x + r * cos(Yaw);
				float cz = cubePosition.z - r * sin(Yaw);
				float Dbeta = dampedVel / r;
				Yaw = Yaw - Dbeta;
				cubePosition.x = cx - r * cos(Yaw);
				cubePosition.z = cz + r * sin(Yaw);
			} else {
				cubePosition.x = cubePosition.x - sin(Yaw) * dampedVel;
				cubePosition.z = cubePosition.z - cos(Yaw) * dampedVel;
				cubePosition.y += flyVel;
			}
			if(cubeDirection.x == 0) {
				if(rotationAngle > deltaT) {
					rotationAngle -= deltaT;
				} else if(rotationAngle < - deltaT) {
					rotationAngle += deltaT;
				} else {
					rotationAngle = 0.0f;
				}					
			}

			glm::vec3 deltaPos = cubePosition - oldPos;

		}
		

		// Press 1 to change between third and first person view
		if(glfwGetKey(window, GLFW_KEY_1)) {
			if(!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_1;
				if(currScene == 0) {
					currScene = 1;
					
					cameraPitch = glm::radians(20.0f);
					cameraYaw   = M_PI;
					cameraRoll  = 0.0f;
					dampedCamPos = cubePosition;
				}
				else {
					currScene = 0;
					cameraPitch = glm::radians(0.0f);
					cameraYaw = M_PI;
					cameraRoll = 0.0f;
				}

				std::cout << "Scene : " << currScene << "\n";
				RebuildPipeline();
			}
		} else {
			if((curDebounce == GLFW_KEY_1) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		// Standard procedure to quit when the ESC key is pressed
		if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}


		if(glfwGetKey(window, GLFW_KEY_V)) {
			if(!debounce) {
				debounce = true;
				curDebounce = GLFW_KEY_V;

				printVec3("Pos = ", cubePosition);
				std::cout << "Yaw         = " << Yaw         << ";\n";
				std::cout << "CamPitch    = " << cameraPitch    << ";\n";
				std::cout << "CamYaw      = " << cameraYaw      << ";\n";
				std::cout << "CamRoll     = " << cameraRoll     << ";\n";
				std::cout << "CamDist     = " << cameraDistance     << ";\n";
				std::cout << "SteeringAng = " << rotationAngle << ";\n";

			}
		} else {
			if((curDebounce == GLFW_KEY_V) && debounce) {
				debounce = false;
				curDebounce = 0;
			}
		}

		
		if(currScene == 0) {
			cameraYaw += rotationSpeed * deltaT * cameraDirection.y;
			cameraPitch -= rotationSpeed * deltaT * cameraDirection.x;
			cameraRoll -= rotationSpeed * deltaT * cameraDirection.z;
			cameraDistance -= moveSpeed * deltaT * cubeDirection.y;
		
			cameraYaw = (cameraYaw < 0.0f ? 0.0f : (cameraYaw > 2*M_PI ? 2*M_PI : cameraYaw));
			cameraPitch = (cameraPitch < 0.0f ? 0.0f : (cameraPitch > M_PI_2-0.01f ? M_PI_2-0.01f : cameraPitch));
			cameraRoll = (cameraRoll < -M_PI ? -M_PI : (cameraRoll > M_PI ? M_PI : cameraRoll));
			cameraDistance = (cameraDistance < 7.0f ? 7.0f : (cameraDistance > 15.0f ? 15.0f : cameraDistance));
				
			glm::vec3 CamTarget = cubePosition + glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0,1,0)) *
							 glm::vec4(CamTargetDelta,1));
			CamPos = CamTarget + glm::vec3(glm::rotate(glm::mat4(1), Yaw + cameraYaw, glm::vec3(0,1,0)) * glm::rotate(glm::mat4(1), -cameraPitch, glm::vec3(1,0,0)) * 
							 glm::vec4(0,0,cameraDistance,1));

			const float lambdaCam = 10.0f;
			dampedCamPos = CamPos * (1 - exp(-lambdaCam * deltaT)) +
						 dampedCamPos * exp(-lambdaCam * deltaT); 
			M = MakeViewProjectionLookAt(dampedCamPos, CamTarget, glm::vec3(0,1,0), cameraRoll, glm::radians(90.0f), Ar, 0.1f, 500.0f);
		} else {

			cameraYaw -= rotationSpeed * deltaT * cameraDirection.y;
			cameraPitch -= rotationSpeed * deltaT * cameraDirection.x;
			cameraRoll -= rotationSpeed * deltaT * cameraDirection.z;
		
			cameraYaw = (cameraYaw < M_PI_2 ? M_PI_2 : (cameraYaw > 1.5*M_PI ? 1.5*M_PI : cameraYaw));
			cameraPitch = (cameraPitch < -0.25*M_PI ? -0.25*M_PI : (cameraPitch > 0.25*M_PI ? 0.25*M_PI : cameraPitch));
			cameraRoll = (cameraRoll < -M_PI ? -M_PI : (cameraRoll > M_PI ? M_PI : cameraRoll));
				
			glm::vec3 Cam1Pos = cubePosition + glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0,1,0)) *
							 glm::vec4(Cam1stPos,1));
			M = MakeViewProjectionLookInDirection(Cam1Pos, Yaw + cameraYaw, cameraPitch, cameraRoll, glm::radians(90.0f), Ar, 0.1f, 500.0f);
		} 

		glm::mat4 ViewPrj =  M;
		UniformBufferObject ubo{};
		glm::mat4 baseTr = glm::mat4(1.0f);								
		// Here is where you actually update your uniforms
		/*
		// updates global uniforms
		GlobalUniformBufferObject gubo{};
		gubo.lightDir = glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 0.0f);
		gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		gubo.eyePos = dampedCamPos;
		gubo.eyeDir = glm::vec4(0);
		gubo.eyeDir.w = 1.0;
		*/
		GlobalUniformBufferObject gubo{};
		gubo.lightDir[0].v = glm::vec3(cos(glm::radians(135.0f)), sin(glm::radians(135.0f)), 0.0f);
		gubo.lightPos[0].v = glm::vec3(1.0f);
		gubo.lightColor[0] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		gubo.lightDir[1].v = glm::normalize(glm::vec3(dampedCamPos.x- cubePosition.x, 0.0f, dampedCamPos.z - cubePosition.z));
		gubo.lightPos[1].v = cubePosition;
		gubo.lightColor[1] = glm::vec4(0.0f, 0.5f, 1.0f, 5.0f);
		gubo.eyePos = dampedCamPos;
		gubo.eyeDir = glm::vec4(0);
		gubo.eyeDir.w = 1.0;
		gubo.lightOn = glm::vec2(1.0f,1.0f);
		gubo.cosIn = cos(0.4591524628390111f); ;
		gubo.cosOut = cos(0.5401793718338013f);

//		for(int i = 0; i < SC.InstanceCount; i++) {
		// Draw the truck
		if(currScene == 0) {
			for (std::vector<std::string>::iterator it = mainObj.begin(); it != mainObj.end(); it++) {
				int i = SC.InstanceIds[it->c_str()];
				glm::vec3 dP = glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0,1,0)) *
										 glm::vec4(*deltaP[i],1));
				ubo.mMat = MakeWorld(cubePosition + dP, Yaw, 0.0f, 0) * baseTr * SC.I[i]->Wm * SC.M[SC.I[i]->Mid]->Wm;
				ubo.mvpMat = ViewPrj * ubo.mMat;
				ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));


				SC.I[i]->DS[0]->map(currentImage, &ubo, sizeof(ubo), 0);
				SC.I[i]->DS[0]->map(currentImage, &gubo, sizeof(gubo), 2);
			}

			/*
			for (std::vector<std::string>::iterator it = trkOpEl.begin(); it != trkOpEl.end(); it++) {
				int i = SC.InstanceIds[it->c_str()];
				ubo.mMat = glm::mat4(0);
				ubo.mvpMat = glm::mat4(0);
				ubo.nMat = glm::mat4(0);

				SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
				SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
			}*/
		} else {

			/*
			for (std::vector<std::string>::iterator it = trkOpEl.begin(); it != trkOpEl.end(); it++) {
				int i = SC.InstanceIds[it->c_str()];
				float extraYaw = ((i == SC.InstanceIds["tswo"]) ? -SteeringAng * 15.0f: 0);
				float extraPitch = ((i == SC.InstanceIds["tswo"]) ? glm::radians(20.0f) : 0);
				glm::vec3 dP = glm::vec3(glm::rotate(glm::mat4(1), Yaw, glm::vec3(0,1,0)) *
										 glm::vec4(*deltaP[i],1));
				ubo.mMat = MakeWorld(Pos + dP, Yaw, 0.0f + extraPitch, extraYaw) * baseTr;
				ubo.mvpMat = ViewPrj * ubo.mMat;
				ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

				SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
				SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
			}
			*/
			for (std::vector<std::string>::iterator it = mainObj.begin(); it != mainObj.end(); it++) {
				int i = SC.InstanceIds[it->c_str()];
				ubo.mMat = glm::mat4(0);
				ubo.mvpMat = glm::mat4(0);
				ubo.nMat = glm::mat4(0);

				SC.I[i]->DS[0]->map(currentImage, &ubo, sizeof(ubo), 0);
				SC.I[i]->DS[0]->map(currentImage, &gubo, sizeof(gubo), 2);
			}
		}

		// Draw the landscape
		for (std::vector<std::string>::iterator it = staticObj.begin(); it != staticObj.end(); it++) {
			int i = SC.InstanceIds[it->c_str()];
//std::cout << *it << " " << i << "\n";
			// Product per transform matrix
			ubo.mMat = SC.I[i]->Wm * baseTr ;
			ubo.mvpMat = ViewPrj * ubo.mMat;
			ubo.nMat = glm::inverse(glm::transpose(ubo.mMat));

			SC.I[i]->DS[0]->map(currentImage, &ubo, sizeof(ubo), 0);
			SC.I[i]->DS[0]->map(currentImage, &gubo, sizeof(gubo), 2);
		}
	}
};

// The main function of the application, do not touch
int main() {
    CGmain app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}