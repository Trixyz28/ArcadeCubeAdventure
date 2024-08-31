// This has been adapted from the Vulkan tutorial

#define JSON_DIAGNOSTICS 1
#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"


// Vector of text
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

// UBO for the cube
struct CubeUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
	alignas(16) glm::vec3 col;
};

struct LightUniformBufferObject {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
	alignas(4) float id;
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
	} lightDir[4];
	struct {
		alignas(16) glm::vec3 v;
	} lightPos[4];
	alignas(16) glm::vec4 lightColor[4];
	alignas(4) float cosIn;
	alignas(4) float cosOut;
	alignas(16) glm::vec3 eyePos;
	alignas(16) glm::vec4 eyeDir;
	alignas(16) glm::vec3 lightOn;
};



// Data structure for the vertices
struct Vertex {
	glm::vec3 pos;
	glm::vec2 UV;
	glm::vec3 norm;
};

struct CubeCollider {
	glm::vec3 center;
	float length;
};


#include "modules/Scene.hpp"


// MAIN ! 
class CGmain : public BaseProject {
protected:

	// Descriptor Layouts ["classes" of what will be passed to the shaders]
	DescriptorSetLayout DSL, DSLcube, DSLlight;

	// Vertex formats
	VertexDescriptor VD;

	// Pipelines [Shader couples]
	Pipeline P, Pcube, Plight;

	// Scene
	Scene SC;
	std::vector<PipelineRef> PRs;

	TextMaker txt;

	std::string cubeObj = "cube";

	// Static elements of the scene to draw
	std::vector<std::string> staticObj = { 
		"floor", "ceiling", "leftwall", "rightwall", "frontwall", "backwall", 
		"redmachine1", "redmachine2", "redmachine3", "hockeytable", "pooltable", "poolsticks", "dancemachine1", "dancemachine2",
		"blackmachine1", "blackmachine2", "blackmachine3", "doublemachine1", "doublemachine2",
		"vendingmachine", "popcornmachine", "sign24h", "paintpacman", "sofa", "coffeetable",
		"bluepouf", "brownpouf", "yellowpouf", "frenchchips", "macaron", "drink1", "drink2", "drink3"
	};

	// Reward gadgets to draw
	std::vector<std::string> gadgetObj = { "diamond" };

	std::vector<std::string> lightObj = {  "light", "sign24h" };

	CubeUniformBufferObject cubeUbo{};
	UniformBufferObject staticUbo{};
	LightUniformBufferObject lightUbo{};


	// Aspect ratio of the application window
	float Ar;

	// Main application parameters
	void setWindowParameters() {

		// Window size, title and initial background
		windowWidth = 1920;
		windowHeight = 1080;
		windowTitle = "CG - Project";
		windowResizable = GLFW_TRUE;
		initialBackgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		Ar = (float)windowWidth / (float)windowHeight;
	}


	// What to do when the window changes size
	void onWindowResize(int w, int h) {
		std::cout << "Window resized to: " << w << " x " << h << "\n";
		Ar = (float)w / (float)h;
	}


	// Other application parameters

	// currScene = 0: third person view
	int currScene = 0;

	// Position and color of the cube
	glm::vec3 cubePosition;
	glm::vec3 cubeColor;
	//for collision management
	CubeCollider cubeCollider;

	// Moving speed, rotation speed and angle of the cube
	float cubeRotAngle, cubeRotSpeed;
	glm::vec3 cubeMovSpeed;

	// Position and rotation of the camera
	glm::vec3 camPosition, camRotation;
	// Rotation speed and the forward / backward speed of the camera
	float camRotSpeed, camNFSpeed;
	// Camera distance and constraints
	float camDistance, minCamDistance, maxCamDistance;
	
	// Jumping speed, initial acceleration, and in-air deceleration of the cube
	float jumpSpeed, jumpForce, gravity;
	// Jumping status of the cube
	bool isJumping;
	// Ground level of the position
	float groundLevel;

	// Maximum abs coordinate of the map (for both x and z axis)
	const float mapLimit = 23.94f;

	// Time offset to compensate different device performance
	float deltaTime;

	bool debounce;
	int currDebounce;
	glm::mat4 viewMatrix;
	glm::vec3 lPos[4];
	glm::vec3 lDir[4];
	glm::vec4 lCol[4];
	int n_lights;
	


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
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(CubeUniformBufferObject)},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
					{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject)}
			});

		DSLlight.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(LightUniformBufferObject)},
					{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
					{2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject)},
					{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1}
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
		P.init(this, &VD, "shaders/Vert.spv", "shaders/PhongFrag.spv", { &DSL });
		P.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_NONE, false);
		Pcube.init(this, &VD, "shaders/CubeVert.spv", "shaders/CubeFrag.spv", { &DSLcube });
		Plight.init(this, &VD, "shaders/LightVert.spv", "shaders/LightFrag.spv", { &DSLlight });

		PRs.resize(3);
		PRs[0].init("P", &P, &VD);
		PRs[1].init("PBlinn", &Pcube, &VD);
		PRs[2].init("PLight", &Plight, &VD);

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
		SC.init(this, &VD, PRs, "models/scene.json");

		// Updates the text
		txt.init(this, &outText);

		// Init local variables
		cubePosition = glm::vec3(0.0f, 0.0f, 0.0f);
		cubeRotAngle = 0.0f;
		cubeMovSpeed = glm::vec3(0.02f,0.02f,0.02f);
		cubeRotSpeed = 0.2f;
		cubeColor = glm::vec3(0.0f, 0.0f, 0.0f);


		camPosition = cubePosition + glm::vec3(0.0f, 0.06f, 0.0f);
		camRotation = glm::vec3(0.0f, 0.0f, 0.0f);
		camRotSpeed = 0.1f;
		camDistance = 0.4f;
		minCamDistance = 0.22f;
		maxCamDistance = 0.7f;

		jumpSpeed = 0.0f;
		isJumping = false;
		gravity = -0.0001f;
		jumpForce = 0.04f;
		groundLevel = 0.0f;
		camNFSpeed = 0.0003f;

		cubeCollider.center = cubePosition;
		cubeCollider.length = 100.0f;

		debounce = false;
		currDebounce = 0;

		deltaTime = getTime();

		viewMatrix = glm::lookAt(camPosition, cubePosition, glm::vec3(0.0f, 1.0f, 0.0f));

		// lights
		nlohmann::json js;
		std::ifstream ifs("models/Lights.json");
		if (!ifs.is_open()) {
			std::cout << "Error! Lights file not found!";
			exit(-1);
		}

		try {
			std::cout << "Parsing JSON\n";
			ifs >> js;
			ifs.close();
			nlohmann::json ld = js["lights"];
			n_lights = ld.size();
			std::cout << "There are " << n_lights << " lights.\n";
			for (int i = 0; i < ld.size(); i++) {
				lPos[i] = glm::vec3(ld[i]["position"][0], ld[i]["position"][1], ld[i]["position"][2]);
				lDir[i] = glm::vec3(ld[i]["direction"][0], ld[i]["direction"][1], ld[i]["direction"][2]);
				lCol[i] = glm::vec4(ld[i]["color"][0], ld[i]["color"][1], ld[i]["color"][2], ld[i]["intensity"]);
			}
		}
		catch (const nlohmann::json::exception& e) {
			std::cout << e.what() << '\n';
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
		Pcube.create();
		Plight.create();

		// Here you define the data set
		SC.pipelinesAndDescriptorSetsInit();
		txt.pipelinesAndDescriptorSetsInit();
	}

	// Here you destroy your pipelines and Descriptor Sets!
	// All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
	void pipelinesAndDescriptorSetsCleanup() {
		// Cleanup pipelines
		P.cleanup();
		Pcube.cleanup();
		Plight.cleanup();

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
		DSLlight.cleanup();

		// Destroies the pipelines
		P.destroy();
		Pcube.destroy();
		Plight.destroy();

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


	// //function for collision check
	// bool checkCollision(const CubeCollider& cube, const BoundingBox& box){
	// 	bool overlapX = cube.center.x >= box.min.x && cube.center.x <= box.max.x;
	// 	bool overlapY = cube.center.x >= box.min.y && cube.center.x <= box.max.y;
	// 	bool overlapZ = cube.center.z >= box.min.z && cube.center.x <= box.max.z;

	// 	std::cout << "cube "  << cube.center.x << " " << cube.center.y << " " << cube.center.z << "\n";
	// 	std::cout << "bbox min: "  << box.min.x << " " << box.min.y << " " << box.min.z << "\n";
	// 	std::cout << "bbox max: "  << box.max.x << " " << box.max.y << " " << box.max.z << "\n";

	// 	return overlapX && overlapY && overlapZ;
	// }

	bool checkCollision(const BoundingBox& box){
		bool overlapX = cubePosition.x >= box.min.x && cubePosition.x <= box.max.x;
		bool overlapY = cubePosition.y >= box.min.y && cubePosition.y <= box.max.y;
		bool overlapZ = cubePosition.z >= box.min.z && cubePosition.z <= box.max.z;

		std::cout << "Overlapping: "  << overlapX << " " << overlapY << " " << overlapZ << "\n";
		std::cout << "cube: "  << cubePosition.x << " " << cubePosition.y << " " << cubePosition.z << "\n";
		std::cout << "bbox min: "  << box.min.x << " " << box.min.y << " " << box.min.z << "\n";
		std::cout << "bbox max: "  << box.max.x << " " << box.max.y << " " << box.max.z << "\n";

		return overlapX && overlapY && overlapZ;
	}


	// place a bouding box on scene
	// TODO
	void placeBB(std::string mId, std::string iId, glm::mat4& World, std::unordered_map<std::string, BoundingBox>& bbMap){

		if(bbMap.find(iId) == bbMap.end()){
			BoundingBox bb;
			glm::vec4 homogeneousPoint;

			bb.min = glm::vec3(std::numeric_limits<float>::max());
			bb.max = glm::vec3(std::numeric_limits<float>::min());
			for(int j=0; j<SC.vecMap[mId].size(); j++){
				glm::vec3 vert = SC.vecMap[mId][j];
				glm::vec4 newVert = World * glm::vec4(vert,1.0f); 
				
				bb.min = glm::min(bb.min, glm::vec3(newVert));
				bb.max = glm::max(bb.max, glm::vec3(newVert));
			}
			bb.max = glm::round(bb.max * 100.0f) / 100.0f;
			bb.min = glm::round(bb.min * 100.0f) / 100.0f;
			(mId.substr(0, 4) == "coin") ? bb.cType = COLLECTIBLE
										 : bb.cType = OBJECT;
			bbMap[iId] = bb;
		}
	}

	float getTime() {
		static auto startTime = std::chrono::high_resolution_clock::now();
		static float lastTime = 0.0f;

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - startTime).count();
		float deltaT = time - lastTime;
		lastTime = time;
		deltaT = deltaT * 1e3;

		return deltaT;
	}


	void getJump() {
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJumping) {
			if (!debounce) {
				debounce = true;
				currDebounce = GLFW_KEY_SPACE;
				changeColor();
				isJumping = true;
				jumpSpeed = jumpForce;
			}
		}
	}


	// Control cube's movements
	void getActions() {

		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			cubeRotAngle += cubeRotSpeed * deltaTime;
			camRotation.x += cubeRotSpeed * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			cubeRotAngle -= cubeRotSpeed * deltaTime;
			camRotation.x -= cubeRotSpeed * deltaTime;
		}
		
		// Forward
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			cubePosition.x += cubeMovSpeed.x * glm::sin(glm::radians(cubeRotAngle)) * deltaTime;
			cubePosition.z += cubeMovSpeed.z * glm::cos(glm::radians(cubeRotAngle)) * deltaTime;
		}

		// Backward
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			cubePosition.x -= cubeMovSpeed.x * glm::sin(glm::radians(cubeRotAngle)) * deltaTime;
			cubePosition.z -= cubeMovSpeed.z * glm::cos(glm::radians(cubeRotAngle)) * deltaTime;
		}

		// Left 
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			cubePosition.x -= cubeMovSpeed.x * glm::cos(glm::radians(cubeRotAngle)) * deltaTime;
			cubePosition.z -= cubeMovSpeed.z * -glm::sin(glm::radians(cubeRotAngle)) * deltaTime;
		}

		// Right
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			cubePosition.x += cubeMovSpeed.x * glm::cos(glm::radians(cubeRotAngle)) * deltaTime;
			cubePosition.z += cubeMovSpeed.z * -glm::sin(glm::radians(cubeRotAngle)) * deltaTime;
		}

		/*
		// Down
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			cubePosition.y -= 1.0f;
		}*/


		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			camRotation.y += camRotSpeed * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			camRotation.y -= camRotSpeed * deltaTime;
		}

		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			camDistance -= camNFSpeed * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
			camDistance += camNFSpeed * deltaTime;
		}

	}


	// Change Cube's color
	void changeColor() {
		float r = (rand() % 100 + 1) / 100.0f;
		float g = (rand() % 100 + 1) / 100.0f;
		float b = (rand() % 100 + 1) / 100.0f;
		glm::vec3 col = glm::vec3(r, g, b);
		// std::cout << "color: " << r << ", " << g << ", " << b << ".\n";
		cubeColor = col;
	}

	// Function to print a glm::mat4 matrix
	void printMatrix(const glm::mat4& matrix) {
		for (int i = 0; i < 4; ++i) { // Loop through rows
			for (int j = 0; j < 4; ++j) { // Loop through columns
				std::cout << matrix[i][j] << " "; // Access and print each element
			}
			std::cout << std::endl; // Newline after each row
		}
	}


	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {


		// Standard procedure to quit when the ESC key is pressed
		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}


		const float fovY = glm::radians(90.0f);
		const float nearPlane = 0.01f;
		const float farPlane = 100.0f;

		glm::mat4 prjMatrix = glm::mat4(1.0f / (Ar * glm::tan(fovY / 2.0f)), 0, 0, 0,
			0, -1.0f / glm::tan(fovY / 2.0f), 0, 0,
			0, 0, farPlane / (nearPlane - farPlane), -1,
			0, 0, (nearPlane * farPlane) / (nearPlane - farPlane), 0);

		glm::mat4 baseMatrix = glm::mat4(1.0f);
		glm::mat4 worldMatrix;
		glm::mat4 viewPrjMatrix = prjMatrix * viewMatrix;

		deltaTime = getTime();

		// Need to check collisions first
		bool isCollision = false;
		std::string collisionId;
		for(auto bb : SC.bbMap) {
			// std::cout << "checking collision: " << bb.first << " "<< "\n";
			if(checkCollision(bb.second)) {
				isCollision = true;
				// Grab key of colliding object
				collisionId = bb.first;
				//std::cout << "\n\n" << "collision with" << collisionId << "\n";
				break;
			}
		}


		glm::vec3 cubeHalfSize = glm::vec3(0.5f,0.5f,0.5f);

		cubeMovSpeed = glm::vec3(0.01f,0.01f,0.01f);

		if(isCollision) {
			std::cout << "isCollision = " << isCollision << ";\n";

			switch(SC.bbMap[collisionId].cType){
				case OBJECT: {

					glm::vec3 cubeMin = cubePosition - cubeHalfSize;
					glm::vec3 cubeMax = cubePosition + cubeHalfSize;

					glm::vec3 closestPoint = glm::clamp(cubePosition, SC.bbMap[collisionId].min, SC.bbMap[collisionId].max);

					glm::vec3 difference = cubePosition - closestPoint;
					std::cout << "closest point: " << closestPoint.x <<" "<< closestPoint.y <<" "<< closestPoint.z << "\n";
					std::cout << "Cube position: " << cubePosition.x <<" "<< cubePosition.y <<" "<< cubePosition.z << "\n";
					std::cout << "difference of point: " << difference.x <<" "<< difference.y <<" "<< difference.z << "\n";

					float distance = glm::length(difference);
            		std::cout << "distance: " << distance << "\n";

					glm::vec3 normal = glm::normalize(difference);

					// Instead of moving the cube directly based on the normal, let's handle each axis separately
					glm::vec3 overlap(0.0f);

					if (cubeMax.x > SC.bbMap[collisionId].min.x && cubeMin.x < SC.bbMap[collisionId].max.x) {
						// Calculate overlap on the x-axis
						float overlapX1 = cubeMax.x - SC.bbMap[collisionId].min.x;
						float overlapX2 = SC.bbMap[collisionId].max.x - cubeMin.x;
						overlap.x = (overlapX1 < overlapX2) ? -overlapX1 : overlapX2;
					}

					if (cubeMax.y > SC.bbMap[collisionId].min.y && cubeMin.y < SC.bbMap[collisionId].max.y) {
						// Calculate overlap on the y-axis
						float overlapY1 = cubeMax.y - SC.bbMap[collisionId].min.y;
						float overlapY2 = SC.bbMap[collisionId].max.y - cubeMin.y;
						overlap.y = (overlapY1 < overlapY2) ? -overlapY1 : overlapY2;
					}

					if (cubeMax.z > SC.bbMap[collisionId].min.z && cubeMin.z < SC.bbMap[collisionId].max.z) {
						// Calculate overlap on the z-axis
						float overlapZ1 = cubeMax.z - SC.bbMap[collisionId].min.z;
						float overlapZ2 = SC.bbMap[collisionId].max.z - cubeMin.z;
						overlap.z = (overlapZ1 < overlapZ2) ? -overlapZ1 : overlapZ2;
					}

					// Resolve collision based on the smallest overlap axis
					if (std::abs(overlap.x) < std::abs(overlap.y) && std::abs(overlap.x) < std::abs(overlap.z)) {
						cubePosition.x += overlap.x;
						cubeMovSpeed.x = 0; // Reset the speed along the x-axis
					} else if (std::abs(overlap.y) < std::abs(overlap.x) && std::abs(overlap.y) < std::abs(overlap.z)) {
						cubePosition.y += overlap.y;
						cubeMovSpeed.y = 0; // Reset the speed along the y-axis
					} else {
						cubePosition.z += overlap.z;
						cubeMovSpeed.z = 0; // Reset the speed along the z-axis
					}

					std::cout << "cubePosition = " << cubePosition.x << " " << cubePosition.y << " " << cubePosition.z << ";\n";
					std::cout << "cubeMovSpeed = " << cubeMovSpeed.x << " " << cubeMovSpeed.y << " " << cubeMovSpeed.z << ";\n";

					break;
				}

				case COLLECTIBLE: {

				}
			}



		}


		if (glfwGetKey(window, GLFW_KEY_V)) {
			if (!debounce) {
				debounce = true;
				currDebounce = GLFW_KEY_V;

				printVec3("Cube position", cubePosition);
				printFloat("DeltaTime", deltaTime);

			}
		}
		else {
			if ((currDebounce == GLFW_KEY_V) && debounce) {
				debounce = false;
				currDebounce = 0;
			}
		}


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

		for (int i = 0; i < n_lights; i++) {
			gubo.lightDir[i].v = lDir[i];
			gubo.lightPos[i].v = lPos[i];
			gubo.lightColor[i] = lCol[i];
		}

		// cube light
		gubo.lightDir[n_lights].v = glm::normalize(glm::vec3(camPosition.x - cubePosition.x, 0.0f, camPosition.z - cubePosition.z));
		gubo.lightPos[n_lights].v = cubePosition;
		gubo.lightColor[n_lights] = glm::vec4(cubeColor, 10.0f);
		
		/*
		gubo.lightDir[1].v = glm::normalize(glm::vec3(camPosition.x - cubePosition.x, 0.0f, camPosition.z - cubePosition.z));
		gubo.lightPos[1].v = cubePosition;
		gubo.lightColor[1] = glm::vec4(cubeColor, 15.0f);
		gubo.lightDir[2].v = glm::vec3(0.0f, 1.0f, 0.0f);
		gubo.lightPos[2].v = glm::vec3(0.0f, 160.0f, 0.0f);
		gubo.lightColor[2] = glm::vec4(1.0f, 0.0f, 1.0f, 50.0f);
		*/
		

		gubo.eyePos = camPosition;
		gubo.eyeDir = glm::vec4(0);
		gubo.eyeDir.w = 1.0;
		gubo.lightOn = glm::vec3(0.0f, 1.0f, 1.0f);
		gubo.cosIn = cos(0.4591524628390111f);
		gubo.cosOut = cos(0.5401793718338013f);



		// Draw the landscape
		for (std::vector<std::string>::iterator it = staticObj.begin(); it != staticObj.end(); it++) {
			int i = SC.InstanceIds[it->c_str()];
			// Product per transform matrix
			// staticUbo.mMat = baseMatrix * SC.I[i]->Wm * SC.M[SC.I[i]->Mid]->Wm;
			staticUbo.mMat = baseMatrix * SC.I[i]->Wm;
			staticUbo.mvpMat = viewPrjMatrix * staticUbo.mMat;
			staticUbo.nMat = glm::inverse(glm::transpose(staticUbo.mMat));
      
			// placeBB(it->c_str(), it->c_str(), SC.I[i]->Wm, SC.bbMap);
			
			SC.I[i]->DS[0]->map(currentImage, &staticUbo, sizeof(staticUbo), 0);
			SC.I[i]->DS[0]->map(currentImage, &gubo, sizeof(gubo), 2);
		}
		std::vector<std::string>::iterator it;
		int k;
		/* k = 1 starts from first point light */
		for (it = lightObj.begin(),  k = 1; it != lightObj.end(); it++, k++) {
			int i = SC.InstanceIds[it->c_str()];
			//std::cout << *it << " " << i << "\n";
						// Product per transform matrix
			
			lightUbo.mMat = baseMatrix * SC.I[i]->Wm;
			lightUbo.mvpMat = viewPrjMatrix * lightUbo.mMat;
			lightUbo.nMat = glm::inverse(glm::transpose(lightUbo.mMat));
			// Light id
			lightUbo.id = k;

			SC.I[i]->DS[0]->map(currentImage, &lightUbo, sizeof(lightUbo), 0);
			SC.I[i]->DS[0]->map(currentImage, &gubo, sizeof(gubo), 2);
		}

		for (std::vector<std::string>::iterator it = gadgetObj.begin(); it != gadgetObj.end(); it++) {
			int i = SC.InstanceIds[it->c_str()];
			
			//std::cout << *it << " " << i << "\n";
						// Product per transform matrix
			staticUbo.mMat = baseMatrix * SC.I[i]->Wm * SC.M[SC.I[i]->Mid]->Wm;
			staticUbo.mvpMat = viewPrjMatrix * staticUbo.mMat;
			staticUbo.nMat = glm::inverse(glm::transpose(staticUbo.mMat));
			SC.I[i]->DS[0]->map(currentImage, &staticUbo, sizeof(staticUbo), 0);
			SC.I[i]->DS[0]->map(currentImage, &gubo, sizeof(gubo), 2);
		}

		// TODO: try above to use placeBB and try collision
		// std::string obj_id = "cube2";
		// placeBB("cube2", "cube2", SC.I[SC.InstanceIds[obj_id]]->Wm, SC.bbMap);
		// Print the matrix
    	// std::cout << "Matrix: " << std::endl;
    	// printMatrix(SC.I[SC.InstanceIds[obj_id]]->Wm);

		getJump();

		if (isJumping) {
			cubePosition.y += jumpSpeed;
			jumpSpeed += gravity * deltaTime;

			if (cubePosition.y <= groundLevel) {
				cubePosition.y = groundLevel;
				isJumping = false;
				jumpSpeed = 0.0f;
			}

			debounce = false;
			currDebounce = 0;

		}

		
		worldMatrix = glm::translate(glm::mat4(1.0f), cubePosition);
		worldMatrix *= glm::rotate(glm::mat4(1.0f), glm::radians(cubeRotAngle),
			glm::vec3(0.0f, 1.0f, 0.0f));

		getActions();

		cubePosition.x = glm::clamp(cubePosition.x, -mapLimit, mapLimit);
		cubePosition.z = glm::clamp(cubePosition.z, -mapLimit, mapLimit);
		cubePosition.y = glm::clamp(cubePosition.y, 0.0f, 16.0f);

		camDistance = glm::clamp(camDistance, minCamDistance, maxCamDistance);
		camRotation.y = glm::clamp(camRotation.y, 0.0f, 89.0f);

		glm::vec3 newCamPosition = glm::normalize(glm::vec3(sin(glm::radians(cubeRotAngle)),
			sin(glm::radians(camRotation.y)),
			cos(glm::radians(cubeRotAngle)))) * camDistance + cubePosition + glm::vec3(0.0f, 0.06f, 0.0f);

		float oldCamRoty = camRotation.y;

		float dampLambda = 10.0f;

		if (abs(newCamPosition.x) > mapLimit - 0.01f || abs(newCamPosition.z) > mapLimit - 0.01f) {
			camRotation.y = camRotation.y * exp(-dampLambda * deltaTime) + 30.0f * (1 - exp(-dampLambda * deltaTime));
		}
		else {
			camRotation.y = oldCamRoty;
		}

		newCamPosition.x = glm::clamp(newCamPosition.x, -mapLimit + 0.02f, mapLimit-0.02f);
		newCamPosition.z = glm::clamp(newCamPosition.z, -mapLimit + 0.02f, mapLimit-0.02f);
		newCamPosition.y = glm::clamp(newCamPosition.y, 0.06f, 16.0f);



		camPosition = camPosition * exp(-dampLambda * deltaTime)  + newCamPosition * (1-exp(-dampLambda * deltaTime));


		viewMatrix = glm::lookAt(camPosition, cubePosition, glm::vec3(0.0f, 1.0f, 0.0f));


		int i = SC.InstanceIds[cubeObj];
		cubeUbo.mMat = baseMatrix * worldMatrix * SC.M[SC.I[i]->Mid]->Wm * SC.I[i]->Wm;
		cubeUbo.mvpMat = viewPrjMatrix * cubeUbo.mMat;
		cubeUbo.nMat = glm::inverse(glm::transpose(cubeUbo.mMat));
		cubeUbo.col = cubeColor;

		SC.I[i]->DS[0]->map(currentImage, &cubeUbo, sizeof(cubeUbo), 0);
		SC.I[i]->DS[0]->map(currentImage, &gubo, sizeof(gubo), 2);

		camPosition = newCamPosition;
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
