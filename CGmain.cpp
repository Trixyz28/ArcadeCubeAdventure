// This has been adapted from the Vulkan tutorial

#define JSON_DIAGNOSTICS 1
#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"



std::vector<SingleText> outText = {
	{1, {"Third Person", ":)", "", ""}, 0, 0},
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
	DescriptorSetLayout DSL, DSLcube;

	// Vertex formats
	VertexDescriptor VD;

	// Pipelines [Shader couples]
	Pipeline P, Pcube;

	Scene SC;
	std::vector<PipelineRef> PRs;

	TextMaker txt;

	std::string cubeObj = "tb";

	// Landscape drawing
	std::vector<std::string> staticObj =  {"pln", "prm", "cube"};

	UniformBufferObject cubeUbo{};
	UniformBufferObject staticUbo{};

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
	
	// currScene = 0: third person view, currScene = 1: first person view
	int currScene = 0;
	

	
	glm::vec3 cubePosition;
	float cubeRotAngle;
	float cubeMovSpeed, cubeRotSpeed;
	
	glm::vec3 camPosition, camRotation;
	float camRotSpeed;
	float camDistance;
	float minCamDistance, maxCamDistance;
	glm::mat4 viewMatrix;
	
	





	
	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts and load the shaders for the pipelines
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSL.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(UniformBufferObject)},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
					{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject)}
				});

		DSLcube.init(this, {
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
		Pcube.init(this, &VD, "shaders/Vert.spv", "shaders/BlinnFrag.spv", { &DSLcube });


		PRs.resize(2);
		PRs[0].init("P", &P, &VD);
		PRs[1].init("PBlinn", &Pcube, &VD);


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
		cubePosition = glm::vec3(0.0f, 0.5f, 0.0f);
		cubeRotAngle = 0.0f;
		cubeMovSpeed = 0.01f;
		cubeRotSpeed = 0.2f;


		camPosition = cubePosition + glm::vec3(3.0f, 2.5f, 3.0f);
		camRotation = glm::vec3(0.0f);
		camRotSpeed = 0.1f;
		camDistance = 2.0f;
		minCamDistance = 1.0f;
		maxCamDistance = 3.0f;
		
		viewMatrix = glm::lookAt(camPosition, cubePosition, glm::vec3(0.0f, 1.0f, 0.0f));




		std::cout << "Initialization completed!\n";
		std::cout << "Uniform Blocks in the Pool  : " << uniformBlocksInPool << "\n";
		std::cout << "Textures in the Pool        : " << texturesInPool << "\n";
		std::cout << "Descriptor Sets in the Pool : " << setsInPool << "\n";
	}
	
	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit() {
		// This creates a new pipeline (with the current surface), using its shaders
		P.create();
		Pcube.create();

		// Here you define the data set
		SC.pipelinesAndDescriptorSetsInit(DSL);
		txt.pipelinesAndDescriptorSetsInit();
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup pipelines
		P.cleanup();
		Pcube.cleanup();

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
		DSLcube.cleanup();
		
		// Destroies the pipelines
		P.destroy();
		Pcube.destroy();

		SC.localCleanup();		
		txt.localCleanup();		
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


	void getTime(float &deltaT) {
		static auto startTime = std::chrono::high_resolution_clock::now();
		static float lastTime = 0.0f;

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - startTime).count();
		deltaT = time - lastTime;
		lastTime = time;
	}

	// Control cube's movements
	void getCubeAction() {

		// Left 
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			cubeRotAngle += cubeRotSpeed;
		}

		// Right
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			cubeRotAngle -= cubeRotSpeed;
		}
		
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			cubeRotAngle += cubeRotSpeed;
			camRotation.x += cubeRotSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			cubeRotAngle -= cubeRotSpeed;
			camRotation.x -= cubeRotSpeed;
		}
		
		// Forward
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			cubePosition.x += cubeMovSpeed * glm::sin(glm::radians(cubeRotAngle));
			cubePosition.z += cubeMovSpeed * glm::cos(glm::radians(cubeRotAngle));
		}

		// Backward
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			cubePosition.x -= cubeMovSpeed * glm::sin(glm::radians(cubeRotAngle));
			cubePosition.z -= cubeMovSpeed * glm::cos(glm::radians(cubeRotAngle));
		}

		/*
		// Up
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			cubePosition.y += 1.0f;
		}

		// Down
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			cubePosition.y -= 1.0f;
		}*/


		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			camRotation.y += camRotSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			camRotation.y -= camRotSpeed;
		}

		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			camDistance -= 0.01f;
		}
		if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
			camDistance += 0.01f;
		}

		camDistance = glm::clamp(camDistance, minCamDistance, maxCamDistance);
		camRotation.y = glm::clamp(camRotation.y, 0.0f, 80.0f);

	}

	
	// Control the visual direction
	void getCamAction() {

	}


	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {

		static bool debounce = false;
		static int curDebounce = 0;


		// Standard procedure to quit when the ESC key is pressed
		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}


		const float fovY = glm::radians(90.0f);
		const float nearPlane = 0.1f;
		const float farPlane = 200.0f;

		glm::mat4 prjMatrix = glm::mat4(1.0f / (Ar * glm::tan(fovY / 2.0f)), 0, 0, 0,
			0, -1.0f / glm::tan(fovY / 2.0f), 0, 0,
			0, 0, farPlane / (nearPlane - farPlane), -1,
			0, 0, (nearPlane * farPlane) / (nearPlane - farPlane), 0);

		glm::mat4 baseMatrix = glm::mat4(1.0f);
		glm::mat4 worldMatrix;
		glm::mat4 viewPrjMatrix = prjMatrix * viewMatrix;


		// Time control
		float deltaT;
		getTime(deltaT);


		/*
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
		*/
		
					
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
		gubo.lightDir[1].v = glm::normalize(glm::vec3(camPosition.x- cubePosition.x, 0.0f, camPosition.z - cubePosition.z));
		gubo.lightPos[1].v = cubePosition;
		gubo.lightColor[1] = glm::vec4(0.0f, 0.5f, 1.0f, 5.0f);
		gubo.eyePos = camPosition;
		gubo.eyeDir = glm::vec4(0);
		gubo.eyeDir.w = 1.0;
		gubo.lightOn = glm::vec2(1.0f,1.0f);
		gubo.cosIn = cos(0.4591524628390111f);
		gubo.cosOut = cos(0.5401793718338013f);

//		for(int i = 0; i < SC.InstanceCount; i++) {
		if(currScene == 0) {


			/*
			for (std::vector<std::string>::iterator it = trkOpEl.begin(); it != trkOpEl.end(); it++) {
				int i = SC.InstanceIds[it->c_str()];
				ubo.mMat = glm::mat4(0);
				ubo.mvpMat = glm::mat4(0);
				ubo.nMat = glm::mat4(0);

				SC.DS[i]->map(currentImage, &ubo, sizeof(ubo), 0);
				SC.DS[i]->map(currentImage, &gubo, sizeof(gubo), 2);
			}*/
		} 

		// Draw the landscape
		for (std::vector<std::string>::iterator it = staticObj.begin(); it != staticObj.end(); it++) {
			int i = SC.InstanceIds[it->c_str()];
//std::cout << *it << " " << i << "\n";
			// Product per transform matrix
			staticUbo.mMat = baseMatrix * SC.I[i]->Wm;
			staticUbo.mvpMat = viewPrjMatrix * staticUbo.mMat;
			staticUbo.nMat = glm::inverse(glm::transpose(staticUbo.mMat));

			SC.I[i]->DS[0]->map(currentImage, &staticUbo, sizeof(staticUbo), 0);
			SC.I[i]->DS[0]->map(currentImage, &gubo, sizeof(gubo), 2);
		}

		getCubeAction();



		getCamAction();


		
		worldMatrix = glm::translate(glm::mat4(1.0f), cubePosition);
		worldMatrix *= glm::rotate(glm::mat4(1.0f), glm::radians(cubeRotAngle),
			glm::vec3(0.0f, 1.0f, 0.0f));

		camPosition = glm::normalize(glm::vec3(sin(glm::radians(cubeRotAngle)) * 0.5f,
			sin(glm::radians(camRotation.y)),
			cos(glm::radians(cubeRotAngle)) *0.5f)) * camDistance + cubePosition;

		viewMatrix = glm::lookAt(camPosition, cubePosition, glm::vec3(0.0f, 1.0f, 0.0f));


		int i = SC.InstanceIds[cubeObj];
		cubeUbo.mMat = baseMatrix * worldMatrix * SC.M[SC.I[i]->Mid]->Wm * SC.I[i]->Wm;
		cubeUbo.mvpMat = viewPrjMatrix * cubeUbo.mMat;
		cubeUbo.nMat = glm::inverse(glm::transpose(cubeUbo.mMat));

		SC.I[i]->DS[0]->map(currentImage, &cubeUbo, sizeof(cubeUbo), 0);
		SC.I[i]->DS[0]->map(currentImage, &gubo, sizeof(gubo), 2);


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